
#include <TimerOne.h>
/*
The virtual Honda engine
 */

#define TAC_OUT 13
#define MAP_OUT 5

#define FUEL_IN 3
#define SPARK_IN 2


#define TAC_TIMER Timer1

#define TOOTH_COUNT 12

#define TAC_DEVIATION 0.15;

volatile int currTooth;

volatile unsigned int mapVal;

volatile unsigned int tacTimer;

volatile unsigned int currSparkTime;
volatile unsigned int oldSparkTime;

volatile int printSpark;

void setup() {
   Serial.begin(115200);
   pinMode(TAC_OUT, OUTPUT);
   pinMode(MAP_OUT, OUTPUT);

   mapVal = 128;
   printSpark = 0;

   currTooth = 0;

   attachInterrupt(0, spark, FALLING);

   tacTimer = 60000/TOOTH_COUNT;
   TAC_TIMER.initialize(tacTimer);
   TAC_TIMER.attachInterrupt(tacSignal);
   TAC_TIMER.start();

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
      if (input == '-')
         decreaseRPM();
      if (input == '+')
         increaseRPM();
   }

   if (printSpark % 20 == 0) {
      //Serial.print("current spark based rpm: ");
      //Serial.println(6E7 / (currSparkTime - oldSparkTime));
      Serial.print("current tac timer: ");
      Serial.println(tacTimer);
   }
}

// send tac signals
void tacSignal() {
   int deviation = tacTimer * TAC_DEVIATION;
   TAC_TIMER.setPeriod(tacTimer + random(-1*deviation,deviation));
   if(++currTooth % TOOTH_COUNT) {
      digitalWrite(TAC_OUT, HIGH);
      delayMicroseconds(100);
      digitalWrite(TAC_OUT, LOW);
   }
}

void increaseMAP() {
   if (mapVal < 246) {
      mapVal += 10;
      analogWrite(MAP_OUT, mapVal);
      Serial.print("current map: ");
      Serial.println(mapVal);
   }
}

void decreaseMAP() {
   if (mapVal > 9) {
      mapVal -= 10;
      analogWrite(MAP_OUT, mapVal);
      Serial.print("current map: ");
      Serial.println(mapVal);
   }
}

void increaseRPM() {
   if(tacTimer>400)
      tacTimer*=(float)5/6;
   TAC_TIMER.setPeriod(tacTimer);
}

void decreaseRPM() {
   if(tacTimer<15000)
      tacTimer*=(float)7/6;
   TAC_TIMER.setPeriod(tacTimer);
}

void spark() {
   oldSparkTime = currSparkTime;
   currSparkTime = micros();
   printSpark++;
}

