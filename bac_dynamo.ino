#include <LedControl.h>
#include <FastLED.h>
#include <ArduinoSTL.h>

using namespace std;

#define NB_VELO 8 // nombre de vélos

typedef struct {
    int raw;               // mesure brute renvoyée par analogRead
    int prod;              // production instantannée en Watts
    int pic;               // pic de production en Watts
    unsigned long dernier; // timestamp du dernier relevé
} velo_t;

velo_t velo[NB_VELO];

struct {
    int prod;                // production instantannée totale en Watts
    float cumul;             // production totale cumulee en Watts/heure
    unsigned long temps = 0; // temps écoulé depuis le lancement
} global;

/* ********************************************************** */
/* Communication avec les MAX7219 pour piloter les afficheurs */
/* ********************************************************** */

#define CS_GLOBAL 9
#define CS_VELO 10
#define MOSI 11
#define CLK 13

LedControl lcVelo = LedControl(MOSI, CLK, CS_VELO, NB_VELO);

/*
 * initialise les modules d'affichage pour les vélos
 */

void setupVelo() {
    for (int i=0; i<NB_VELO; i++) {
        lcVelo.setScanLimit(i,6);
        lcVelo.setIntensity(i,8);
        lcVelo.clearDisplay(i);
        lcVelo.shutdown(i,false);
    }
}

/*
 * initalise les modules d'affichage des mesures globales
 */

void setupGlobal() {
    for (int i=0; i<4; i++) {
        lcVelo.setScanLimit(i,8);
        lcVelo.setIntensity(i,8);
        lcVelo.clearDisplay(i);
        lcVelo.shutdown(i,false);
    }
}

/*
 * sur les petits affichages, deux nombres composés de trois chiffres sont
 * affichés. Les deux nombres à afficher sont transmis sous la forme d'un
 * entier au format XXXYYY
 */

long combiner3(long x, long y) {
    if (x > 999) x = 999;
    if (y > 999) y = 999;
    return x*1000 + y;
}

/*
 * Mise à jour de tous les affichages vélo.
 */

void updateVelo() {
    long num;
    for (int i=0; i<NB_VELO; i++) {
        num = combiner3(velo[i].prod, velo[i].pic);
        for (int j=5; j>=0; j--) {
            lcVelo.setDigit(i, j, num % 10, false);
            num = num/10;
        }
    }
}

/*
 * mise à jour des affichages des valeurs globales
 */

void updateGlobal() {

}

/* ************************* */
/* Pilotage du ruban de LEDs */
/* ************************* */

#define PIN_RUBAN 6 // pin du ruban de LED
#define NB_LEDS 24 // nombre de LEDs sur le ruban

CRGB leds[NB_LEDS];
CRGB colors[NB_LEDS] = {
    CHSV(160,255,255),
    CHSV(150,255,255),
    CHSV(140,255,255),
    CHSV(130,255,255),
    CHSV(120,255,255),
    CHSV(110,255,255),
    CHSV(96,255,255),
    CHSV(91,255,255),
    CHSV(86,255,255),
    CHSV(81,255,255),
    CHSV(76,255,255),
    CHSV(71,255,255),
    CHSV(64,255,255),
    CHSV(59,255,255),
    CHSV(54,255,255),
    CHSV(49,255,255),
    CHSV(44,255,255),
    CHSV(39,255,255),
    CHSV(32,255,255),
    CHSV(26,255,255),
    CHSV(20,255,255),
    CHSV(14,255,255),
    CHSV(8,255,255),
    CHSV(0,255,255),
};

int niveau;

void setupRuban() {
    pinMode(PIN_RUBAN, OUTPUT);
    FastLED.addLeds<NEOPIXEL, PIN_RUBAN>(leds, NB_LEDS);
    /* for (int i=0; i<NB_LEDS; i++) { */
    /*     colors[i] = CHSV(80+i*255/NB_LEDS, 255, 255); */
    /* } */
}

const int echelle[24] = {
    13,26,32,48,64,80, // premier palier 80W
    125,170,215,260,305,350, // deuxième palier, 350W
    458,566,675,783,891,1000, // troisième palier, 1000W
    1333,1666,2000,2333,2666,3000 // dernier palier, 3000W
};

void updateRuban() {
    int niv = 0;
    while (niv<24 && echelle[niv] <= global.prod)
        niv++;

    if (niv > niveau) {
        int dur = 500 / (niv-niveau);
        for (int i=niveau; i<niv; i++) {
            leds[i] = colors[i];
            FastLED.show();
            delay(dur);
        }
    } else if (niv < niveau) {
        int dur = 500 / (niveau-niv);
        for (int i=niveau; i>=niv; i--) {
            leds[i] = CRGB::Black;
            FastLED.show();
            delay(dur);
        }
    }
    niveau = niv;
}


/* ******************************** */
/* MESURES DE COURANT SUR LES VELOS */
/* ******************************** */

const byte amp_pins[8] = {A0, A1, A2, A3, A4, A5, A6, A7};

void setupMesure() {
    for (int i=0; i<NB_VELO; i++) {
        pinMode(amp_pins[i], INPUT_PULLUP);
    }
}

/* Le module ACS712 peut mesurer un courant allant jusqu'à 20A, positif ou
 * négatif. La correspondance entre le courant mesuré et la tension en sortie
 * du module est donc la suivante :
 * 
 *  20A -->   5V --> 1023
 *   0A --> 2.5V -->  512
 * -20A -->   0V -->    0
 *
 */

// facteur permettant de faire la conversion analogRead() -> Courant mesuré
const float ampParUnit = 20 / 512.;
// tension produite par un générateur
const int tensionGen = 26;

/* Convertit la tension mesurée par une entrée analogique en puissance fournie
 * par le générateur.
 */

int calculProd(int raw) {
    float intensite = (raw-512) * ampParUnit;
    if (intensite < 0) intensite = -intensite; // au cas ou le câble serait branché à l'envers
    int puissance = intensite * tensionGen;
    return puissance;
}

const int sensibilite = 5;

void updateMesure() {
    global.prod = 0;
    for (int i=0; i<NB_VELO; i++) {
        int raw = analogRead(amp_pins[i]);
        // on ignore toute variation inférieure à la sensibilité, pour limiter les flottements
        if (abs(velo[i].raw - raw) > sensibilite) {
            velo[i].raw = raw;
            velo[i].prod = calculProd(raw);
        }
        global.prod += velo[i].prod;
        if (velo[i].prod > velo[i].pic) {
            velo[i].pic = velo[i].prod;
        }
        cout << velo[i].prod << " ";
    }
    cout << endl;
    // puissance produite depuis la dernière mesure
    float dur = (millis() - global.temps) / 1000. / 3600.; // temps écoulé en heures
    global.cumul += dur * global.prod; // ajout de la puissance produite en Watts/h
}

/* ******************** */
/* FONCTION PRINCIPALES */
/* ******************** */

void setup() {
    Serial.begin(9600);
    setupMesure();
    setupVelo();
    /* setupGlobal(); */
    setupRuban();
    global.temps = millis();
}

void loop() {
    updateMesure();
    updateVelo();
    /* updateGlobal(); */
    updateRuban();
    delay(1000);
}
