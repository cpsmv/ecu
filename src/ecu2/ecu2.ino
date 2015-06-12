// these are above the description because Arduino IDE puts them here!
#include <DueTimer.h>
#include <stdio.h>
#include "table.h"
#include "tuning.h"

/*
 *  Name: ecu2.c
 *  Revision: 1.0.1
 *  Date: 6/11/15
 *  Authors: Ivan Pachev and Alex Pink
 *
 *  Description:
 *  A finite state machine designed to control the Honda GX35 engine with 
 *  an EcoTrons interfacing kit. For use with the GX35 EFI Shield and 
 *  Arduino Due.
 *
 *  Note to the programmer:
 *  Please use Arduino IDE to compile / upload sketches! To use the built-in
 *  Arduino libraries, the IDE must be used. The Makefile and code directory
 *  structure is too convoluted to use.
 *
 *  TODO LIST:
 *  AFR table & lookup eqn
 *  calibrate Eco Trons injector & MASS_FLOW_RATE
 *
 */

//***********************************************************
// Preprocessor Defines
//***********************************************************
// Pin Definitions 
#define KILLSWITCH_IN 	3
#define MAP_IN 			A0
#define ECT_IN 			A8
#define IAT_IN			A7
#define O2_IN 			A1
#define TPS_IN			A11
#define TACH_IN			21
#define SPARK_OUT 		7
#define FUEL_OUT 		8

// Timers
#define FUEL_START_TIMER        Timer0
#define FUEL_STOP_TIMER	        Timer1
#define SPARK_CHARGE_TIMER      Timer2
#define SPARK_DISCHARGE_TIMER   Timer3

// Engine Parameters
#define DWELL_TIME 3000                 // time required to inductively charge the spark coil [us]
#define CALIB_ANGLE 30                  // tach sensor position from Top Dead Center, in direction of crankshaft rotation [degrees]
#define TDC 360                         // Top Dead Center is 360 degrees [degrees]
#define FUEL_END_ANGLE 120              // when to stop fueling; set for during the intake stroke [degrees]
#define ENGAGE_RPM 800                  // when to start calculating fuel 
#define ENGINE_DISPLACEMENT 35.8E-6     // GX35 displacement is 35.8cc, convert to m^3 [1cc = 1E-6 m^3]
#define MOLAR_MASS_AIR 28.97f           // molar mass of dry air [g/mol]
#define R_CONSTANT 8.314f               // ideal gas constant [J/(mol*K)]
#define AIR_FUEL_RATIO 14.7f            // stoichiometric AFR for gasoline [kg/kg] 
#define MASS_FLOW_RATE 0.0006f          // fueling rate of the injector [kg/s]
#define VOLTS_PER_ADC_BIT 3.23E-3       // for converting an ADC read to voltage 

//***********************************************************
// Global Variables
//*********************************************************** 
// Booleans
bool killswitch;
bool fuelCycle;
bool sensorDebugMode = 0;

// Serial Buffer for USB transmission
char serialBuffer[100];

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
float MAPval;	// Manifold Absolute Pressure reading, w/ calibration curve [kPa]
float ECTval;   // Engine Coolant Temperature reading, w/ calibration curve [K]
float IATval;   // Intake Air Temperature reading, w/ calibration curve [K]
float TPSval;   // Throttle Position Sensor Reading [%]
float O2Val;    // Oxygen Sensor Reading, w/ calibration curve [AFR]

// Real Time Stuff
float currAngularSpeed;             // current speed [degrees per microsecond]
unsigned int calibAngleTime;        // time of position calibration [microseconds]
unsigned int lastCalibAngleTime;    // last posiition calibration time, for RPM calcs [microseconds]
float volEff;                       // volumetric efficiency [% out of 100]
float airVolume;                    // volume of inducted air [m^3]
float airMolar;                     // moles of inducted air [mols]
float fuelMass;                     // mass of fuel to be injected [g]
float fuelDuration;                 // length of injection pulse [us]
float fuelDurationDegrees;          // length of injection pulse [degrees]
float sparkChargeAngle;             // when to start inductively charging the spark coil [degrees]
float sparkDischargeAngle;          // when to discharge the coil, chargeAngle + DWELL_TIME [degrees]
float fuelStartAngle;               // when to start the injection pulse [degrees]
float currEngineAngle;              // current engine position [degrees]

//***********************************************************
// Functions & Macros
//*********************************************************** 
float readECT(){
    int rawTemp;
    // temperature sensor is nonlinear, so divided into 2 curves with 3 points
    // temp is below 40 C
    if( (rawTemp = analogRead(ECT_IN)) > 499 )
        //      [ADC]  * [V/ADC]           * [C/V]   + ( [83.14 C] + [273 C -> K])
        return rawTemp * VOLTS_PER_ADC_BIT * -26.79f + 356.14f;
    // temp is above 40 C
    else
        //      [ADC]  * [V/ADC]           * [C/V]   + ( [136.63 C] + [273 C -> K])
        return rawTemp * VOLTS_PER_ADC_BIT * -60.02f + 409.63f;
}

float readIAT(){
    int rawTemp;
    // temperature sensor is nonlinear, so divided into 2 linear curves w/ 3 data points
    // temp is below 40 C
    if( (rawTemp = analogRead(IAT_IN)) > 499 )
        //      [ADC]  * [V/ADC]           * [C/V]   + ( [83.14 C] + [273 C -> K])
        return rawTemp * VOLTS_PER_ADC_BIT * -26.79f + 356.14f;
    // temp is above 40 C
    else
        //      [ADC]  * [V/ADC]           * [C/V]   + ( [136.63 C] + [273 C -> K])
        return rawTemp * VOLTS_PER_ADC_BIT * -60.02f + 409.63f;
}

//       [kPa]   =              [ADC]         * [V/ADC]           * [kPa/V] + [kPa]
#define readMap() ( (float)analogRead(MAP_IN) * VOLTS_PER_ADC_BIT * 28.58   + 10.57 )

//        [%]    =              [ADC]         / [max ADC]
#define readTPS() ( (float)analogRead(TPS_IN) / 1023.0f )

#define readO2() ( (float)analogRead(O2_IN) ) 				//TODO: calibration

#define calcSpeed(CURR_TIME, PREV_TIME, CURR_ANG_SPEED)	( 0.7*(360.0f / (CURR_TIME - PREV_TIME)) + 0.3*(CURR_ANG_SPEED) ) 	//TODO: weighting??

//         [rev/min]                  =    [degrees/us]   * [ (1 rev/360 deg)*(60E6 us/1 min)]
#define convertToRPM(ANGULAR_SPEED_US) ( ANGULAR_SPEED_US * 166667.0f )

//         [deg/us]        =  [rev/min] * [(360 deg/1 rev)*(1 min/60E6 us)]
#define convertFromRPM(RPM) ( RPM       * 6E-6f )

float getCurrAngle(float angular_speed, unsigned int calib_time){
    //[degrees] = [degrees/us]  * (  [us]   -     [us]  ) +  [degrees]
	float angle = angular_speed * (micros() - calib_time) + CALIB_ANGLE;
    // always keep the engine angle less than 360 degrees
	return angle >= 360 ? angle - 360 : angle;
}

//***********************************************************
// Arduino Setup Function
//*********************************************************** 
void setup(void){

    // Set 12-bit ADC Readings
    //analogReadResolution(12);

    // Pin directions
	pinMode(KILLSWITCH_IN, INPUT);
	pinMode(TACH_IN, INPUT);
	pinMode(SPARK_OUT, OUTPUT);
	pinMode(FUEL_OUT, OUTPUT);

	// Initialize Variables
	currAngularSpeed = 0;
	calibAngleTime = 0;				
	lastCalibAngleTime = 0;			

	// setup all interrupts and timers
	attachInterrupt(KILLSWITCH_IN, killswitch_ISR, CHANGE);
	attachInterrupt(TACH_IN, tach_ISR, FALLING);
	SPARK_CHARGE_TIMER.attachInterrupt(chargeSpark_ISR);    	
	SPARK_DISCHARGE_TIMER.attachInterrupt(dischargeSpark_ISR);
  	FUEL_START_TIMER.attachInterrupt(startFuel_ISR);
  	FUEL_STOP_TIMER.attachInterrupt(stopFuel_ISR);


    killswitch = digitalRead(KILLSWITCH_IN);
    fuelCycle = false;         // start on a non-fuel cycle (arbitrary, since no CAM position sensor)
	currState = NOT_ENGAGED;   // start the engine in its disengaged state
}

//***********************************************************
// Arduino Loop Function
//*********************************************************** 
void loop(void){
    if(sensorDebugMode){

        MAPval = readMap();
        IATval = readIAT();
        ECTval = readECT();
        TPSval = readTPS();

        sprintf(serialBuffer, "%5f         %5f       %5f       %5f", 
                               MAPval,     ECTval,   IATval,   TPSval);

        SerialUSB.println("MAP(kPa):   ECT(K):   IAT(K):   TPS(%)");
        SerialUSB.println(serialBuffer);

    }
    else{
    	switch(currState){

    		case NOT_ENGAGED:
    			if(currAngularSpeed > convertFromRPM(ENGAGE_RPM) && killswitch)
    				currState = READ_SENSORS;
    			break;

    		case READ_SENSORS:
    			currState = CALC;

    			MAPval = readMap();
    			IATval = readIAT();
    			ECTval = readECT();
    			TPSval = readTPS();
    			//O2Val  = readO2();

    			break;

    		case CALC:
    			currState = SERIAL_OUT;

    			if(fuelCycle){
    				// table lookup for volumetric efficiency 
          			volEff = tableLookup(&VETable, convertToRPM(currAngularSpeed), MAPval);

          			// calculate volume of air inducted into the cylinder
                    // [m^3]  =    [%]  *        [m^3]         
          			airVolume =  volEff * ENGINE_DISPLACEMENT;

                    // calculate moles of air inducted into the cylinder
                    // [n]    =   [m^3]   * ([kPa] * [Pa/1kPa]) / ([kg/(mol*K)] * [K] )
                    airMolar  = airVolume * (MAPval * 1E-3)     / (R_CONSTANT   * IATval);

                    // calculate moles of fuel to be injected 
                    // [g fuel] =  [n air] *  [g/n air]     / [g air / g fuel]
                    fuelMass    = airMolar * MOLAR_MASS_AIR / AIR_FUEL_RATIO;

                    // calculate fuel injection duration in microseconds
                    // [us]      =  [g]     * [kg/1E3 g] * [1E6 us/s] / [kg/s]
                    fuelDuration = fuelMass * 1E3 / MASS_FLOW_RATE;		
     				
                    // calculate the angle at which to begin fuel injecting
          			// [degrees]   =   [degrees]    - (    [us]     *   [degrees/us]  )
          			fuelStartAngle = FUEL_END_ANGLE - (fuelDuration * currAngularSpeed); 

                    // update current angular position
          			currEngineAngle = getCurrAngle(currAngularSpeed, calibAngleTime);

                    // determine when [in us] to start the fuel injection pulse and set a timer appropriately
                    //                      (  [degrees]    -    [degrees]   ) /   [degrees/us]   
          			FUEL_START_TIMER.start( (fuelStartAngle - currEngineAngle) / currAngularSpeed );
          		}

          		// find out at what angle to begin/end charging the spark
          		sparkDischargeAngle = TDC - tableLookup(&SATable, convertToRPM(currAngularSpeed), MAPval);  // calculate spark advance angle
          		sparkChargeAngle = sparkDischargeAngle - DWELL_TIME * currAngularSpeed; // calculate angle at which to begin charging the spark

                // update current angular position again, for timer precision
          		currEngineAngle = getCurrAngle(currAngularSpeed, calibAngleTime);

                // determine when [in us] to start charging the coil and set a timer appropriately
                //                         (   [degrees]    -    [degrees]   ) /    [degrees/us]
          		SPARK_CHARGE_TIMER.start( (sparkChargeAngle - currEngineAngle) / currAngularSpeed );

    			break;

    		case SERIAL_OUT:
    			currState = ENGINE_RUNNING;

    			sprintf(serialBuffer, "%5f    %3f         %3f      %4f           %5d", 
    				convertToRPM(currAngularSpeed), MAPval, volEff, sparkDischargeAngle, fuelDuration);

    			SerialUSB.println("RPM:   MAP(kPa):   VE(%):   SPARK(deg):   FUEL PULSE(us):");
    			SerialUSB.println(serialBuffer);

    			break;

    		case ENGINE_RUNNING:
                // 
    			if( convertToRPM(currAngularSpeed) < ENGAGE_RPM ){
    				currState = NOT_ENGAGED;
    			}
    			break;
    	}
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
	currAngularSpeed = calcSpeed(calibAngleTime, lastCalibAngleTime, currAngularSpeed);

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
    digitalWrite(SPARK_OUT, LOW);
	SPARK_DISCHARGE_TIMER.stop();
}

void startFuel_ISR(void){
	FUEL_START_TIMER.stop();
	// start injecting fuel
	digitalWrite(FUEL_OUT, HIGH);
	// set discharge timer for dwell time
	FUEL_STOP_TIMER.start(fuelDuration);
}

void stopFuel_ISR(void){
    // stop injecting fuel
    digitalWrite(FUEL_OUT, LOW);
	FUEL_STOP_TIMER.stop();
}
