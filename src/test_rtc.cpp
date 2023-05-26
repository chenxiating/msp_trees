/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/xchen/Documents/Arduino/particle/msp_trees-main/src/test_rtc.ino"
/*
 * Simple data logger.
 */
#include <SPI.h>
#include "TestLibrary.h"


void setup();
void loop();
void check_SD(int k);
int logData(unsigned int logMinutes, int logTimes);
void excitation();
void poweroff();
String calc();
void RGsetup();
void tip();
String getRain();
#line 8 "/Users/xchen/Documents/Arduino/particle/msp_trees-main/src/test_rtc.ino"
SYSTEM_MODE(MANUAL);
// SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;

// Define data logger
TestLib MyLogger;

// Heating/logging to update
int heatohm = 14;           // Heater wire's resistance
float heatmin = 20;        // Heating duration, 20, when testing 1
int logTime = 30;            // Logging interval (min), 30, when testing 1

// Variables
String Header; //Information header
int k = 0;

// Data logger & Calculations
long previousMillis = - logTime * 1000 * 60;        // will store last time data were logged
// int heatval = sqrt(0.2*heatohm)/2.5*255; // FOR HOME-BREWED SENSORS
int heatval = 60; // TEST::::   FOR HOME-BREWED SENSORS, Keep temp diff to be 550-600 uV
// int heatval = 232;        // FOR DYNAMAX SENSORS, 232
float pwm = heatval*100/255;  


// Rain gauge
volatile unsigned int TipCount = 0; //Global counter for tipping bucket 
unsigned long Holdoff = 25; //25ms between bucket tips minimum
const uint8_t intPin = D2; //TX for some, D2 for Orchard, DB (Canopy)

void setup() {
  // Header = "Soil Top [V], Soil Mid [V], Soil Bottom [V], Rain [in]";
  Header = "TC [uV]";
  // Header = "Rain [in]";
  MyLogger.begin(Header); //Pass header info to logger
  RGsetup();
  Log.info("Finish initialization!"); // DEBUG!!
  // k = logData(logTime, 3);
  // check_SD(k);
}

void loop() {
   if (Header.substring(0, 2) == "TC") {
    // Sap flux: Log data every 10 minutes, and 3 times each log. 
    k = logData(logTime, 3);
  } else if (Header.substring(0, 4) == "Soil") {
    // Soil moisture: Log data every 15 minutes, and 3 times each log. 
    k = logData(logTime, 3);
  } else if (Header.substring(0, 4) == "Rain") {
    k = logData(logTime, 1);
  }
  check_SD(k);
}

void check_SD(int k) {
  if (k == 0) {
    Log.error("We don't have a SD card, reset!!");
    setup();
  } 
}

int logData(unsigned int logMinutes, int logTimes) {
  unsigned long currentMillis = millis();
  unsigned int logInterval = logMinutes * 1000 * 60;
  int logData_success = 1;
  if ((currentMillis - previousMillis) > logInterval) {
    Log.info("Data are logged for %d times every %d minutes. ", logTimes, logMinutes);
    // save the last time you logged data
    previousMillis = currentMillis; 
    excitation();       // Turn on heating or soil moisture relay
    for (int i = 0; i < logTimes; i++) {
      logData_success = MyLogger.addDataPoint(calc);  // Add data point
      if (!logData_success){
        break;
      }
      delay(10000);     // wait 10 seconds for a second data point
    }
    poweroff();
  }
  return logData_success;
}

void excitation(){
  digitalWrite(D7, HIGH);
  Serial.println("Power ON");
  if (Header.substring(0, 2) == "TC") {
    Log.info("0.2W for %d Ohm heater, PWM is %.0f%% (%d/255).", heatohm, pwm, heatval);
    MyLogger.heating(heatval, heatmin); // default to heat for 20 minutes in the cpp file. 
  } else if (Header.substring(0, 4) == "Soil") {
    MyLogger.Soilsetup();
  } else {}
}

void poweroff(){
  if (Header.substring(0, 2) == "TC") {
    MyLogger.heatingoff(); // turn off heating
  } else if (Header.substring(0, 4) == "Soil") {
    MyLogger.Soiloff(); // turn off relay pin
  } else {}
  digitalWrite(D7, LOW);
  Serial.println("Power OFF");

}

String calc(){
  if (Header.substring(0, 2) == "TC") {
    double volt;
    volt = MyLogger.getTCvoltage();
    return String(volt);
  } else if (Header.substring(0, 4) == "Soil") {
    String rain;
    String soil;
    soil = MyLogger.getSoilvoltage();
    rain = getRain();
    return soil+","+rain;
  } else if (Header.substring(0, 4) == "Rain") {
    String rain;
    rain = getRain();
    return rain;
  } 
}

// RAIN GAUGE CODE
void RGsetup() {
    pinMode(intPin, INPUT_PULLUP); // For rain gauge
    attachInterrupt(digitalPinToInterrupt(intPin), tip, FALLING);
    
}

void tip()
{ 
	static unsigned long TimeLocal = millis(); //Protect the system from rollover errors
	if((millis() - TimeLocal) > Holdoff) {
		TipCount++; //Increment global counter 
	}
	//Else do nothing
	TimeLocal = millis(); //Update the local time
}

String getRain() 
{
	float Val1 = TipCount*0.01;  //Account per tip (in)
  Serial.print("Tip Count: "); // DEBUG!
  Serial.println(TipCount); // DEBUG!
	TipCount = 0; //Clear count with each update 
	return String(Val1);
}