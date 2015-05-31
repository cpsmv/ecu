#include <DueTimer.h>

/*
 *	Name: ecu.c
 *	Revision: 1.0
 *	Date: 5/30/15
 *	Authors: Ivan Pachev and Alex Pink
 *
 *	Description:
 *	A finite state machine designed to control the Honda GX35 engine with 
 *  an EcoTrons interfacing kit. 
 */
// all includes are done in the Arduino IDE (because its a little beezy)
#include <stdio.h>
#include "tuning.h"
#include "table.h"

//***********************************************************
// Preprocessor Defines
//***********************************************************
// Pin Defs
#define KILLSWITCH_IN 	3
#define MAP_IN 			A0
#define ECT_IN 			A8
#define IAT_IN			A7
#define O2_IN 			A1
#define TPS_IN			A11
#define TACH_IN			21
#define SPARK_OUT 		7
#define FUEL_OUT 		8

#define FUEL_START_TIMER 		Timer0
#define FUEL_STOP_TIMER			Timer1
#define SPARK_CHARGE_TIMER 		Timer2
#define SPARK_DISCHARGE_TIMER 	Timer3

// Engine Parameters
#define DWELL_TIME 3000
#define CALIB_ANGLE 30
#define TDC 360
#define FUEL_END_ANGLE 120
#define ENGAGE_RPM 800
#define ENGINE_DISPLACEMENT 35.8f // TODO: comments with units!
#define R_CONSTANT 287.0f
#define AIR_FUEL_RATIO 14.7f
#define MASS_FLOW_RATE 0.0006f // TODO: calibrate Eco Trons injector

//***********************************************************
// Global Variables
//*********************************************************** 

// Booleans
bool killswitch;
bool fuelCycle;

// State Machine
typedef enum {
	NOT_ENGAGED,
	READ_SENSORS,
	CALC,
	SERIAL_OUT,
	ENGINE_RUNNING
} state;

state currState;

// Sensor Readings
float mapVal;
float ectVal;
float iatVal;
float tpsVal;
float o2Val;

// Real-Time Variables
float currentAngularSpeed;			// current speed in degrees per microsecond
unsigned int calibAngleTime;		// time when hall effect clicked around
unsigned int lastCalibAngleTime;	// previous time of 
float volEff;
float airVolume;
float fuelDuration;
float sparkChargeAngle;
float sparkDischargeAngle;

float fuelDurationAngle;
float fuelStartAngle;
float currEngineAngle;
float sparkAdvAngle;

char serialBuffer[100];


//***********************************************************
// Helper Macros
//*********************************************************** 
#define readMap() ( (75.757 * 3.3 * (float)analogRead(MAP_IN) / (float)1023 + 15.151) * 0.987167 ) //TODO: calibration

#define readTemp(SENSOR) ( (float)analogRead(SENSOR) ) //TODO: Calibration in Kelvin

#define readTPS() ( (float)analogRead(TPS_IN) / 1023.0f )

#define readO2() ( (float)analogRead(O2_IN) ) //TODO: calibration

#define calcSpeed(CURR_TIME, PREV_TIME, CURR_SPEED)	( 0.7*(360.0f / (CURR_TIME - PREV_TIME)) + 0.3*(CURR_SPEED) ) //TODO: weighting??

#define convertToRPM(ANGULAR_SPEED_US) ( ANGULAR_SPEED_US * 166667.0f )

#define convertFromRPM(RPM) ( RPM * 6E-6f )

float getCurrAngle(float angular_speed, unsigned int calib_time){
	float angle = angular_speed * (micros() - calib_time) + CALIB_ANGLE;
	return angle >= 360 ? angle - 360 : angle;
}

//***********************************************************
// Arduino Setup Function
//*********************************************************** 
void setup(void){

	pinMode(KILLSWITCH_IN, INPUT);
	pinMode(TACH_IN, INPUT);
	pinMode(SPARK_OUT, OUTPUT);
	pinMode(FUEL_OUT, OUTPUT);


	// Init Variables
	currentAngularSpeed = 0;			// current speed in degrees per microsecond
	calibAngleTime = 0;						// time when hall effect clicked around
	lastCalibAngleTime = 0;					// previous time of 

	killswitch = digitalRead(KILLSWITCH_IN);
	fuelCycle = false;

	// setup all interrupts
	attachInterrupt(KILLSWITCH_IN, killswitch_ISR, CHANGE);
	attachInterrupt(TACH_IN, tach_ISR, FALLING);
	SPARK_CHARGE_TIMER.attachInterrupt(chargeSpark_ISR);    	// set up the spark ISR
	SPARK_DISCHARGE_TIMER.attachInterrupt(dischargeSpark_ISR);
  	FUEL_START_TIMER.attachInterrupt(startFuel_ISR);      		// set up the fuel injection ISR
  	FUEL_STOP_TIMER.attachInterrupt(stopFuel_ISR);

	currState = NOT_ENGAGED;
}

//***********************************************************
// Arduino Loop Function
//*********************************************************** 
void loop(void){
	switch(currState){

		case NOT_ENGAGED:
			if(currentAngularSpeed > convertFromRPM(ENGAGE_RPM) && killswitch)
				currState = READ_SENSORS;
			break;

		case READ_SENSORS:
			currState = CALC;

			mapVal = readMap();
			iatVal = readTemp(IAT_IN);
			ectVal = readTemp(ECT_IN);
			tpsVal = readTPS();
			o2Val  = readO2();

			break;

		case CALC:
			currState = SERIAL_OUT;

			if(fuelCycle){
				// table lookup for volumetric efficiency 
      			volEff = tableLookup(&VETable, convertToRPM(currentAngularSpeed), mapVal);

      			// calculate volume of air to be taken in in m^3
      			airVolume =  volEff * ENGINE_DISPLACEMENT / 1E8;

      			// airvolume (m^3) * (map in kPa converted to Pa) / (J/(kg*K)) * (K) * (ratio) * (kg/s)
      			fuelDuration = airVolume * mapVal * 1013 / (R_CONSTANT * iatVal * AIR_FUEL_RATIO * MASS_FLOW_RATE) * 1E6;			
 				// TODO: check fuel duration calcuations

      			// find out at what angle to begin and end fueling
      			fuelDurationAngle = fuelDuration * currentAngularSpeed; // calculate the angular displacement during fuel injection
      			fuelStartAngle = FUEL_END_ANGLE - fuelDurationAngle; // calculate the angle at which to begin fuel injecting

      			currEngineAngle = getCurrAngle(currentAngularSpeed, calibAngleTime);

      			FUEL_START_TIMER.start( (fuelStartAngle - currEngineAngle) / currentAngularSpeed );
      		}

      		// find out at what angle to begin and end charging the spark
      		sparkDischargeAngle = TDC - tableLookup(&SATable, convertToRPM(currentAngularSpeed), mapVal);  // calculate spark advance angle
      		sparkChargeAngle = sparkAdvAngle - DWELL_TIME * currentAngularSpeed; // calculate angle at which to begin charging the spark

      		currEngineAngle = getCurrAngle(currentAngularSpeed, calibAngleTime);

      		SPARK_CHARGE_TIMER.start( (sparkChargeAngle - currEngineAngle) / currentAngularSpeed );

			break;

		case SERIAL_OUT:
			currState = ENGINE_RUNNING;
			// communicate serial inf

			sprintf(serialBuffer, "%5f    %3f         %3f      %4f           %5d", 
				convertToRPM(currentAngularSpeed), mapVal, volEff, sparkDischargeAngle, fuelDuration);

			SerialUSB.println("RPM:   MAP(atm):   VE(%):   SPARK(deg):   FUEL PULSE(us):");
			SerialUSB.println(serialBuffer);

			break;

		case ENGINE_RUNNING:
			if( convertToRPM(currentAngularSpeed) < ENGAGE_RPM ){
				currState = NOT_ENGAGED;
			}
			break;
	}
}

//***********************************************************
// Interrupt Service Routines
//*********************************************************** 
void killswitch_ISR(void){
	killswitch = digitalRead(KILLSWITCH_IN);
	if(!killswitch){
		currState = NOT_ENGAGED;
	}
}

void tach_ISR(void){
	lastCalibAngleTime = calibAngleTime;
	calibAngleTime = micros();
	currentAngularSpeed = calcSpeed(calibAngleTime, lastCalibAngleTime, currentAngularSpeed);

	fuelCycle = !fuelCycle;
	if( currState != NOT_ENGAGED ){
		currState = READ_SENSORS;
	}
}

void chargeSpark_ISR(void){
	SPARK_CHARGE_TIMER.stop();
	// start chargine coil
	digitalWrite(SPARK_OUT, HIGH);
	// set discharge timer for dwell time
	SPARK_DISCHARGE_TIMER.start(DWELL_TIME);
}

void dischargeSpark_ISR(void){
	// discharge coil
	SPARK_DISCHARGE_TIMER.stop();
	// start chargine coil
	digitalWrite(SPARK_OUT, LOW);
}

void startFuel_ISR(void){
	FUEL_START_TIMER.stop();
	// start chargine coil
	digitalWrite(FUEL_OUT, HIGH);
	// set discharge timer for dwell time
	FUEL_STOP_TIMER.start(fuelDuration);
}

void stopFuel_ISR(void){
	// stop fueling
	FUEL_STOP_TIMER.stop();
	// start chargine coil
	digitalWrite(FUEL_OUT, LOW);
}
