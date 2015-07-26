/*
 *  Name: check_ecu.c
 *  Author: Alex Pink
 *
 *  Description:
 *
 *
 *  Note to the programmer:
 *  Please use Arduino IDE to compile / upload sketches! To use the built-in
 *  Arduino libraries, the IDE must be used. The Makefile and code directory
 *  structure is too convoluted to use.
 *
 *  TODO LIST:
 *
 */
#include <math.h>
#include "gtest/gtest.h"
#include "../ecu.h"
//#include "../ecu.cpp"

//#define PRINTING

#ifdef PRINTING
using namespace std;
#endif

/******************************************************************************************
*                                 S P E E D    T E S T 
******************************************************************************************/
TEST(SpeedTest, calcSpeedTest){
    unsigned long calibTime = 62000;    // us
    unsigned long lastCalibTime = 500;  // us
    float oldSpeed = 0.006;             // 1000 rpm = .006 degrees/us
    float newSpeed = calcSpeed(calibTime, lastCalibTime, oldSpeed);

    EXPECT_FLOAT_EQ(0.00589756, newSpeed); 
    #ifdef PRINTING
    cout << "             Speed after 1st test = " << newSpeed << "\n";
    #endif

    lastCalibTime = calibTime;
    oldSpeed = newSpeed;
    calibTime = calibTime + 61000;
    newSpeed = calcSpeed(calibTime, lastCalibTime, oldSpeed);

    EXPECT_FLOAT_EQ(0.0059004155, newSpeed);
    #ifdef PRINTING
    cout << "             Speed after 2nd test = " << newSpeed << "\n";
    #endif

    // zero case
    //EXPECT_FLOAT_EQ(INFINITY, calcSpeed(500, 500, 0.007f));
}

TEST(SpeedTest, rpmTest){
    // converting from deg/us to RPM
    EXPECT_FLOAT_EQ(1166.6669, convertToRPM(0.007f));
    EXPECT_FLOAT_EQ(0, convertToRPM(0));
    EXPECT_FLOAT_EQ(166666.7f, convertToRPM(1));

    // converting from RPM to deg/us
    EXPECT_FLOAT_EQ(.042, convertFromRPM(7000));
    EXPECT_FLOAT_EQ(0, convertFromRPM(0));
    EXPECT_FLOAT_EQ(6E-6f, convertFromRPM(1));

}

/******************************************************************************************
*                             F U E L    E Q N    T E S T 
******************************************************************************************/
TEST(FuelEqnTest, pulseLength){
    float map = 101.3f;
    float airVol = 0.6 * ENGINE_DISPLACEMENT;
    float iat = 20.0f + CELSIUS_TO_KELVIN;
    float pulse = injectorPulse(airVol, map, iat);
    EXPECT_FLOAT_EQ(4632.72481648, pulse);


    //4.47155694529e-10
    //8.81231324525e-10
}


/******************************************************************************************
*                                       M A I N 
******************************************************************************************/
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}