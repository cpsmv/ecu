#include <TimerOne.h>

#define MAP_OUT 3
#define TAC_OUT 4

#define SPARK_IN 2

#define TIC_TAC_AT_1K_RPM 4616

#define MAX_TEETH 13

int tooth;
int mapVal;

unsigned int currSparkTime;
unsigned int oldSparkTime;
unsigned int sparkTimeDelta;

const unsigned int prescalers[] = {
  0, 1, 8, 64, 256, 1024};

void setup() {
	Serial.begin(115200);
	mapVal = 128;
	tooth = 0;
	Timer1.attachInterrupt(tacISR,TIC_TAC_AT_1K_RPM);
	attachInterrupt(0, spark, FALLING);
}


void loop() {
	static char input;
	if(Serial.available() > 0) {
		input = Serial.read();
		if(input == 'a')
			increaseMAP();
		if(input == 'z')
			decreaseMAP();
	}
	Serial.print("current map: ");
	Serial.println(mapVal);
	Serial.print("current spark based rpm: ");
	Serial.println(6E7/(currSparkTime - oldSparkTime));
}

// send tac signals
void tacISR() {
	tooth = (tooth + 1)%MAX_TEETH;
	if (tooth != 0) {
		digitalWrite(TAC_OUT, HIGH);
		delayMicroseconds(50);
		digitalWrite(TAC_OUT, LOW);
	}
}

void increaseMAP() {
	if(mapVal<255) {
		++mapVal;
		analogWrite(MAP_OUT, mapVal);
	}
}

void decreaseMAP() {
	if(mapVal>1) {
		--mapVal;
		analogWrite(MAP_OUT, mapVal);
	}
}

void spark() {
	oldSparkTime = currSparkTime;
	currSparkTime = micros();
}
