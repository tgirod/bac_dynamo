#include <LedControl.h>
#include <FastLED.h>

#define NB_VELO 8 // nombre de vélos

// les 4 grosses parties du programme
//#define MESURE // activer la mesure réelle
//#define GLOBAL // activer l'affichage global
//#define VELO // activer l'affichage individuel
#define RUBAN // activer le ruban de led

// état interne

typedef struct {
    int prod;              // production instantannée en Watts
    int pic;               // pic de production en Watts
} velo_t;

velo_t velo[NB_VELO];

struct {
    int prod;                // production instantannée totale en Watts
    float cumul;             // production totale cumulee en Watts/heure
    unsigned long temps = 0; // milliseconde à laquelle la dernière mesure à été faite
} global;

#define CLK 9
#define MOSI 8

/* ******************************************* */
/* GESTION DE L'AFFICHAGE DES VALEURS GLOBALES */ 
/* ******************************************* */

#ifdef GLOBAL

#define CS_GLOBAL 11

// controlleur des afficheurs globaux
LedControl lcGlobal = LedControl(MOSI, CLK, CS_GLOBAL, 4);

/*
 * initalise les modules d'affichage des mesures globales
 */

void setupGlobal() {
    Serial.println(__FUNCTION__);
    for (int i=0; i<4; i++) {
        lcGlobal.setScanLimit(i,4);
        lcGlobal.setIntensity(i,8);
        lcGlobal.clearDisplay(i);
        lcGlobal.shutdown(i,false);
    }
}

/*
 * mise à jour des affichages des valeurs globales
 */

void updateGlobal() {
    Serial.println(__FUNCTION__);
    // MAJ temps écoulé
    unsigned int secondes = millis() / 1000;
    int hh = secondes / 3600; // nb d'heures à afficher
    int mm = (secondes % 3600) / 60;  // nb de minutes à afficher
    lcGlobal.setDigit(0, 0, hh/10, false);
    lcGlobal.setDigit(0, 1, hh%10, true);
    lcGlobal.setDigit(0, 2, mm/10, false);
    lcGlobal.setDigit(0, 3, mm%10, false);

    // MAJ prod courante
    int prod = global.prod;
    for (int i=3; i>=0; i--) {
        lcGlobal.setDigit(1, i, prod % 10, false);
        prod = prod/10;
    }

    // MAJ prod cumulée depuis le début
    int cumul = global.cumul;
    for (int i=3; i>=0; i--) {
        lcGlobal.setDigit(2, i, cumul % 10, false);
        cumul = cumul/10;
    }

    // ????
}

#endif


/* ******************************** */
/* GESTION DE L'AFFICHAGE DES VELOS */
/* ******************************** */

#ifdef VELO

#define CS_VELO 10

LedControl lcVelo = LedControl(MOSI, CLK, CS_VELO, NB_VELO);


/*
 * initialise les modules d'affichage pour les vélos
 */

void setupVelo() {
    Serial.println(__FUNCTION__);
    for (int i=0; i<NB_VELO; i++) {
        lcVelo.setScanLimit(i,6);
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
    Serial.println(__FUNCTION__);
    long num;
    for (int i=0; i<NB_VELO; i++) {
        num = combiner3(velo[i].prod, velo[i].pic);
        for (int j=5; j>=0; j--) {
            lcVelo.setDigit(i, j, num % 10, false);
            num = num/10;
        }
    }
}

#endif

/* ************************* */
/* PILOTAGE DU RUBAN DE LEDS */
/* ************************* */

#ifdef RUBAN

#define PIN_RUBAN 12 // pin du ruban de LED
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
    Serial.println(__FUNCTION__);
    pinMode(PIN_RUBAN, OUTPUT);
    FastLED.addLeds<NEOPIXEL, PIN_RUBAN>(leds, NB_LEDS);
}

const int echelle[24] = {
    13,26,32,48,64,80, // premier palier 80W
    125,170,215,260,305,350, // deuxième palier, 350W
    458,566,675,783,891,1000, // troisième palier, 1000W
    1333,1666,2000,2333,2666,3000 // dernier palier, 3000W
};

void updateRuban() {
    Serial.println(__FUNCTION__);
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

#endif

/* ******************************** */
/* MESURES DE COURANT SUR LES VELOS */
/* ******************************** */


const byte amp_pins[8] = {A0, A1, A2, A3, A4, A5, A6, A7};

void setupMesure() {
    Serial.println(__FUNCTION__);
    for (int i=0; i<NB_VELO; i++) {
        pinMode(amp_pins[i], INPUT);
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

const int sensibilite = 5;

#ifdef MESURE

void updateMesure() {
    Serial.println(__FUNCTION__);
    global.prod = 0;
    for (int i=0; i<NB_VELO; i++) {
        // lire l'entrée analogique
        int raw = analogRead(amp_pins[i]);
        // convertir la mesure en puissance (Watts)
        float intensite = abs(raw-512) * ampParUnit;
        int puissance = intensite * tensionGen;

        // ignorer toute mesure inférieure à la sensibilité
        if (puissance < sensibilite) {
            puissance = 0;
        }

        // on ne change la mesure que si elle diffère suffisament de la précédente
        if (abs(velo[i].prod - puissance) > sensibilite) {
            velo[i].prod = puissance;
        }

        if (velo[i].prod > velo[i].pic) {
            velo[i].pic = velo[i].prod;
        }

        global.prod += velo[i].prod;
    }
    // puissance produite depuis la dernière mesure
    unsigned long t = millis();
    float dur = (t - global.temps) / 1000. / 3600.; // temps écoulé en heures
    global.cumul += dur * global.prod; // ajout de la puissance produite en Watts/h
    global.temps = t;
}

#else

void updateMesure() {
    Serial.print(__FUNCTION__);
    Serial.println("_FAKE");
    global.prod = 0;
    for (int i=0; i<NB_VELO; i++) {
        velo[i].prod = random(0,200);
        if (velo[i].prod > velo[i].pic) {
            velo[i].pic = velo[i].prod;
        }
        global.prod += velo[i].prod;
    }
    unsigned long t = millis();
    float dur = (t - global.temps) / 1000. / 3600.; // temps écoulé en heures
    global.cumul += dur * global.prod; // ajout de la puissance produite en Watts/h
    global.temps = t;
}

#endif

/* ******************** */
/* FONCTION PRINCIPALES */
/* ******************** */

void setup() {
    Serial.println(__FUNCTION__);
    Serial.begin(9600);
#ifdef MESURE
    setupMesure();
#endif
#ifdef VELO
    setupVelo();
#endif
#ifdef GLOBAL
    setupGlobal();
#endif
#ifdef RUBAN
    setupRuban();
#endif
    global.temps = millis();
}

void loop() {
    Serial.println(__FUNCTION__);
    updateMesure();
    for (int i=0; i<NB_VELO; i++) {
        Serial.print(velo[i].prod);
        Serial.print(" ");
    }
    Serial.print(" = ");
    Serial.println(global.prod);

#ifdef VELO
    updateVelo();
#endif
#ifdef GLOBAL
    updateGlobal();
#endif
#ifdef RUBAN
    updateRuban();
#endif
    delay(1000);
}
