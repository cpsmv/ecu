#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define ledPin 13

const unsigned int prescalers[] = {
  0, 1, 8, 64, 256, 1024};
  
unsigned int q,i;

void configTimer(long m) {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0 | (1 << WGM12);
  TCNT1 = 0;
  TIMSK1 |= (1<<OCIE1A);
  for(i=1;i<6;i++) {
    if(m/prescalers[i]*16 < 65535) {
      TCCR1B |= i;	
      q = OCR1A = m/prescalers[i]*16;
      break;
    }
  }
  interrupts();
}

int main() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("SUP");
  // initialize timer1
  configTimer(1000000); // configure timer for 1 s
  
  Serial.print("regster set to ");
  Serial.println(q);
  Serial.print("prescaler set to ");
  Serial.println(prescalers[i]);

  while(1);
}

ISR(TIMER1_COMPA_vect) {
 digitalWrite(ledPin, digitalRead(ledPin) ^ 1); 
}


