/*
 *    
 *
 */
#include <SPI.h>
#include "tach.h"

void initTach(){
   pinMode(CS_100K, OUTPUT); 
   pinMode(CS_10K, OUTPUT);

    // disable device to start with 
   digitalWrite(CS_100K, HIGH);
   digitalWrite(CS_10K, HIGH); 
}

float setTachResistance(float resistance){
   // set 100K wiper position first
   int r100kPos = ( (resistance > R_BW_100K ? R_BW_100K : resistance) - R_W_100K) / R_RES_100K;
   setDigiPotResistance(CS_100K, r100kPos);

   // set 10k wiper position with remaining resistance
   float remaining = resistance - r100kPos * R_RES_100K;
   int r10kPos = ( (remaining > R_BW_10K ? R_BW_10K : remaining) - R_W_10K) / R_RES_10K;
   setDigiPotResistance(CS_10K, r10kPos);

   // return actual resistance set (less than entered resistance)
   return r100kPos * R_RES_100K + r10kPos * R_RES_10K; 
}

void setDigiPotResistance(int chipSelectPin, int wiperPos){

   digitalWrite(chipSelectPin, LOW);
   SPI.transfer( B00000000 );
   SPI.transfer( B11111111 & (wiperPos <= 0 ? 0 : wiperPos) );
   digitalWrite(chipSelectPin, HIGH);
}

float calcTachResistance(float currRPM){
   //        [s/tooth]   =   ( [rev/min]] * [tooth/rev] / [s/min] )^(-1)
   float secondsPerTooth = 1 / (currRPM   * NUM_TRIG_TEETH / 60);

   //Serial.print("Pulse: ");
   //Serial.println(secondsPerTooth);

   // R] = [s/tooth]      / (   [1.1]           *        F        )
   return secondsPerTooth / (FIVE55_TIMER_CONST * TACH_CAPACITANCE);
}
