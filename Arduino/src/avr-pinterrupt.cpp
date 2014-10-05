#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define intPin 2
#define ledPin 13

volatile int state;

void interruptlol(){
    digitalWrite(ledPin, state=!state);
}

int qmain()
{
  state = LOW;
  pinMode(intPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  attachInterrupt(0,interruptlol, CHANGE);
  interrupts();             // enable all interrupts
  Serial.begin(115200);
  
  while(1);
}

