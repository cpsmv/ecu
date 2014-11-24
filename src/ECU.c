#include <DueTimer.h>
#include "table.h" 

#define TRUE 1
#define FALSE 0

#define TAC_PIN	0
#define FUEL_PIN	0
#define SPARK_PIN	0
#define MAP_PIN   0

#define FUEL_TIMER Timer0
#define SPARK_TIMER Timer1


#define DWELLTIME 10 //n microseconds
#define TDC 360 //top dead center in degrees
#define GRACE 5 // degrees between release of spark charge and closing fuel injector
#define ENGINE_DISPLACEMENT 49
#define CALIB_ANGLE 0 //blaze it
#define ANGLE_PER_TOOTH 420 // blaze it
#define AMBIENT_TEMP 298;
#define R_CONSTANT 287;
#define AIR_FUEL_RATIO 13;
#define MASS_FLOW_RATE // kg/s PUT SOMETHING HERE

volatile int instantDPMS;  //pseudoinstantaneous angular velocity in degrees per microsec

volatile char fuelOpen; //0 or 1
volatile char chargingSpark;

volatile int fuelDuration;


volatile int toothCount; // tooth count

volatile int lastTick; //last counted time
volatile int lastTickDelta;
volatile int prevTick;
volatile int prevTickDelta;
volatile int prevPrevTick;

volatile int lastToothAngle; //last known angle

volatile char recalc; // flag to recalculate stuff after spark for next cycle

//fuel injection
void fuelISR()
{
   FUEL_TIMER.stop();

   if(fuelOpen)
   {
      digitalWrite(FUEL_PIN, LOW);
      fuelOpen = FALSE;
   }
   else
   {
      digitalWrite(FUEL_PIN, HIGH);
      fuelOpen = TRUE;
      FUEL_TIMER.start(fuelDuration);
   }
}

//spark advance
void sparkISR()
{
   SPARK_TIMER.stop();

   if(chargingSpark)   // if charging, time to discharge!
   {
      // send signal to discharge 
      // ASK PINK WHAT SIGNALS ARE NECESSARY
      chargingSpark = FALSE;
      recalc = TRUE; // time to begin recalculating for the new cycle
   }
   else  // if not charging, time to charge
   {
      chargingSpark = TRUE;
      // send signal to begin charge
      SPARK_TIMER.start(DWELLTIME);
   }
}

void tacISR()
{
   prevPrevTick = prevTick;
   prevTick = lastTick;
   lastTick = micros();

   lastTickDelta = lastTick - prevTick;
   prevTickDelta = prevTick - prevPrevTick;

   if(lastTickDelta > prevTickDelta * 1.3f) {
      lastToothAngle = CALIB_ANGLE;
      toothCount = 0;
   }
   else {
      ++toothCount;
      lastToothAngle += ANGLE_PER_TOOTH;
   }

   instantDPMS = ANGLE_PER_TOOTH / lastTickDelta;
}


int main() {
   int sparkAdvAngle;
   int sparkChargeAngle;
   int sparkChargeTime;
   
   int fuelStartAngle;
   int fuelStartTime;
   int fuelEndAngle;
   int fuelDurationAngle;

   float airVolume;
   float map; // in kPa
   float volEff;

   attachInterrupt(TAC_PIN, tacISR, RISING);
   SPARK_TIMER.attachInterrupt(sparkISR);
   FUEL_TIMER.attachInterrupt(fuelISR);

   while(1) {

      map = analogRead(MAP_PIN) * some conversion factor motha fucka;

      if(recalc) {

         sparkAdvAngle = TDC - tableLookup(SATable, instantDPMS * 166667, map);
         volEff = tableLookup(VETable, instantDPMS * 166667, map);

         airVolume = ENGINE_DISPLACEMENT * volEff * 1E-8f; // IN M^3
         fuelDuration = airVolume * map * 1E3 / (R_CONSTANT * AMBIENT_TEMP * AIR_FUEL_RATIO * MASS_FLOW_RATE);

         fuelDurationAngle = fuelDuration * instantDPMS;
         fuelEndAngle = sparkAdvAngle - GRACE;
         fuelStartAngle = fuelEndAngle - fuelDurationAngle;

         sparkChargeAngle = sparkAdvAngle - DWELLTIME * instantDPMS;
         approxAngle = lastToothAngle + (micros() - lastTick) * instantDPMS;
         sparkChargeTime = (sparkChargeAngle - approxAngle) / instantDPMS;
         SPARK_TIMER.start(sparkChargeTime);

         approxAngle = lastToothAngle + (micros() - lastTick) * instantDPMS;
         fuelStartTime = (fuelStartAngle - approxAngle) / instantDPMS;
         FUEL_TIMER.start(fuelStartTime);

      }
   }
}
