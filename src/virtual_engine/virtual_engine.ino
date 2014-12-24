#include <TimerOne.h>

#define MAP_OUT 5
#define TAC_OUT 13

#define SPARK_IN 2

#define TIC_TAC_AT_1K_RPM 4616

#define MAX_TEETH 13

const unsigned int prescalers[] = {
  0, 1, 8, 64, 256, 1024};

int tooth;
unsigned int mapVal;

unsigned int currSparkTime;
unsigned int oldSparkTime;

unsigned int sparkChargeTime;

void setup() {
    Serial.begin(115200);
    pinMode(TAC_OUT, OUTPUT);
    pinMode(MAP_OUT, OUTPUT);
    mapVal = 128;
    tooth = 0;
    attachInterrupt(0, spark, FALLING);
    attachInterrupt(0, charge, RISING);
    configTimer(TIC_TAC_AT_1K_RPM);
    analogWrite(MAP_OUT, mapVal);
}


void loop() {
    static char input;
    if(Serial.available() > 0) {
        input = Serial.read();
        if(input == 'a')
            increaseMAP();
        if(input == 'z')
            decreaseMAP();
        else {
        	Serial.print("current map: ");
		    Serial.println(mapVal);
		    Serial.print("current spark based rpm: ");
		    Serial.println(6E7/(currSparkTime - oldSparkTime));
		    Serial.print("dwell time: ");
		    Serial.println(sparkDischargeTime-sparkChargeTime);
        }
    }
    
}

// send tac signals
ISR(TIMER1_COMPA_vect) {
    tooth = (tooth + 1)%MAX_TEETH;
    if (tooth != 0) {
        digitalWrite(TAC_OUT, HIGH);
        delayMicroseconds(1000);
        digitalWrite(TAC_OUT, LOW);
    }
}

void increaseMAP() {
    if(mapVal<246) {
        mapVal+=10;
        analogWrite(MAP_OUT, mapVal);
    }
}

void decreaseMAP() {
    if(mapVal>9) {
        mapVal-=10;
        analogWrite(MAP_OUT, mapVal);
    }
}

void charge() {
	sparkChargeTime = micros();
}

void spark() {
    oldSparkTime = currSparkTime;
    currSparkTime = micros();
}


void configTimer(long m) {
  noInterrupts();
  int i;
  TCCR1A = 0;
  TCCR1B = 0 | (1 << WGM12);
  TCNT1 = 0;
  TIMSK1 |= (1<<OCIE1A);
  for(i=1;i<6;i++) {
    if(m/prescalers[i]*16 < 65535) {
      TCCR1B |= i;  
      OCR1A = m/prescalers[i]*16;
      break;
    }
  }
  interrupts();
}