//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

//=====[Defines]===============================================================

//Light levels to adjust based on testing of light sensor and potentiometer
#define DAYLIGHT       0.025
#define DUSK           0.005
#define LIGHTS_ON      0.3
#define AUTO_LIGHTS     0.7

#define NUMBER_OF_AVG_SAMPLES    100
#define TIME_INCREMENT_MS        10 //Needed for debounce of ignition?

//=====[Declaration and initialization of public global objects]===============

DigitalIn ignition(BUTTON1);  
DigitalIn driverSeat(D2); 

DigitalOut lowBeamLeft(D3);
DigitalOut lowBeamRight(D4);


UnbufferedSerial uartUsb(USBTX, USBRX, 115200); // set up for debugging

AnalogIn potentiometer(A0);
AnalogIn ldrSensor(A1);

//=====[Declaration and initialization of public global variables]=============

int lightMode = 0; //lights on = 0, auto = 1, off = 2

bool ignitionPressed = false;
bool ignitionReleased = true;
bool lightSystemActive = false;

//=====[Declarations (prototypes) of public functions]=========================

void inputsInit();
void outputsInit();

void ignitionUpdate();
void setLightMode();
void lightControl();

float averageLdrReading();
float averagePotReading();

//=====[Main function, the program entry point after power on or reset]========

int main()
{
    inputsInit();
    outputsInit();
    while (true) {
        ignitionUpdate();  //check for light system activation 
        setLightMode();   //check light dial and set mode
        lightControl(); //if automode, control light, else set them on/off
        delay(TIME_INCREMENT_MS);
    }
}

//=====[Implementations of public functions]===================================

void inputsInit()
{
    driverSeat.mode(PullDown);  //No initialization required for two analog inputs
}

void outputsInit()
{
    lowBeamLeft = OFF;
    lowBeamRight = OFF;
}

void ignitionUpdate()
{
    if ( !lightSystemActive ) {  // Only run if not already active
        if ( driverSeat && ignition) {
            ignitionPressed = true;
//          ignitionReleased = false;
        }
        if ( ignitionPressed && driverSeat) {
            if ( !ignition ) { //ignition released
            lightSystemActive = true;
//            ignitionReleased = true;
            ignitionPressed = false;
            uartUsb.write ("Light System Active \r\n", 22);
            }
        }
    } else {  // Deactivate when system is active, and ignition pressed/released
        if (ignition) {
            ignitionPressed = true;
            }
        if (ignitionPressed && !ignition) {
            lightSystemActive = false;
            ignitionPressed = false;
            uartUsb.write("Light System Inactive\r\n", 24);
            }

        }
    }

void setLightMode() {
    char str[100];
    int strLength;
    float potAve = averagePotReading();

  if (lightSystemActive) {
    sprintf ( str, "Potentiometer: %.3f    ", potAve);
    strLength = strlen(str);
    uartUsb.write( str, strLength );
    if (potAve <= LIGHTS_ON ) {
        lightMode = 0;
//        uartUsb.write("Lights ON \r\n", 11);
    }
    if ((potAve > LIGHTS_ON) && (potAve <= AUTO_LIGHTS) ){
        lightMode = 1;
//uartUsb.write("Lights AUTO \r\n", 13);
    }
    if ((potAve > AUTO_LIGHTS)) {
        lightMode = 2;
//uartUsb.write("Lights OFF \r\n", 12);
    }
  }
}  

void lightControl() {
    char str[100];
    int strLength;
 
   if (lightSystemActive) {
       switch (lightMode) {
           case (0): { //OFF
               lowBeamLeft = OFF;
               lowBeamRight = OFF;
    sprintf ( str, "Potentiometer: %.3f    ", averagePotReading() );
    strLength = strlen(str);
    uartUsb.write( str, strLength );
           }
           case (1): { //AUTO
            sprintf ( str, "LDR Sensor: %.3f\r\n", averageLdrReading() );
            strLength = strlen(str);
            uartUsb.write( str, strLength );
             if ( averageLdrReading() > DAYLIGHT ) {
                lowBeamLeft = ON;
                lowBeamRight = ON;
            } 
            if (averageLdrReading() < DUSK ) {
                lowBeamLeft = OFF;
                lowBeamRight = OFF;
                }  
           }
           case (2): { //ON
                lowBeamLeft = ON;
                lowBeamRight = ON;
           }
       }
   }
}

//These two next functions should be generalized into one function
//with the sensor sent as a parameter
float averageLdrReading()
{
    float ldrSensorAverage  = 0.0;
float ldrSensorSum      = 0.0;
float ldrSensorReadingsArray[NUMBER_OF_AVG_SAMPLES];
float ldrSensorReading  = 0.0;
static int ldrSampleIndex = 0;
    int i = 0;

    ldrSensorReadingsArray[ldrSampleIndex] = ldrSensor.read();
    ldrSampleIndex++;
    if ( ldrSampleIndex >= NUMBER_OF_AVG_SAMPLES) {
        ldrSampleIndex = 0;
    }
    
       ldrSensorSum = 0.0;
    for (i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        ldrSensorSum = ldrSensorSum + ldrSensorReadingsArray[i];
    }
    ldrSensorAverage = ldrSensorSum / NUMBER_OF_AVG_SAMPLES;  
    return ldrSensorAverage;
}

float averagePotReading()
{
    float potAverage  = 0.0;
    float potSum      = 0.0;
    float potReadingsArray[NUMBER_OF_AVG_SAMPLES];
    static int potSampleIndex = 0;
    int i = 0;

    potReadingsArray[potSampleIndex] = potentiometer.read();
    potSampleIndex++;
    if ( potSampleIndex >= NUMBER_OF_AVG_SAMPLES) {
        potSampleIndex = 0;
    }
       potSum = 0.0;
    for (i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        potSum = potSum + potReadingsArray[i];
    }
    potAverage = potSum / NUMBER_OF_AVG_SAMPLES;  
    return potAverage;
}
