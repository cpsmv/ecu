#include <avr/io.h>
#include <avr/interrupt.h>
#include "table.h" 

//input
#define MAP PD5 //manifold air pressure
#define RPM PD6  //RPM
//output
#define FUEL PD1 
#define SPARK PD2

#define DWELLTIME 10 //n microseconds
#define FUELTIME 10 //n amount of fuel per unit time
#define ANGLEDISTANCE 10 //n angular distance between pins
#define SPARKOFFSET 10//allowable diffrence for spark offset (in time miliseconds)
#define TOOTHOFFSET 10//allowable deviation between pin dectetion (in time milliseconds)
#define TOOTHNUM 10 //number of teeth

volatile char doFuel; //0, 1, or 2
volatile float fuelAmmount; //from ve table

volatile char doSpark; //0, 1, or 2
volatile float desiredAngle; //from s

volatile float curAngle; //current angle
volatile float avgTime;

volatile int lastTime; //last counted time

volatile int toothCount; // touth count

//fuel injection
ISR(TIMER0_COMPA_vect)
{
   if(doFuel == 0)
   {
      //open injector pin write
      doFuel = 1;
      //timercounter += fuel ammount*62.5 // time in millseconds * timer frequency
   }
   else if(doFuel == 1)
   {
      //close injector pin write
      doFuel = 0;
   }
}

//spark advance
ISR(TIMER2_COMPA_vect)
{
   if(doSpark == 0)
   {
      //((desiredAngle - curAngle)/ANGLEDISTANCE)* avgTime is the time until the desired angle is reached
      if(DWELLTIME - ((desiredAngle - curAngle)/ANGLEDISTANCE)* avgTime <= SPARKOFFSET)
      {
         //wirte to spark pin start charing spark
         doSpark = 1;
         //timercounter +=  DWELLTIME *62.5;
      }
      else
      {
         //timercounter += (desiredAngle - curAngle)/ANGLEDISTANCE)* avgTime + SPARKOFFSET - DWELLTIME * 62.5
      }
   }
   else if(doSpark == 1)
   {
      //write to spark pin stop charging spark //spark will realese
      doSpark = 0;

   }
}

ISR(PCINT_0)// pin interrupt
{
   //lastTime = timercounter * (timer frequency)
   OCR1A = 0; //timer counter = 0
   toothCount++;
   angle = toothCount * ANGLEDISTANCE +(ANGLEDISTANCE * 2);
   //stop timer
   //read counter
   //start timer
   //counter => timer
}

int main(void)
{
   //initialize variables
   avgTime = 0;
   curAngle = 0;
   doFuel = 0;
   doSpark = 0;
   toothCount = 0;
   lastTime = 0;
   float rpmValue = 0; //read rpm
   float mapValue = 0; //read map 
   float airVolume = 0;
   for(;;)
   {
      rpmValue = 0; //read rpm
      mapValue = 0; //read map 
      avgTime = (lastTime + avgTime) / 2;
      if(lastTime > ( avgTime + TOOTHOFFSET))
      {
         toothCount = 0;
      }
      //angle distance * 2 is the distance between the two teeth with the missing tooth inbetween
      //angle = toothCount * ANGLEDISTANCE +(ANGLEDISTANCE * 2); 
      if(doFuel == 0)
      {
         airVolume = tableLookup(VETable, rpmValue, mapValue);
         fuelAmmount = 0; //equation for fuel volume by mass of air / 14.7
         //fuel ammont will be value in milliseconds
         //timer counter 1 = 0; //trigger interupt 
      }
      if(doSpark == 0)
      {
         desiredAngle = tableLookup(SATable, rpmValue, mapValue);
         //timer cointer 2 = 0; //trigger interupt
      }
   }

}
