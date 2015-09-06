#include <SPI.h>

/*
 *  Name: ecu.c
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
 *  calibrate Eco Trons injector & MASS_FLOW_RATE
 *  O2 sensor working
 *  temp sensor linear regression
 *  RPM weighting
 *
 */
#include <Arduino.h>
#include <DueTimer.h>
#include <stdio.h>
#include "ecu.h"

/******************************************************************************************
*                                    D E F I N E S
******************************************************************************************/
// Mode Definitions
//#define DIAGNOSTIC_MODE
#define DEBUG_MODE

// Pin Definitions 
#define KILLSWITCH_IN   9
#define MAP_IN          A0
#define ECT_IN          A8
#define IAT_IN          A7
#define O2_IN           A1
#define TPS_IN          A11
#define TACH_IN         5
#define SPARK_OUT       7
#define FUEL_OUT        8
#define DAQ_CS          4

// Timers
#define FUEL_START_TIMER        Timer0
#define FUEL_STOP_TIMER         Timer1
#define SPARK_CHARGE_TIMER      Timer2
#define SPARK_DISCHARGE_TIMER   Timer3
#define STATE_DEBUG_TIMER       Timer4
#define TACH_DEBOUNCE_TIMER     Timer5
#define NUM_TACH_DEBOUNCE_CHECKS 3

// Serial
#define SERIAL_PORT Serial

// SPI
#define MAP_ADC_CHNL 4
#define O2_ADC_CHNL  0
#define IAT_ADC_CHNL 2
#define ECT_ADC_CHNL 3
#define TPS_ADC_CHNL 1

/***********************************************************
*             G L O B A L   V A R I A B L E S
***********************************************************/ 
// State Machine
typedef enum e_state {
    READ_SENSORS,
    CALIBRATION,
    CRANKING,
    RUNNING,
    REV_LIMITER,
    SERIAL_OUT
} state;

volatile state currState;


// Booleans
volatile bool killswitch;
volatile bool fuelCycle;
bool revLimit;

// Serial 
static char serialOutBuffer[100];
volatile int serialPrintCount;

// Sensor Readings
volatile float MAPval;  // Manifold Absolute Pressure reading, w/ calibration curve [kPa]
volatile float ECTval;   // Engine Coolant Temperature reading, w/ calibration curve [K]
volatile float IATval;   // Intake Air Temperature reading, w/ calibration curve [K]
volatile float TPSval;   // Throttle Position Sensor Reading [%]
volatile float O2val;    // Oxygen Sensor Reading, w/ calibration curve [AFR]

// Thermistors
struct thermistor ECT = {-45.08, 95.22, 30, 100, 5.0, 1.2};
struct thermistor IAT = {-45.08, 88.83, 30, 100, 5.0, 1.2};

// Real Time Stuff
volatile float currAngularSpeed;             // current speed [degrees per microsecond]
volatile float currRPM;                      // current revolutions per minute [RPM]
volatile unsigned int calibAngleTime;        // time of position calibration [microseconds]
volatile unsigned int lastCalibAngleTime;    // last posiition calibration time, for RPM calcs [microseconds]
float volEff;                       // volumetric efficiency [% out of 100]
float airVolume;                    // volume of inducted air [m^3]
float airMolar;                     // moles of inducted air [mols]
float fuelMass;                     // mass of fuel to be injected [g]
float fuelDuration;                 // length of injection pulse [us]
float fuelDurationDegrees;          // length of injection pulse [degrees]
volatile float sparkChargeAngle;             // when to start inductively charging the spark coil [degrees]
volatile float sparkDischargeAngle;          // when to discharge the coil, chargeAngle + DWELL_TIME [degrees]
volatile float fuelStartAngle;               // when to start the injection pulse [degrees]
float currEngineAngle;              // current engine position [degrees]
int numTachClicks;
volatile float tachResistance;

int debounce;


/***********************************************************
*                  F U N C T I O N S   
***********************************************************/ 
float getCurrAngle(float angular_speed, unsigned int calib_time){
    //[degrees] = [degrees/us]  * (  [us]   -     [us]  ) +  [degrees]
    float angle = angular_speed * (micros() - calib_time) + CALIB_ANGLE;
    // always keep the engine angle less than 360 degrees
    return angle >= 360 ? angle - 360 : angle;
}

float readTemp(struct thermistor therm, int adc_channel){
    // [K] =                                    [C]                        +       [K]                  
    return   thermistorTemp(therm, readADC(adc_channel)*VOLTS_PER_ADC_BIT) + CELSIUS_TO_KELVIN;
}

float readMAP(void){
    //     [kPa]     =           [ADC]       * [V/ADC]           
    float MAPvoltage = readADC(MAP_ADC_CHNL) * VOLTS_PER_ADC_BIT;

    if( MAPvoltage < 0.5f )
        return 20.0f;
    else if( MAPvoltage > 4.9f )
        return 103.0f;
    else                  
        //          [V]   * [kPa/V] + [kPa]
        return MAPvoltage * 18.86   + 10.57;
}

float readO2(void){
    // [kg/kg] = (           [ADC]     *      [V/ADC]     ) * [(kg/kg)/V] + [kg/kg]
    return       (readADC(O2_ADC_CHNL) * VOLTS_PER_ADC_BIT) * 3.008       + 7.35;
}

/* From experimentation,
    Low TPS rawVal: 301
    High TPS rawVal: 3261
*/

float readTPS(void){
    int rawTPS = readADC(TPS_ADC_CHNL);

    if( rawTPS < TPS_RAW_MIN )
        return 0.0;
    else if( rawTPS > TPS_RAW_MAX )
        return 1.0;
    else
        return ((float)rawTPS - TPS_RAW_MIN) / (TPS_RAW_MAX - TPS_RAW_MIN);
}

float injectorPulse(float airVol, float currMAP, float IAT){
    // calculate moles of air inducted into the cylinder
    // [n]    =   [m^3] * ([kPa]   * [1000 Pa/1kPa]) / ([J/(mol*K)] * [K] )
    airMolar  = airVol  * (currMAP * 1E3)            / (R_CONSTANT   * IAT);

    // calculate moles of fuel to be injected 
    // [g fuel] =  [n air] *  [g/n air]     / [g air / g fuel]
    fuelMass    = airMolar * MOLAR_MASS_AIR / AIR_FUEL_RATIO;

    // calculate fuel injection duration in microseconds
    // [us] =  [g]     * [1E6 us/s] / [g/s]
    return    fuelMass * 1E6        / MASS_FLOW_RATE;     
}

float timeToStartFueling(float fuelDuration, float angularSpeed, float engineAngle){
    // calculate the angle at which to begin fuel injecting
    // [degrees]   =   [degrees]    - (    [us]     *   [degrees/us]  )
    fuelStartAngle = FUEL_END_ANGLE - (fuelDuration * angularSpeed); 

    // determine when [in us] to start the fuel injection pulse and set a timer appropriately
    //                      (  [degrees]    -    [degrees]   ) /   [degrees/us]   
    return ( (fuelStartAngle - engineAngle) / angularSpeed );
}

float timetoChargeSpark(float sparkDischargeAngle, float currAngularSpeed, float currEngineAngle){

    sparkChargeAngle = sparkDischargeAngle - DWELL_TIME * currAngularSpeed; // calculate angle at which to begin charging the spark

    // determine when [in us] to start charging the coil and set a timer appropriately
    //        (   [degrees]    -    [degrees]   ) /    [degrees/us]
    return ( (sparkChargeAngle - currEngineAngle) / currAngularSpeed ); 
}


void updateTach(){

    #ifdef DIAGNOSTIC_MODE

        numTachClicks++;
        lastCalibAngleTime = calibAngleTime;
        calibAngleTime = micros();
        currAngularSpeed = calcSpeed(calibAngleTime, lastCalibAngleTime);
        currRPM = convertToRPM(currAngularSpeed);

        sprintf(serialOutBuffer, "%2f    %5f     %d         %d       %d      %5f", 
        currRPM, currAngularSpeed, calibAngleTime, lastCalibAngleTime, numTachClicks, tachResistance);

        SERIAL_PORT.println("RPM:    Ang Spd:    recentTime:    lastTime:    numTac:    tachR:");
        SERIAL_PORT.println(serialOutBuffer);
        SERIAL_PORT.println("\n");

    #else // ECU state machine

        numTachClicks++;
        lastCalibAngleTime = calibAngleTime;
        calibAngleTime = micros();
        currAngularSpeed = calcSpeed(calibAngleTime, lastCalibAngleTime);
        currRPM = convertToRPM(currAngularSpeed);

        #ifdef DEBUG_MODE
        Serial.println(numTachClicks);
        #endif

        // all of the following code assumes one full rotation has occurred (with a single toothed
        // crankshaft, this is true)

        fuelCycle = !fuelCycle;
        serialPrintCount = (serialPrintCount >= 9 ? serialPrintCount = 0 : serialPrintCount + 1);

        currState = CALIBRATION;

    #endif 
}


/***********************************************************
*      A R D U I N O    S E T U P    F U N C T I O N
***********************************************************/ 
void setup(void){
    // Set 12-bit ADC Readings
    //analogReadResolution(12);

    // Pin directions
    pinMode(KILLSWITCH_IN, INPUT);
    pinMode(TACH_IN, INPUT);
    pinMode(SPARK_OUT, OUTPUT);
    pinMode(FUEL_OUT, OUTPUT);

    digitalWrite(SPARK_OUT, LOW);
    digitalWrite(FUEL_OUT, LOW);

    // Initialize Variables
    currAngularSpeed = 0;
    calibAngleTime = 0;             
    lastCalibAngleTime = 0;       
    currRPM = 0;  
    numTachClicks = 0;

    // set up all interrupts and timers
    attachInterrupt(KILLSWITCH_IN, killswitch_ISR, CHANGE);
    attachInterrupt(TACH_IN, hallEffect_ISR, FALLING);
    SPARK_CHARGE_TIMER.attachInterrupt(chargeSpark_ISR);        
    SPARK_DISCHARGE_TIMER.attachInterrupt(dischargeSpark_ISR);
    FUEL_START_TIMER.attachInterrupt(startFuel_ISR);
    FUEL_STOP_TIMER.attachInterrupt(stopFuel_ISR); 

    // start Serial communication
    SERIAL_PORT.begin(115200);
    serialPrintCount = 1;   // wait 5 cycles before printing any information

    initSPI();      // set up SPI communication to the MCP3304 DAQ
    //initTach();     // set up Tach circuit
    //tachResistance = setTachResistance( calcTachResistance(CRANKING_SPEED + TACH_SAFETY_MARGIN) );

    killswitch = digitalRead(KILLSWITCH_IN);
    fuelCycle = false;          // start on a non-fuel cycle (arbitrary, since no CAM position sensor)
    revLimit = false;
    currState = READ_SENSORS;   // start off by reading sensors

    // state machine DEBUG_MODE timer, every 1ms
    /*#ifdef DEBUG_MODE
    STATE_DEBUG_TIMER.attachInterrupt(stateDebug_ISR);
    STATE_DEBUG_TIMER.start(1000000);
    #endif*/
}

/***********************************************************
*       A R D U I N O    L O O P    F U N C T I O N
***********************************************************/ 
void loop(void){

    #ifdef DIAGNOSTIC_MODE

        // Serial stuff and lock
        char serialChar;
        static bool commandLock = 0;

        // Raw ADC reads
        int MAPraw = readADC(MAP_ADC_CHNL);
        int IATraw = readADC(IAT_ADC_CHNL);
        int ECTraw = readADC(ECT_ADC_CHNL);
        int TPSraw = readADC(TPS_ADC_CHNL);
        int O2raw  = readADC(O2_ADC_CHNL);
        int channelTest = readADC(7);

        /*sprintf(serialOutBuffer, "%d  %d  %d  %d  %d  %d", 
        MAPraw, ECTraw, IATraw, TPSraw, O2raw, channelTest);

        SERIAL_PORT.println("MAP:  ECT:  IAT:  TPS:  O2:  Test:");
        SERIAL_PORT.println(serialOutBuffer);*/

        // Calibrated sensor reads
        MAPval = readMAP();
        IATval = readTemp(IAT, IAT_ADC_CHNL);
        ECTval = readTemp(ECT, ECT_ADC_CHNL);
        TPSval = readTPS();
        O2val  = readO2();
        killswitch = digitalRead(KILLSWITCH_IN);
        int tachRaw = digitalRead(TACH_IN);

        sprintf(serialOutBuffer, "%2f  %2f  %2f  %2f  %2f   %d   %d   %5f", 
        MAPval, (ECTval-CELSIUS_TO_KELVIN), (IATval-CELSIUS_TO_KELVIN), TPSval, O2val, killswitch, tachRaw, tachResistance);

        SERIAL_PORT.println("MAP:    ECT:       IAT:       TPS:   O2:  KillSW:  Tach:  tachR:");
        SERIAL_PORT.println(serialOutBuffer);
        SERIAL_PORT.println("\n");

        // Serial keyboard commands
        if( SERIAL_PORT.available() ){

            serialChar = SERIAL_PORT.read();

            if(serialChar == 'l'){
                commandLock = true;
                SERIAL_PORT.println("Unlocked.");
            }
            else if(serialChar == 's' && commandLock){
                SERIAL_PORT.println("Performing spark ignition event test in 2s.");
                SPARK_CHARGE_TIMER.start(2000000);
                commandLock = false;
            }
            else if(serialChar == 'f' && commandLock){
                SERIAL_PORT.println("Performing fuel ignition pulse test in 2s.");
                FUEL_START_TIMER.start(2000000);
                fuelDuration = 10000;
                commandLock = false;
            }
        }
        
        delay(1000); // delay so serial port is readable and serial buffer is not overloaded

    #else // ECU state machine

        switch(currState){

        // Constantly poll the all sensors. The "default" state (all state flows eventually lead back here).
        //if( currState == READ_SENSORS ){
        case READ_SENSORS:

            currState = READ_SENSORS;

            MAPval = readMAP();
            IATval = readTemp(IAT, IAT_ADC_CHNL);
            //ECTval = readECT();
            TPSval = readTPS();
            O2val  = readO2();

            break; // state machine case READ_SENSORS 
        //}

        // when the TACH ISR determines it has reached its calibration angle, this state is selected. This state directs the
        // flow of the state machine.
        //else if( currState == CALIBRATION ){
        case CALIBRATION:
            switch(killswitch){

                case true:
                    if( revLimit ){
                        if( currRPM < LOWER_REV_LIMIT){
                            revLimit = false;
                            currState = RUNNING;
                            #ifdef DEBUG_MODE
                            SERIAL_PORT.println("INFO: rev limiter deactivated");
                            #endif
                        }
                        else currState = READ_SENSORS;
                    }
                    else{
                        if( currRPM >= UPPER_REV_LIMIT )
                            currState = REV_LIMITER;
                        else if( currRPM < UPPER_REV_LIMIT && currRPM >= CRANKING_SPEED )
                            currState = RUNNING;
                        else if( currRPM < CRANKING_SPEED && currRPM >= ENGAGE_SPEED )
                            currState = CRANKING;
                        else
                            currState = READ_SENSORS;
                    }
                    break;  // killswitch case true

                case false:
                    currState = READ_SENSORS;
                    break;  // killswitch case false
            }

            #ifdef DEBUG_MODE
            switch(currState){
                case READ_SENSORS:
                    SERIAL_PORT.println("INFO: to READ_SENSORS");
                    break;  // print case READ_SENSORS
                case CRANKING:
                    SERIAL_PORT.println("INFO: to CRANKING");
                    break;  // print case CRANKING
                case RUNNING:
                    SERIAL_PORT.println("INFO: to RUNNING");
                    break;  // print case RUNNING
                case REV_LIMITER:
                    SERIAL_PORT.println("INFO: to REV_LIMITER");
                    break;  // print case REV_LIMITER
                //default:
                    //SERIAL_PORT.println("INFO: state machine to READ_SENSORS");
                    //break;
            }
            #endif

            break;  // state machine case CALIBRATION
        //}

        // CRANKING uses hardcoded values for Volumetric Efficiency and Spark Advance angle. It is an engine
        // enrichment algorithm that usually employs a rich AFR ratio.
        // else if( currState == CRANKING ){
        case CRANKING:

            //currState = (serialPrintCount == 0 ? SERIAL_OUT : READ_SENSORS); 

            currState = READ_SENSORS;

            //tachResistance = setTachResistance( calcTachResistance(CRANKING_SPEED + TACH_SAFETY_MARGIN) );

            if(fuelCycle){
                // calculate volume of air inducted into the cylinder, using hardcoded cranking VE
                // [m^3]  =    [%]         *        [m^3]         
                airVolume =  CRANK_VOL_EFF * ENGINE_DISPLACEMENT;

                // Calculate fuel injector pulse length
                fuelDuration = injectorPulse(airVolume, MAPval, IATval);

                // update current angular position
                currEngineAngle = getCurrAngle(currAngularSpeed, calibAngleTime);

                // calculate and set the time to start injecting fuel
                FUEL_START_TIMER.start( timeToStartFueling(fuelDuration, currAngularSpeed, currEngineAngle) );
            }
            // find out at what angle to begin/end charging the spark
            sparkDischargeAngle = TDC - CRANK_SPARK_ADV;    // hardcoded cranking spark advance angle

            // update current angular position again, for timer precision
            currEngineAngle = getCurrAngle(currAngularSpeed, calibAngleTime);

            // calculate and set the time to start charging the spark coil
            SPARK_CHARGE_TIMER.start( timetoChargeSpark(sparkDischargeAngle, currAngularSpeed, currEngineAngle) );

            break;  // state machine case CRANKING
        //}

        // This state runs during the normal running operation of the engine. The Volumetric Efficiency and 
        // Spark Advance are determined using fuel map lookup tables. 
        //else if( currState == RUNNING ){
        case RUNNING:

            //currState = (serialPrintCount == 0 ? SERIAL_OUT : READ_SENSORS);

            currState = READ_SENSORS;

            //tachResistance = setTachResistance( calcTachResistance(currRPM) - TACH_SAFETY_MARGIN );
            //tachResistance = setTachResistance( calcTachResistance(CRANKING_SPEED + TACH_SAFETY_MARGIN) );

            if(fuelCycle){
                // table lookup for volumetric efficiency 
                volEff = table2DLookup(&VETable, convertToRPM(currAngularSpeed), MAPval);

                // calculate volume of air inducted into the cylinder
                // [m^3]  =    [%]  *        [m^3]         
                airVolume =  volEff * ENGINE_DISPLACEMENT;

                // Calculate fuel injector pulse length
                fuelDuration = injectorPulse(airVolume, MAPval, IATval); 

                // update current angular position
                currEngineAngle = getCurrAngle(currAngularSpeed, calibAngleTime);

                // calculate and set the time to start injecting fuel
                FUEL_START_TIMER.start( timeToStartFueling(fuelDuration, currAngularSpeed, currEngineAngle) );
            }

            // find out at what angle to begin charging the spark coil
            sparkDischargeAngle = TDC - table2DLookup(&SATable, convertToRPM(currAngularSpeed), MAPval);  // calculate spark advance angle

            // update current angular position again, for timer precision
            currEngineAngle = getCurrAngle(currAngularSpeed, calibAngleTime);
            
            // calculate and set the time to start charging the spark coil
            SPARK_CHARGE_TIMER.start( timetoChargeSpark(sparkDischargeAngle, currAngularSpeed, currEngineAngle) );

            break;  // state machine case RUNNING
        //}
        // When the engine's RPM is greater than UPPER_REV_LIMIT, the engine must enact a rev limiter algorithm,
        // to prevent possible damage to the engine internals / hardware. The engine doesn't fuel or spark.
        //else if( currState == REV_LIMITER ){
        case REV_LIMITER:

            //currState = (serialPrintCount == 0 ? SERIAL_OUT : READ_SENSORS);

            currState = READ_SENSORS;

            //tachResistance = setTachResistance( calcTachResistance(LOWER_REV_LIMIT) );

            #ifdef DEBUG_MODE
            SERIAL_PORT.println("INFO: rev limiter activated");
            #endif

            revLimit = true;

            break;  // state machine case REV_LIMITER
        //}

        //else if( currState == SERIAL_OUT ){
        case SERIAL_OUT:

            currState = READ_SENSORS;
        
            //sprintf(serialOutBuffer, "%1f     %2f        %2f       %2f       %2f          %5d            %2f", 
            //    currRPM, MAPval, volEff, IATval, sparkDischargeAngle, fuelDuration, O2val);

            SERIAL_PORT.println("RPM:     MAP:     VE:     SPARK:     FUEL:     O2:");
            SERIAL_PORT.print(currRPM);
            SERIAL_PORT.print("  ");
            SERIAL_PORT.print(MAPval);

            break;  // state machine case SERIAL_OUT
        //}

        // If the engine is in an undefined state, alert of the situation and go back to read sensors
        //else{
        default:

            currState = READ_SENSORS;

            SERIAL_PORT.println("INFO: undefined state");

            break;  // state machine case default
        //}

        } // switch statement bracket

    #endif
}


/***********************************************************
*   I N T E R R U P T   S E R V I C E    R O U T I N E S
***********************************************************/
void hallEffect_ISR(void){
    detachInterrupt(TACH_IN);   // disable hall effect interrupts until debouncing completed
    debounce = 0;

    //Serial.println("hf");

    TACH_DEBOUNCE_TIMER.attachInterrupt(tachDebounce_ISR);
    TACH_DEBOUNCE_TIMER.start(1);
}

void tachDebounce_ISR(void){

    TACH_DEBOUNCE_TIMER.stop();

    if( digitalRead(TACH_IN) == LOW){

        debounce++;

        //Serial.print("d");
        //Serial.println(debounce);

        if(debounce >= NUM_TACH_DEBOUNCE_CHECKS){
            updateTach();
            attachInterrupt(TACH_IN, hallEffect_ISR, FALLING);
        }
        else{
            TACH_DEBOUNCE_TIMER.start(1);
            //Serial.println("dgo");
        }
    }
    else{
        attachInterrupt(TACH_IN, hallEffect_ISR, FALLING);
        //Serial.println("df");
    }
}

void killswitch_ISR(void){
    killswitch = digitalRead(KILLSWITCH_IN);
}

void chargeSpark_ISR(void){
    SPARK_CHARGE_TIMER.stop();
    // start chargine coil
    digitalWrite(SPARK_OUT, HIGH);
    // set discharge timer for dwell time
    SPARK_DISCHARGE_TIMER.start(DWELL_TIME);

    #ifdef DIAGNOSTIC_MODE
    SERIAL_PORT.println("Charging spark.");
    #endif

}

void dischargeSpark_ISR(void){
    // discharge coil
    digitalWrite(SPARK_OUT, LOW);
    SPARK_DISCHARGE_TIMER.stop();

    #ifdef DIAGNOSTIC_MODE
    SERIAL_PORT.println("Discharging spark.");
    #endif
}

void startFuel_ISR(void){
    FUEL_START_TIMER.stop();
    // start injecting fuel
    digitalWrite(FUEL_OUT, HIGH);
    // set discharge timer for dwell time
    FUEL_STOP_TIMER.start(fuelDuration);

    #ifdef DIAGNOSTIC_MODE
    SERIAL_PORT.println("Starting fuel pulse for 10ms.");
    #endif
}

void stopFuel_ISR(void){
    // stop injecting fuel
    digitalWrite(FUEL_OUT, LOW);
    FUEL_STOP_TIMER.stop();

    #ifdef DIAGNOSTIC_MODE
    SERIAL_PORT.println("Ending fuel pulse.");
    #endif
}

#ifdef DEBUG_MODE
void stateDebug_ISR(void){

    STATE_DEBUG_TIMER.stop();

    switch(currState){
        case READ_SENSORS:
            SERIAL_PORT.println("INFO: READ_SENSORS");
            break;
        case CALIBRATION:
            SERIAL_PORT.println("INFO: CALIBRATION");
            break;
        case CRANKING:
            SERIAL_PORT.println("INFO: CRANKING");
            break;
        case RUNNING:
            SERIAL_PORT.println("INFO: RUNNING");
            break;
        case REV_LIMITER:
            SERIAL_PORT.println("INFO: REV_LIMITER");
            break;
        case SERIAL_OUT:
            SERIAL_PORT.println("INFO: SERIAL_OUT");
            break;
    }

    STATE_DEBUG_TIMER.start(1000000);
}
#endif