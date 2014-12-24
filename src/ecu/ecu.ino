#include <DueTimer.h>
#include "table.h"

#define TRUE 1
#define FALSE 0

#define TAC_IN	3  // pin used for tachometer
#define MAP_IN   2  // pin used for manifold air pressure

#define FUEL_OUT 7  // pin used for fuel injection
#define SPARK_OUT 5  // pin used for spark

#define FUEL_TIMER Timer0
#define SPARK_TIMER Timer1

#define DWELLTIME 10 // spark coil dwell time in us
#define TDC 360      // crankshaft top dead center in degrees
#define GRACE 0      // degrees between finish fuel inject and discharge spark

#define CALIB_ANGLE 0            // angle of the first tooth after the missing one
#define ANGLE_PER_TOOTH 27.69f   // angle distance between teeth

#define ENGINE_DISPLACEMENT 49   // volume of the engine in cubic centimeters
#define AMBIENT_TEMP 298        // ambient temperature in kelvin
#define R_CONSTANT 287          // R_specific for dry air in J/kg/K
#define AIR_FUEL_RATIO 13       // air to fuel ratio for octane (MAY NEED REVISION)
#define MASS_FLOW_RATE  0.1f         // fuel injection flow rate in kg/s PUT SOMETHING HERE

volatile float instantDPMS;     //pseudoinstantaneous angular velocity in degrees per us

volatile char fuelOpen;       // whether the fuel injector is open
volatile char chargingSpark;  // whether the spark is charging

volatile char useFuel;	      // whether or not to use fuel (only fuel every other cycle)

volatile int fuelDuration;    // how long to fuel inject

volatile int lastTick;        // last tachometer interrupt
volatile int lastTickDelta;   // time difference (us) between last tac interrupt and the previous one
volatile int prevTick;        // previous tachometer interrupt
volatile int prevTickDelta;   // previous lastTickDelta

volatile int toothCount;      // which tooth we're at on the crankshaft
volatile float lastToothAngle;  // angle of the last tooth that passed by

volatile char recalc;         // flag to recalculate stuff after spark for next cycle


///////////////////////////////////////////////////////////////

float approxAngle;

float sparkAdvAngle;    // angle at which to discharge the spark
float sparkChargeAngle; // angle at which to begin charging the spark
int sparkChargeTime;    // in how long to begin charging the spark

float fuelStartAngle;   // angle at which to begin injecting fuel
int fuelStartTime;      // in how long to begin injecting fuel
float fuelEndAngle;     // angle at which to stop injecting fuel
int fuelDurationAngle;  // duration of time to inject fuel

float airVolume;        // volume of air that the engine will intake in m^3
float mapVal;              // manifold air pressure in kPa
int mapPulseHigh;

int sparkTimes = 0;

/////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);
  Serial.println("WHAAAAAAAAAAAAAAAAT");
  pinMode(TAC_IN, INPUT);
  pinMode(MAP_IN, INPUT);

  pinMode(FUEL_OUT, OUTPUT);
  pinMode(SPARK_OUT, OUTPUT);

  useFuel = TRUE;            // use fuel on the first cycle and every other cycle thereafter
  recalc = TRUE;

  attachInterrupt(TAC_IN, tacISR, RISING); // set up the tachometer ISR
  SPARK_TIMER.attachInterrupt(sparkISR);    // set up the spark ISR
  FUEL_TIMER.attachInterrupt(fuelISR);      // set up the fuel injection ISR

}

void loop() {
  
  if (recalc) {
      Serial.print("instant dpms");
      Serial.println(instantDPMS,5);
      Serial.print("times sparked");
      Serial.println(sparkTimes);
      Serial.print("spark adv");
      Serial.println(sparkAdvAngle);
      mapPulseHigh = pulseIn(MAP_IN, HIGH);    // virtual engine uses unfiltered PWM, so we don't use ADC
      mapVal = (float)mapPulseHigh / (mapPulseHigh + pulseIn(MAP_IN, LOW)) * 100; // read in manifold air pressure

      sparkAdvAngle = TDC - tableLookup(&SATable, instantDPMS * 166667, mapVal);  // calculate spark advance angle

      // calculate volume of air to be taken in in m^3
      airVolume = ENGINE_DISPLACEMENT * tableLookup(&VETable, instantDPMS * 166667, mapVal) * 1E-8f;

      // calculate how long to fuel inject
      fuelDuration = airVolume * mapVal * 1E3 / (R_CONSTANT * AMBIENT_TEMP * AIR_FUEL_RATIO * MASS_FLOW_RATE);

      fuelDurationAngle = fuelDuration * instantDPMS; // calculate the angular displacement during fuel injection
      fuelEndAngle = sparkAdvAngle - GRACE;           // calculate the angle at which to stop fuel injecting
      fuelStartAngle = fuelEndAngle - fuelDurationAngle; // calculate the angle at which to begin fuel injecting

      sparkChargeAngle = sparkAdvAngle - DWELLTIME * instantDPMS; // calculate angle at which to begin charging the spark

      // figure out in how long we need to begin charging spark (how long until sparkChargeAngle)
      approxAngle = lastToothAngle + (micros() - lastTick) * instantDPMS;
      sparkChargeTime = (sparkChargeAngle - approxAngle) / instantDPMS;

      // check if we haven't already started charging the spark (i.e. if the spark charge time is in the future)
      if (sparkChargeTime > 0)
        SPARK_TIMER.start(sparkChargeTime); // set timer to begin charging spark on time

      // check if we need to fuel on the current cycle
      useFuel = 0;
      if (useFuel) {
        // figure out in how long we need to begin fuel injecting
        approxAngle = lastToothAngle + (micros() - lastTick) * instantDPMS;
        fuelStartTime = (fuelStartAngle - approxAngle) / instantDPMS;

        // check if we haven't already begun fueling (i.e. if the fuel start time is in the future)
        if (fuelStartTime > 0)
          FUEL_TIMER.start(fuelStartTime); // set timer to begin injecting on time
        else
          recalc = FALSE; // we've already done all we can at this point, JESUS TAKE THE WHEEEL
      }

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
    FUEL_TIMER.start(fuelDuration);  // inject for fuel duration and then stop
  }
}

//spark advance
void sparkISR()
{
  SPARK_TIMER.stop();  // prevent timer from restarting

  if (chargingSpark)   // if charging, time to discharge!
  {
    // send signal to discharge
    digitalWrite(SPARK_OUT, LOW);
    chargingSpark = FALSE;  // no longer charging
    recalc = TRUE;          // time to begin recalculating for the new cycle
    useFuel = !useFuel;     // if we just fueled, not necessary to fuel on the next cycle
    sparkTimes++;
  }
  else                 // if not charging, time to charge
  {
    // send signal to begin charge
    digitalWrite(SPARK_OUT, HIGH);
    chargingSpark = TRUE;   // currently charging spark
    SPARK_TIMER.start(DWELLTIME); // discharge after DWELLTIME us
  }
}

// tachometer
void tacISR()
{
  prevTick = lastTick;    // keep track of the previous tachometer tick.
  lastTick = micros();    // record the current tachometer tick

  prevTickDelta = lastTickDelta;
  lastTickDelta = lastTick - prevTick; // calculate time between lastTick and prevTick

  // if the difference is 1.4 times more than the last one, we hit the missing tooth, time to calibrate
  if (lastTickDelta > prevTickDelta * 1.4f) {
    lastToothAngle = CALIB_ANGLE;
    toothCount = 0;
    instantDPMS = ANGLE_PER_TOOTH*2 / lastTickDelta;
  }
  // if not, behave normally
  else {
    ++toothCount;
    lastToothAngle += ANGLE_PER_TOOTH;
    // calculate the angular velocity using the time between the last 2 ticks
    instantDPMS = ANGLE_PER_TOOTH / lastTickDelta;
  }

  
}
