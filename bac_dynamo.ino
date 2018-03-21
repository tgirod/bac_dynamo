#include <LedControl.h>
#include <FastLED.h>

#define DEBUG Serial.println(__func__)
#define NB_INDIV 8 // nombre de vélos

typedef struct {
    int prod;              // production instantannée en Watts
    int pic;               // pic de production en Watts
    unsigned long dernier; // timestamp du dernier relevé
} velo_t;

velo_t velo[NB_INDIV];

unsigned long temps = 0; // temps écoulé depuis le lancement
int prod;                // production instantannée totale en Watts
float prod_cumul;        // production totale cumulée en Watts/heure

/* ********************************************************** */
/* Communication avec les MAX7219 pour piloter les afficheurs */
/* ********************************************************** */

#define CS_GLOBAL 9
#define CS_INDIV 10
#define MOSI 11
#define CLK 13

LedControl lcIndiv = LedControl(MOSI, CLK, CS_INDIV, NB_INDIV);

/*
 * initialise les modules d'affichage pour les vélos
 */

void setupIndiv() {
    for (int i=0; i<NB_INDIV; i++) {
        lcIndiv.setScanLimit(i,6);
        lcIndiv.setIntensity(i,8);
        lcIndiv.clearDisplay(i);
        lcIndiv.shutdown(i,false);
    }
}

/*
 * initalise les modules d'affichage des mesures globales
 */

void setupGlobal() {
    for (int i=0; i<4; i++) {
        lcIndiv.setScanLimit(i,8);
        lcIndiv.setIntensity(i,8);
        lcIndiv.clearDisplay(i);
        lcIndiv.shutdown(i,false);
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

void updateIndiv() {
    long num;
    for (int i=0; i<NB_INDIV; i++) {
        num = combiner3(velo[i].prod, velo[i].pic);
        for (int j=5; j>=0; j--) {
            lcIndiv.setDigit(i, j, num % 10, false);
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
    while (niv<24 && echelle[niv] <= prod)
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

void mesurer() {
    prod = 0;
    for (int i=0; i<NB_INDIV; i++) {
        velo[i].prod = random(0,200); // FIXME remplacer par la vraie mesure
        prod += velo[i].prod;
        if (velo[i].prod > velo[i].pic) {
            velo[i].pic = velo[i].prod;
        }
        /* Serial.print("velo "); */
        /* Serial.print(i); */
        /* Serial.print(" : "); */
        /* Serial.println(velo[i].prod); */
    }
    unsigned long dur = millis() - temps;
    prod_cumul += (dur / 1000.0) * prod / 3600.0; 
    Serial.print("prod actuelle ");
    Serial.println(prod);
    Serial.print("prod totale ");
    Serial.println(prod_cumul);
}

/* ******************** */
/* FONCTION PRINCIPALES */
/* ******************** */

void setup() {
    Serial.begin(9600);
    setupIndiv();
    /* setupGlobal(); */
    setupRuban();
    /* temps = millis(); */
}

void loop() {
    mesurer();
    updateIndiv();
    /* updateGlobal(); */
    updateRuban();
    delay(1000);
}
