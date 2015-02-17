#include <DueTimer.h>

#include <DueTimer.h>
#include "table.h"
#include "tuning.h"

#define TRUE 1
#define FALSE 0

#define INTERRUPT_LATENCY_US 127

#define TAC_IN  2  // pin used for tachometer
#define MAP_IN  A3  // pin used for manifold air pressure

#define FUEL_OUT 4  // pin used for fuel injection
#define SPARK_OUT 6  // pin used for spark

#define FUEL_TIMER Timer0
#define SPARK_TIMER Timer1

#define DWELLTIME 3000 // spark coil dwell time in us

#define ACTIVE_RPM 300     // don't do anything below this rpm

#define NUM_TEETH 11
#define CALIB_ANGLE 180.0f            // angle of the first tooth after the missing one
#define ANGLE_PER_TOOTH 30.0f   // angle distance between teeth
#define TDC 360.0f      // crankshaft top dead center in degrees

#define DEGREES_PER_CYCLE 360.0f

#define ENGINE_DISPLACEMENT 49.0f  // volume of the engine in cubic centimeters
#define AMBIENT_TEMP 298.0f        // ambient temperature in kelvin
#define R_CONSTANT 287.0f          // R_specific for dry air in J/kg/K
#define AIR_FUEL_RATIO 14.7f      // air to fuel ratio for octane (MAY NEED REVISION)
#define MASS_FLOW_RATE  0.0006f         // fuel injection flow rate in kg/s

#define CALIBRATION_FACTOR 2.3f

volatile float engineSpeedDPMS;
volatile float instantDPMS;
volatile int teethPassed = 0;

volatile char fuelOpen;       // whether the fuel injector is open
volatile char chargingSpark;  // whether the spark is charging

volatile char useFuel;        // whether or not to use fuel (only fuel every other cycle)

volatile int fuelDuration;    // how long to fuel inject

volatile int lastTick;        // last tachometer interrupt
volatile int lastTickDelta;   // time difference (us) between last tac interrupt and the previous one
volatile int prevTick;        // tachometer interrupt before the last one
volatile int prevTickDelta;   // previous lastTickDelta

volatile int lastRevEnd;        // time when the last cycle ended
volatile int lastRevDuration;   // duration of the last revolution
volatile int prevRevEnd;        // time when the previous cycle ended
volatile int prevRevDuration;   // duration of the previous cycle

volatile float lastToothAngle;  // angle of the last tooth that passed by
volatile float nextToothAngle;
volatile float approxAngle;   // the approximate engine position is calculated here

volatile char recalc;         // flag to recalculate stuff after spark for next cycle


///////////////////////////////////////////////////////////////



float sparkAdvAngle;    // angle at which to discharge the spark
float sparkChargeAngle; // angle at which to begin charging the spark
int sparkChargeTime;    // in how long to begin charging the spark

float fuelStartAngle;   // angle at which to begin injecting fuel
int fuelStartTime;      // in how long to begin injecting fuel
float fuelEndAngle;     // angle at which to stop injecting fuel
float fuelDurationAngle;  // duration of time to inject fuel

float airVolume;        // volume of air that the engine will intake in m^3
float mapVal;              // manifold air pressure in kPa

//////////////////////////////////////////////////////////////

int printStuff;      // use this to print things every n cycles

void setup() {

   Serial.begin(115200);

   pinMode(TAC_IN, INPUT);
   pinMode(MAP_IN, INPUT);

   pinMode(FUEL_OUT, OUTPUT);
   pinMode(SPARK_OUT, OUTPUT);

   digitalWrite(FUEL_OUT, LOW);
   digitalWrite(SPARK_OUT, LOW);

   engineSpeedDPMS = 0;

   prevRevDuration = 0;
   lastRevDuration = 0;

   chargingSpark = FALSE;
   fuelOpen = FALSE;

   printStuff = 0;

   useFuel = FALSE;            // use fuel on the first cycle and every other cycle thereafter
   recalc = FALSE;

   attachInterrupt(TAC_IN, tacISR, RISING); // set up the tachometer ISR
   SPARK_TIMER.attachInterrupt(sparkISR);    // set up the spark ISR
   FUEL_TIMER.attachInterrupt(fuelISR);      // set up the fuel injection ISR
}

int messedUp = 0;
int timesCalibrated = 0;
float realSparkAngle;
char sparkConsumed = TRUE;
char fuelConsumed = TRUE;
float lastMessedUpAngle;
int lastMessedUpToothCount;

void loop() {
   // only recalculate stuff if it is necessary and if the engine is still running
   if (recalc && engineSpeedDPMS * 166667 > ACTIVE_RPM) {
      // use this to print every n cycles
      printStuff++;

      // read in manifold air pressure (map calibration)
      // NOTE: the last number 0.987167 is 1/1.013 !!!! because division is slow
      mapVal = (75.757 * 3.3 * (float)analogRead(MAP_IN) / (float)1023 + 15.151) * 0.987167;

      //mapPulseHigh = pulseIn(MAP_IN, HIGH);    // virtual engine uses unfiltered PWM, so we don't use ADC
      //mapVal = 100 * (float)mapPulseHigh / (float)(mapPulseHigh + pulseIn(MAP_IN, LOW)); // read in manifold air pressure

      // our map calibration might not be good so we should not exceed 100% of 101.3 kPa
      if(mapVal >= 100) 
         mapVal = 99.9f;

      /////////////////////////////////////////////////////////
      //     FUEL PULSE DURATION CALCULATION
      ////////////////////////////////////////////////////////// 
   
      // calculate volume of air to be taken in in m^3
      airVolume =  tableLookup(&VETable, engineSpeedDPMS * 166667, mapVal) * ENGINE_DISPLACEMENT / 1E8;

      // airvolume (m^3) * (map in kPa converted to Pa) / (J/(kg*K)) * (K) * (ratio) * (kg/s)
      fuelDuration = airVolume * mapVal * 1013 / (R_CONSTANT * AMBIENT_TEMP * AIR_FUEL_RATIO * MASS_FLOW_RATE) * 1E6;

      ///////////////////////////////////////////////////////// 

      // find out at what angle to begin and end fueling
      fuelEndAngle = TDC - 60;   // finish fueling 60 degrees before TDC
      fuelDurationAngle = fuelDuration * engineSpeedDPMS; // calculate the angular displacement during fuel injection
      fuelStartAngle = fuelEndAngle - fuelDurationAngle; // calculate the angle at which to begin fuel injecting

      // find out at what angle to begin and end charging the spark
      sparkAdvAngle = TDC - tableLookup(&SATable, engineSpeedDPMS * 166667, mapVal);  // calculate spark advance angle
      sparkChargeAngle = sparkAdvAngle - DWELLTIME * engineSpeedDPMS; // calculate angle at which to begin charging the spark

      fuelConsumed = FALSE;
      sparkConsumed = FALSE;

      recalc = FALSE;
   }

   else if (printStuff == 20)
   {
      printStuff = 0;
      Serial.println("map(%atm)   spark(deg)     fuel pulse(us)          rpm");
      Serial.print(mapVal, 6);
      Serial.print("       ");
      Serial.print(sparkAdvAngle, 3);
      Serial.print("            ");
      Serial.print(fuelDuration);
      Serial.print("            ");
      Serial.println(engineSpeedDPMS * 166667);
      Serial.print("messed up times:");
      Serial.print(messedUp);
      Serial.print("    times calibrated: ");
      Serial.print(timesCalibrated);
      Serial.print("    real spark angle: ");
      Serial.println(realSparkAngle);
   }
}

//fuel injection
void fuelISR()
{
   FUEL_TIMER.stop();   // prevent timer from restarting

   if (fuelOpen)  // if currently injecting fuel
   {
      digitalWrite(FUEL_OUT, LOW);  // close fuel injector
      fuelOpen = FALSE;             // no longer injecting fuel
   }
   else           // if currently not injecting fuel
   {
      digitalWrite(FUEL_OUT, HIGH); // open fuel injector
      fuelOpen = TRUE;              // currently injecting fuel
      if (fuelDuration > INTERRUPT_LATENCY_US)
         FUEL_TIMER.start(fuelDuration - INTERRUPT_LATENCY_US); // inject for fuel duration and then stop
      else
         FUEL_TIMER.start(1);
   }
}

//spark advance
void sparkISR()
{
   SPARK_TIMER.stop();  // prevent timer from restarting

   if (chargingSpark)   // if charging, time to discharge!
   {
      // send signal to discharge
      realSparkAngle = lastToothAngle + (micros() - lastTick) * instantDPMS;
      digitalWrite(SPARK_OUT, LOW);
      chargingSpark = FALSE;  // no longer charging
   }
   else  // if not charging, time to charge
   {
      // send signal to begin charge
      digitalWrite(SPARK_OUT, HIGH);
      chargingSpark = TRUE;   // currently charging spark
      SPARK_TIMER.start(DWELLTIME - INTERRUPT_LATENCY_US); // discharge after DWELLTIME us
   }
}

int prevPrevRevDuration;

// tachometer
void tacISR()
{
   prevTick = lastTick;    // keep track of the previous tachometer tick.
   lastTick = micros();    // record the current tachometer tick

   prevTickDelta = lastTickDelta;
   lastTickDelta = lastTick - prevTick; // calculate time between lastTick and prevTick

   instantDPMS = ANGLE_PER_TOOTH / lastTickDelta;

   if(lastTickDelta > prevTickDelta * CALIBRATION_FACTOR)
   {
      timesCalibrated++;
      if(lastToothAngle != 120.0f) 
      {
         messedUp++; // count how many times we calibrated inaccurately
         lastMessedUpAngle = lastToothAngle;
         lastMessedUpToothCount = teethPassed;
      }
   }

   ++teethPassed;

   if(teethPassed >= NUM_TEETH)
   {
      teethPassed = 0;
   }
      

   // if the difference between the ticks is greater than CALIBRATION_FACTOR times, we reached the calibration position.
   if (lastTickDelta > prevTickDelta * CALIBRATION_FACTOR || teethPassed == 0) {
      teethPassed = 0;
      prevRevEnd = lastRevEnd;
      lastRevEnd = lastTick;
      prevPrevRevDuration = prevRevDuration;
      prevRevDuration = lastRevDuration;
      lastRevDuration = lastRevEnd - prevRevEnd;
      lastToothAngle = CALIB_ANGLE;
      engineSpeedDPMS = DEGREES_PER_CYCLE * 3.0f / (prevPrevRevDuration + prevRevDuration + lastRevDuration);
      instantDPMS *= 2.0f;
   }
   else
   {
      lastToothAngle += ANGLE_PER_TOOTH;
      // as soon as we pass top dead center, start a new cycle
      if(lastToothAngle >= TDC)
      {
         recalc = TRUE;          // new cycle means we should recalculate!
         lastToothAngle -= DEGREES_PER_CYCLE;  // since we passed TDC, normalize lastToothAngle for the next cycle
         useFuel = !useFuel;     // if we just fueled, not necessary to fuel on the next cycle
      }      
   }
   
   nextToothAngle = lastToothAngle + ANGLE_PER_TOOTH;
   // if we're not charging spark and spark charge angle is going to be between the next two tac ticks
   if(!sparkConsumed && !chargingSpark && sparkChargeAngle < nextToothAngle + ANGLE_PER_TOOTH && nextToothAngle <= sparkChargeAngle)
   {
      approxAngle = lastToothAngle + (micros() - lastTick) * instantDPMS;
      sparkChargeTime = (sparkChargeAngle - approxAngle) / instantDPMS;
      SPARK_TIMER.start(sparkChargeTime - INTERRUPT_LATENCY_US); // set timer to begin charging spark on time
      sparkConsumed = TRUE;
   }

   // if we're not fueling and we need to fuel and the fuel start angle is between the next two tac ticks
   if (!fuelConsumed && !fuelOpen && useFuel && fuelStartAngle < nextToothAngle + ANGLE_PER_TOOTH && nextToothAngle <= fuelStartAngle) 
   {
      approxAngle = lastToothAngle + (micros() - lastTick) * instantDPMS;
      fuelStartTime = (fuelStartAngle - approxAngle) / instantDPMS;
      FUEL_TIMER.start(fuelStartTime - INTERRUPT_LATENCY_US); // set timer to begin injecting on time
      fuelConsumed = TRUE;
   }

}
