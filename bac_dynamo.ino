#include <LedControl.h>

#define DEBUG Serial.println(__func__)
#define NB_INDIV 8

typedef struct {
    int prod; // production instantannée en Watts
    int pic; // pic de production en Watts
    unsigned long dernier; // timestamp du dernier relevé
} velo_t;

velo_t velo[NB_INDIV];

unsigned long temps = 0; // temps écoulé depuis le lancement
int prod; // production instantannée totale en Watts
int prod_cumul; // production totale cumulée en Watts/heure

/*
 * Communication avec les MAX7219 pour piloter les afficheurs
 */

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

/*
 * relever les mesures de courant sur les vélos
 */

void mesurer() {
    for (int i=0; i<NB_INDIV; i++) {
        velo[i].prod++;
        velo[i].pic++;
    }
}

void setup() {
    Serial.begin(9600);
    for (int i=0; i<NB_INDIV; i++) {
        velo[i].prod= i;
        velo[i].pic = i*10;
    }

    setupIndiv();
    /* setupGlobal(); */
    /* temps = millis(); */
}

void loop() {
    mesurer();
    updateIndiv();
    /* updateGlobal(); */
    delay(1000);
}
