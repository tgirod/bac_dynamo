#include <SPI.h>

#define NB_VELO 2

typedef struct {
    int prod; // production instantannée en Watts
    int pic; // pic de production en Watts
    unsigned long dernier; // timestamp du dernier relevé
} velo_t;

velo_t velo[NB_VELO] = {{0,0,0}};

unsigned long temps = 0; // temps écoulé depuis le lancement
int prod; // production instantannée totale en Watts
int prod_cumul; // production totale cumulée en Watts/heure

/*
 * Communication avec les MAX7219 pour piloter les afficheurs
 */

#define CS_GLOBAL 9
#define CS_VELO 10
#define MOSI 11
#define MISO 12 // a priori inutile
#define CLK 13

// Opcodes
#define OP_NOOP         0x00
#define OP_DIGIT0       0x01
#define OP_DIGIT1       0x02
#define OP_DIGIT2       0x03
#define OP_DIGIT3       0x04
#define OP_DIGIT4       0x05
#define OP_DIGIT5       0x06
#define OP_DIGIT6       0x07
#define OP_DIGIT7       0x08
#define OP_DECODEMODE   0x09
#define OP_INTENSITY    0x0A
#define OP_SCANLIMIT    0x0B
#define OP_SHUTDOWN     0x0C
#define OP_DISPLAYTEST  0x0F

/*
 * Paramètres pour la connexion série. Potentiellement utile si j'ai des soucis
 * de transmission
 */

const SPISettings settings = SPISettings(
        10000000, // maximum speed
        MSBFIRST, // bit order
        SPI_MODE0 // data mode
        );

/*
 * FIXME : utiliser SPISettings pour initialiser la liaison
 */

#define OPEN(CS) digitalWrite((CS), LOW)
#define SEND(ADDR,DATA) SPI.transfer16((ADDR<<8)+(DATA))
#define CLOSE(CS) digitalWrite((CS), HIGH)

void setupSPI() {
    pinMode(CS_VELO, OUTPUT);
    digitalWrite(CS_VELO, HIGH);
    pinMode(CS_GLOBAL, OUTPUT);
    digitalWrite(CS_GLOBAL, HIGH);
    SPI.setBitOrder(MSBFIRST);
    SPI.begin();
}

/*
 * initialise les modules d'affichage pour les vélos
 */

void setupVelo() {
    // mode B, permet d'afficher 0-9
    OPEN(CS_VELO);
    for (int i=0; i<NB_VELO; i++) SEND(OP_DECODEMODE, 0xFF);
    CLOSE(CS_VELO);

    // réglage de l'intensité de l'affichage
    OPEN(CS_VELO);
    for (int i=0; i<NB_VELO; i++) SEND(OP_INTENSITY, 0xFF);
    CLOSE(CS_VELO);

    // Scan 6 digits
    OPEN(CS_VELO);
    for (int i=0; i<NB_VELO; i++) SEND(OP_SCANLIMIT, 0x05);
    CLOSE(CS_VELO);

    // Activation de la puce
    OPEN(CS_VELO);
    for (int i=0; i<NB_VELO; i++) SEND(OP_SHUTDOWN, 0x01);
    CLOSE(CS_VELO);
}

/*
 * initalise les modules d'affichage des mesures globales
 */

void setupGlobal() {

    // mode B, permet d'afficher 0-9
    OPEN(CS_GLOBAL);
    for (int i=0; i<4; i++) SEND(OP_DECODEMODE, 0xFF);
    CLOSE(CS_GLOBAL);

    // réglage de l'intensité de l'affichage
    OPEN(CS_GLOBAL);
    for (int i=0; i<4; i++) SEND(OP_INTENSITY, 0x00);
    CLOSE(CS_GLOBAL);

    // Scan 6 digits
    OPEN(CS_GLOBAL);
    for (int i=0; i<4; i++) SEND(OP_SCANLIMIT, 0x05);
    CLOSE(CS_GLOBAL);

    // Activation de la puce
    OPEN(CS_GLOBAL);
    for (int i=0; i<4; i++) SEND(OP_SHUTDOWN, 0x01);
    CLOSE(CS_GLOBAL);
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
    // pour chaque vélo, on combine les deux nombres à afficher en un seul
    long data[NB_VELO];
    for (int i=0; i<NB_VELO; i++) {
        data[i] = combiner3(velo[i].prod, velo[i].pic);
    }

    // chaque digit doit être envoyé séparément, en commençant par la fin
    for (word addr = OP_DIGIT5; addr >= OP_DIGIT0; addr--) {
        OPEN(CS_VELO);
        // pour éviter les NOOP, on actualise DIGITx en même temps sur tous les
        // modules
        for (int i = NB_VELO-1; i>=0; i--) {
            SEND(addr, data[i]%10);
            data[i] = data[i]/10;
        }
        CLOSE(CS_VELO);
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
    for (int i=0; i<NB_VELO; i++) {
        velo[i].prod++;
        velo[i].pic++;
    }
}

void setup() {
    Serial.begin(9600);
    setupSPI();
    setupVelo();
    /* setupGlobal(); */
    temps = millis();
}

void loop() {
    mesurer();
    updateVelo();
    /* updateGlobal(); */
    delay(1000);
}
