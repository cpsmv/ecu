#include <avr/io.h>
#include <avr/interrupt.h>
#define ledPin 13

void setup()
{
  pinMode(ledPin, OUTPUT);
  
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;  // no config settings in this register
  TCCR1B = 0;  // init to zero but waiting for config settings
  TCNT1  = 0;  // init timer 1 counter to zero

  OCR1A = 65535;            // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS02) | (1 << CS00);    // 1024 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
  Serial.begin(115200);
}

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
  digitalWrite(ledPin, !digitalRead(ledPin));
  if(OCR1A > 1000) OCR1A /=2 ;   // toggle LED pin
}

void loop()
{
  Serial.println("penis\n");// your program here...
}

