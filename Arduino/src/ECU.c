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
#define SPARKOFFSET 10//allowable difference for spark offset (in time milliseconds)
#define TOOTHOFFSET 10//allowable deviation between pin detection (in time milliseconds)
#define TOOTHNUM 10 //number of teeth
#define TDC 360 //top dead center in degrees
#define GRACE 10 // degrees between release of spark charge and closing fuel injector
#define CONFIGTIMEROFFSET 15 //degrees to configure the timers  within

volatile char fuelOpen; //0 or 1
volatile float fuelTime; //calculated by main (when to fuel)
volatile int fuelDurration; //amount of fuel (translates to time to keep injector open)

//volatile float sparkAngle; //angle to release the spark at
volatile char charging;

volatile float curAngle; //current angle
volatile float avgTime;

volatile int lastTime; //last counted time

volatile int toothCount; // tooth count
volatile float approxAngle;

volatile char valSet; //set to false after the spark has been fired so that new values are calculated
//fuel injection
ISR(TIMER0_COMPA_vect)
{
   if(fuelOpen)
   {
      //cloes fuel injector
      fuelOpen = 0;
   }
   else
   {
      //open fuel injector
      fuelOpen = 1;
      //run timer for fuelDurration 
   }
}

//spark advance
ISR(TIMER2_COMPA_vect)
{
   if(!charging)
   {
      charging = 1;
      //run timer for dwell time
   }
   else
   {
      //discharge
      charging = 0;
      valSet = 0;
   }
}

void tacISR()
{
   lastTime = //timer value * conversion factor to microseconds
   int diffrence = lastTime - (2*avgTime);
   if(diffrence < 0)
      diffrence *= -1;
   if(diffrence >= TOOTHOFFSET)
   {
      toothCount = 0;
      curAngle = 0;
   }
   else
   {
      toothCount++;
      curAngle = toothCount * ANGLEDISTANCE;
      avgTime = (lastTime + avgTime) / 2
   }
   approxAngle = curAngle;
   //run timer for a while
}

int main(void)
{
   float sparkAngle; //angle to release the spark at
   float fuelAngle; //angle to stop fueling at
   int airVolume;
   int rpm;
   int map;
   float setTimer; //angle to set timers at
   float fuelStart;
   float sparkStart;
   char timerSet = 0;
   //initialize timers
   noInterrupts();
   attachInterrupt(0,tacISR, CHANGE);
   interrupts();
   //configure timer 0 timer 2
   curAngle = approxAngle = 0;
   valSet = 0;
   for(;;)
   {
      //read RPM MAP
      approxAngle = curAngle + //timer value converted to microseconds * RPM converted to rotations per microsecond
      
      if(!valSet)
      {
	 sparkAngle = TDC - tableLookup(SATable,rpm, map );
	 fuelAngle = sparkAngle - GRACE;
	 airVolume = tableLookup(VETable, rpmValue, mapValue);
	 fuelAmmount = //equation for fuel amount from air volume, convert to time (microseconds)

	 fuelStart = fuelAngle - (fuelAmmount * //RPM converted to rotations per microsecond)
	 sparkStart = sparkAngle - (DWELLTIME * //RPM converted to rotations per microsecond;
	 timerSet = 0;
	 valSet = 1;
      }

      if(!timerSet)
      {

	 if(charging && fuelOpen)
	 {
	    timerSet = 1;
	 }
	 if(fuelStart - approxAngle <= CONFIGTIMEROFFSET && !fuelOpen)
	 {
	    //start fuel timer
	    fuelOpen = 1;
	 }
	 if(sparkStart - approxAngle <= CONFIGTIMEROFFSET && !charging)
	 {
	    //start spark timer
	    charging = 1;
	 }
      }     
   }
}
