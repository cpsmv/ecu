#include <TimerOne.h>

#define MAP_OUT 5
#define TAC_OUT 13

#define SPARK_IN 2
#define FUEL_IN 3

#define TIC_TAC_AT_1K_RPM 4616

#define MAX_TEETH 13

const unsigned int prescalers[] = {
    0, 1, 8, 64, 256, 1024
};

volatile int tooth;
volatile unsigned int mapVal;

volatile unsigned int currSparkTime;
volatile unsigned int oldSparkTime;

volatile unsigned int sparkChargeTime;

volatile char printSpark;
volatile int chargeAtTooth;
volatile int sparkAtTooth;

volatile int fuelStartTooth;
volatile int fuelEndTooth;


void setup() {
    Serial.begin(115200);
    pinMode(TAC_OUT, OUTPUT);
    pinMode(MAP_OUT, OUTPUT);

    mapVal = 128;
    tooth = 0;
    printSpark = 0;
    printFuel = 0;

    attachInterrupt(0, sparkCharging, FALLING);
    attachInterrupt(0, spark, FALLING);

    attachInterrupt(1, fuelStart, RISING);
    attachInterrupt(1, fuelEnd, RISING);

    configTimer(TIC_TAC_AT_1K_RPM);
    analogWrite(MAP_OUT, mapVal);
}


void loop() {
    static char input;
    if (Serial.available() > 0) {
        input = Serial.read();
        if (input == 'a')
            increaseMAP();
        if (input == 'z')
            decreaseMAP();
        else {
            Serial.print("current map: ");
            Serial.println(mapVal);
        }
    }

    if (printSpark) {
        Serial.print("current spark based rpm: ");
        Serial.println(6E7 / (currSparkTime - oldSparkTime));
        Serial.print("dwell time (us): ");
        Serial.println(currSparkTime - sparkChargeTime);
        Serial.print("tooth during spark: ");
        Serial.println(sparkAtTooth);
        Serial.print("tooth during fuel start: ");
        Serial.println(fuelStartTooth);
        Serial.print("tooth during fuel end: ");
        Serial.println(fuelEndTooth);
        Serial.println();
        printSpark = 0;
    }

}

// send tac signals
ISR(TIMER1_COMPA_vect) {
    tooth = (tooth + 1) % MAX_TEETH;
    if (tooth != 0) {
        digitalWrite(TAC_OUT, HIGH);
        delayMicroseconds(1000);
        digitalWrite(TAC_OUT, LOW);
    }
}

void increaseMAP() {
    if (mapVal < 246) {
        mapVal += 10;
        analogWrite(MAP_OUT, mapVal);
    }
}

void decreaseMAP() {
    if (mapVal > 9) {
        mapVal -= 10;
        analogWrite(MAP_OUT, mapVal);
    }
}

void fuelStart() {
    fuelStartTooth = tooth;
}

void fuelEnd() {
    fuelEndTooth = tooth;
}

void chargeSpark() {
    sparkChargeTime = micros();
    chargeAtTooth = tooth;
}

void spark() {
    oldSparkTime = currSparkTime;
    currSparkTime = micros();
    sparkAtTooth = tooth;
    printSpark = 1;
}

void configTimer(long m) {
    noInterrupts();
    int i;
    TCCR1A = 0;
    TCCR1B = 0 | (1 << WGM12);
    TCNT1 = 0;
    TIMSK1 |= (1 << OCIE1A);
    for (i = 1; i < 6; i++) {
        if (m / prescalers[i] * 16 < 65535) {
            TCCR1B |= i;
            OCR1A = m / prescalers[i] * 16;
            break;
        }
    }
    interrupts();
}
