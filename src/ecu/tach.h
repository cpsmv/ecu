/*
 *    
 *
 */


#define CS_100K 3
#define CS_10K 2

#define R_BW_100K    107000.0f
#define R_W_100K     99.3f
#define R_RES_100K   835.9375f       // (R_BW_100k / 128 bits)

#define R_BW_10K     9660.0f
#define R_W_10K      93.2f
#define R_RES_10K    75.46875f      // (R_BW_10k / 128 bits)

#define NUM_TRIG_TEETH 1            // number of teeth on trigger wheel
#define FIVE55_TIMER_CONST 1.1      // ln(3) ~= 1.1
#define TACH_CAPACITANCE 0.000001f  // 1.0uF

// Prototypes
void initTach();
float setTachResistance(float resistance);
void setDigiPotResistance(int chipSelectPin, int wiperPos);
float calcTachResistance(float currRPM);
