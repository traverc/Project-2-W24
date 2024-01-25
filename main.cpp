//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

//=====[Defines]===============================================================

//Light levels to adjust based on testing of light sensor and potentiometer
#define DAYLIGHT       0.025
#define DUSK           0.005
#define LIGHTS_ON      0.3
#define LIGHTS_OFF     0.7
//Delays for auto mode
#define LIGHT_DELAY_DAY           2000
#define LIGHT_DELAY_DUSK          1000

#define NUMBER_OF_AVG_SAMPLES    100
#define TIME_INCREMENT_MS        10 //Needed for debounce of ignition?
#define DEBOUNCE_BUTTON_TIME_MS                 40

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
bool engineOn = false;
int accumulatedDebounceButtonTime     = 0;
enum buttonState_t {
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_FALLING,
    BUTTON_RISING
};
buttonState_t ignitionState;

//=====[Declarations (prototypes) of public functions]=========================

void inputsInit();
void outputsInit();
void ignitionButtonInit();

void ignitionUpdate();
void setLightMode();
void lightControl();

float averageLdrReading();
float averagePotReading();

bool debounceIgnitionUpdate();

//=====[Main function, the program entry point after power on or reset]========

int main()
{
    inputsInit();
    outputsInit();
    while (true) {
        ignitionUpdate();  //check for light system activation 
        setLightMode();   //check light dial and set mode
        lightControl(); //if automode, control light, else set lights on/off
        delay(TIME_INCREMENT_MS);
    }
}

//=====[Implementations of public functions]===================================

void inputsInit()
{
    driverSeat.mode(PullDown);  //No initialization required for two analog inputs
    ignitionButtonInit();
}

void outputsInit()
{
    lowBeamLeft = OFF;
    lowBeamRight = OFF;
}

void ignitionButtonInit()
{
    if( ignition ) {
        ignitionState = BUTTON_DOWN;
    } else {
        ignitionState = BUTTON_UP;
    }
}
void ignitionUpdate()
{
    bool ignitionReleasedEvent = debounceIgnitionUpdate();
    
    if ( !engineOn ) {  // Turn engine on if it is off
        if ( driverSeat && ignitionReleasedEvent) {
            engineOn = true;
            uartUsb.write ("Engine on \r\n", 12);
        }
    }else{             // Turn engine off if it is on
        if ( driverSeat && ignitionReleasedEvent) {
            engineOn = false;
            uartUsb.write ("Engine off \r\n", 13);
        }
    }
}

void setLightMode() {
    char str[100];
    int strLength;
    float potAve = averagePotReading();

  if (engineOn) {
    sprintf ( str, "Potentiometer: %.3f    ", potAve);
    strLength = strlen(str);
 //   uartUsb.write( str, strLength );
    if (potAve <= LIGHTS_ON ) {
        lightMode = 0;
//        uartUsb.write("Lights ON \r\n", 11);
    }
    if (potAve >= LIGHTS_OFF ){
        lightMode = 2;
//uartUsb.write("Lights OFF \r\n", 12);
    }
    if ((potAve > LIGHTS_ON) && (potAve < LIGHTS_OFF)) {
        lightMode = 1;
//uartUsb.write("Lights AUTO \r\n", 13);
    }
  }
}  

void lightControl() {
    char str[100];
    int strLength;
 
   if (engineOn) {
       switch (lightMode) {
           case (0): { //OFF
               lowBeamLeft = ON;
               lowBeamRight = ON;
    sprintf ( str, "Potentiometer: %.3f    ", averagePotReading() );
    strLength = strlen(str);
    uartUsb.write( str, strLength );
           }
           break;
           case (1): { //AUTO - need to add delay mechanisms
            sprintf ( str, "LDR Sensor: %.3f \r\n", averageLdrReading() );
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
           break;
           case (2): { //OFF
                lowBeamLeft = OFF;
                lowBeamRight = OFF;
           }
           break;

           default:  //indicate something is wrong with OFF/ON
                lowBeamLeft = OFF;
                lowBeamRight = ON;
           break;
       }
   }
}

bool debounceIgnitionUpdate()
{
    bool ignitionReleasedEvent = false;
    switch( ignitionState ) {

    case BUTTON_UP:
        if( ignition ) {
            ignitionState = BUTTON_FALLING;
            accumulatedDebounceButtonTime = 0;
        }
        break;

    case BUTTON_FALLING:
        if( accumulatedDebounceButtonTime >= DEBOUNCE_BUTTON_TIME_MS ) {
            if( ignition ) {
                ignitionState = BUTTON_DOWN;
            } else {
                ignitionState = BUTTON_UP;
            }
        }
        accumulatedDebounceButtonTime = accumulatedDebounceButtonTime +
                                        TIME_INCREMENT_MS;
        break;

    case BUTTON_DOWN:
        if( !ignition ) {
            ignitionState = BUTTON_RISING;
            accumulatedDebounceButtonTime = 0;
        }
        break;

    case BUTTON_RISING:
        if( accumulatedDebounceButtonTime >= DEBOUNCE_BUTTON_TIME_MS ) {
            if( !ignition ) {
                ignitionState = BUTTON_UP;
                ignitionReleasedEvent = true;
            } else {
                ignitionState = BUTTON_DOWN;
            }
        }
        accumulatedDebounceButtonTime = accumulatedDebounceButtonTime +
                                        TIME_INCREMENT_MS;
        break;

    default:
        ignitionButtonInit();
        break;
    }
    return ignitionReleasedEvent;
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
