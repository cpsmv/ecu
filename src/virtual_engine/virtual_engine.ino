#include <TimerOne.h>
/*
The virtual Honda engine
 (find out how to) find out amount of fuel injected when injector is opening and closing
 */
#define MAP_OUT 5
#define TAC_OUT 13

#define SPARK_IN 2
#define FUEL_IN 3

#define TAC_TIMER Timer1


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

   attachInterrupt(0, spark, FALLING);

   tacTimer = 60000; // starter motor simulation
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

   if (printSpark) {
      Serial.print("current spark based rpm: ");
      Serial.println(6E7 / (currSparkTime - oldSparkTime));
      Serial.print("current tac timer: ");
      Serial.println(tacTimer);
      printSpark = 0;
   }
}

// send tac signals
void tacSignal() {
   digitalWrite(TAC_OUT, HIGH);
   delayMicroseconds(200);
   digitalWrite(TAC_OUT, LOW);
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
   if(tacTimer>5000)
      tacTimer*=(float)5/6;
   TAC_TIMER.setPeriod(tacTimer);
}

void decreaseRPM() {
   if(tacTimer<70000)
      tacTimer*=(float)7/6;
   TAC_TIMER.setPeriod(tacTimer);
}

void spark() {
   oldSparkTime = currSparkTime;
   currSparkTime = micros();
   printSpark = 1;
}

