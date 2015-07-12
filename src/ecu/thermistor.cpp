/*
 *  Name: thermistor.cpp
 *  Author: Alex Pink
 *
 *  Description:
 *  Datatypes and functions for reading thermistor temperature sensors. 
 *
 *  Thermistors are passive temperature sensors that change resistance with temperature. They are
 *  exploited electrically by putting one in series with a pullup resistor. One side of the thermistor
 *  is connected to circuit ground, while one is connected to a middle node. 
 *
 *  Vcc                    
 *   ^                        --> Vsensor       ^
 *   |       Rpullup          |                /
 *   -------/\/\/\/\/\--------------------/\/\/\/\/\------------
 *                                           / Rthermistor     |
 *                                          /                 GND
 *
 *  The thermistor is electrically modeled by a variable resistor. its resistance is first determined
 *  from a backwards voltage divider calculation. 
 *
 *  Every thermistor has a Resistance over Temperature plot associated with it. However, the two are
 *  negatively exponentially related. To prevent excessive calculation (and because a lot of thermistors
 *  only normally have two or three data points), a linear regression must be performed to determine a 
 *  slope and intercept.
 *
 *  Since we are going from resistance back to temperature, I have named the regression parameters
 *  inverse slope and inverse y-intercept. This is because we are plotting the resistance on an 
 *  inverse Temperature over Resistance plot. All of the regression must run on an outside 
 *  calculator (for now).
 *
 *  Note to the programmer:
 *  Please use Arduino IDE to compile / upload sketches! To use the built-in
 *  Arduino libraries, the IDE must be used. The Makefile and code directory
 *  structure is too convoluted to use.
 *
 *  TODO LIST:
 *  linear regression algorithm, run one time during setup
 *
 */

#include "thermistor.h"

// calculates the given thermistor's temperature from a pullup resistor network
float thermistorTemp(struct thermistor therm, int rawSensorVoltage){

    // find the thermistor's resistance from electrical conditions
    //           [kOhms]      =       [Volts]     *          [kOhms]           /           [Volts]         -       [Volts]
    float currThermResistance = (rawSensorVoltage * therm.pullupResistor) / (therm.supplyVoltage - rawSensorVoltage);

    // find the temperature from linear-regression values of a thermistor Resistance/Temperature plot
    //       [C]   =       [C/jOhms]     *        [kOhms]      +          [C]
    float currTemp = therm.invSlope * currThermResistance + therm.invYintercept;

    // ensure the temperature is appropriate by checking it lies within the approximated linear region
    if( currTemp < therm.lowTempLimit )
        return therm.lowTempLimit;
    else if( currTemp > therm.highTempLimit )
        return therm.highTempLimit;
    else
        return currTemp;
}

