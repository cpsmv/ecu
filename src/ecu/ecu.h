/*
 *  Name: ecu.h
 *  Author: Alex Pink
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
 *  calibrate Eco Trons injector & MASS_FLOW_RATE
 *  O2 sensor working
 *  temp sensor linear regression
 *  RPM weighting
 *
 */

#ifndef ECU_H
#define ECU_H

#include "table.h"
#include "tuning.h"
#include "spi_adc.h"
#include "thermistor.h"

/***********************************************************
*                      D E F I N E S
***********************************************************/
// Scientific Constants
#define AIR_FUEL_RATIO 14.7f            // stoichiometric AFR for gasoline [kg/kg] 
#define MOLAR_MASS_AIR 28.97f           // molar mass of dry air [g/mol]
#define R_CONSTANT 8.314f               // ideal gas constant [J/(mol*K)]
#define TDC 360                         // Top Dead Center is 360 degrees [degrees]
#define CELSIUS_TO_KELVIN 273.0f

// Electrical Constants
#define VOLTS_PER_ADC_BIT 1.221E-3//3.23E-3       // for converting an ADC read to voltage 
#define MAX_ADC_VAL 4095.0f

// Engine Parameters
#define DWELL_TIME 4000                 // time required to inductively charge the spark coil [us]
#define CALIB_ANGLE 30                  // tach sensor position from Top Dead Center, in direction of crankshaft rotation [degrees]
#define FUEL_END_ANGLE 120              // when to stop fueling; set for during the intake stroke [degrees]
#define ENGINE_DISPLACEMENT 35.8E-6     // GX35 displacement is 35.8cc, converted to m^3 [1cc * 1E-6 = m^3]
#define MASS_FLOW_RATE 0.6333f          // fueling rate of the injector [g/s]

// Cranking / Engagement Parameters
#define ENGAGE_SPEED    100             // engine's rotational speed must be above this speed to fuel or spark [RPM]
#define CRANKING_SPEED  500             // below this rotational speed, the engine runs a cranking mode fuel enrichment algorithm [RPM]
#define UPPER_REV_LIMIT 6000            // above this rotational speed, the engine enacts a rev limit algorithm [RPM]
#define LOWER_REV_LIMIT 5800            // this is the hysteresis for the rev limit. The engine must fall below this speed to resume normal operation [RPM]
#define CRANK_VOL_EFF   0.30f           // hardcoded value for enrichment cranking enrichment algorithm [%]
#define CRANK_SPARK_ADV 10              // hardcoded cranking spark advance angle [degrees]

// TPS Sensor
#define TPS_RAW_MIN 301.0f
#define TPS_RAW_MAX 3278.0f


/***********************************************************
*         F U N C T I O N S   &   M A C R O S
***********************************************************/             

//#define calcSpeed(CURR_TIME, PREV_TIME, CURR_ANG_SPEED)  (  0.7*(360.0f / (CURR_TIME - PREV_TIME)) + 0.3*(CURR_ANG_SPEED)  )   
#define calcSpeed(CURR_TIME, PREV_TIME) ( 360.0f / (CURR_TIME - PREV_TIME) )
//         [rev/min]                   =    [degrees/us]   * [ (1 rev/360 deg)*(60E6 us/1 min)]
#define convertToRPM(ANGULAR_SPEED_US)  (  ANGULAR_SPEED_US * 166666.7f  )
//         [deg/us]        =  [rev/min] * [(360 deg/1 rev)*(1 min/60E6 us)]
#define convertFromRPM(RPM)  (  RPM       * 6E-6f  )

float getCurrAngle(float angular_speed, unsigned int calib_time);

float readTemp(struct thermistor therm, int adc_channel);
float readMAP(void);
float readO2(void);
float readTPS(void);

float injectorPulse(float airVol, float currMAP, float IAT);
float timeToStartFueling(float fuelDuration, float angularSpeed, float engineAngle);
float timetoChargeSpark(float sparkDischargeAngle, float currAngularSpeed, float currEngineAngle);

#endif