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
void logData(unsigned int logMinutes, int logTimes);
void excitation();
void poweroff();
String calc();
void RGsetup();
void tip();
String getRain();
#line 7 "/Users/xchen/Documents/Arduino/particle/msp_trees-main/src/test_rtc.ino"
SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;

// Define data logger
TestLib MyLogger;
SdFat SD;

// Rain gauge
volatile unsigned int TipCount = 0; //Global counter for tipping bucket 
unsigned long Holdoff = 25; //25ms between bucket tips minimum
const uint8_t intPin = TX;

// Heating
int heatval = 126;      // This depends on the heater wire's resistance

// Variables
String Header; //Information header

// Data logger
long previousMillis = 0;        // will store last time data were logged

void setup() {
  Header = "Soil Top [V], Soil Mid [V], Soil Bottom [V], Rain [in], ";
  // Header = "TC [uV]";
  MyLogger.begin(Header); //Pass header info to logger
  Serial.println("Finish initialization!"); // DEBUG!!
  MyLogger.initLogFile();
  pinMode(intPin, INPUT); // For rain gauge
}

void loop() {
  // // Sap flux: Log data every 30 minutes, and 3 times each log. 
  // logData(30, 3);
  // Soil moisture: Log data every 15 minutes, and 3 times each log. 
  logData(15, 3);
}

void logData(unsigned int logMinutes, int logTimes) {
  unsigned long currentMillis = millis();
  unsigned int logInterval = logMinutes * 1000 * 60;
  if((currentMillis - previousMillis) > logInterval) {
    // save the last time you logged data
    previousMillis = currentMillis; 
    excitation();       // Turn on heating or soil moisture relay
    for (int i = 0; i < logTimes; i++) {
      MyLogger.addDataPoint(calc);  // Add data point
      delay(10000);     // wait 10 seconds for a second data point
    }
    poweroff();
  }
}

void excitation(){
  digitalWrite(D7, HIGH);
  if (Header.substring(0, 2) == "TC") {
    MyLogger.heating(heatval); // default to heat for 20 minutes in the cpp file. 
  } else {
    MyLogger.Soilsetup();
  }
}

void poweroff(){
  if (Header.substring(0, 2) == "TC") {
    MyLogger.heatingoff(); // turn off heating
  } else {
    MyLogger.Soiloff(); // turn off relay pin
  }
  digitalWrite(D7, LOW);
  Serial.println("Power off");

}

String calc(){
  if (Header.substring(0, 2) == "TC") {
    double volt;
    volt = MyLogger.getTCvoltage();
    return String(volt);
  } else {
    String rain;
    String soil;
    soil = MyLogger.getSoilvoltage();
    Log.info("Soil: ");
    Serial.println(soil);
    rain = getRain();
    Log.info("Rain: ");
    Serial.println(rain);
    return soil+","+rain;
  }
}


// RAIN GAUGE CODE
void RGsetup() {
    attachInterrupt(digitalPinToInterrupt(intPin), tip, FALLING);
    pinMode(intPin, INPUT_PULLUP);
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
	float Val1 = TipCount*8.3;  //Account for volume per tip (mL)
	TipCount = 0; //Clear count with each update 
	return String(Val1);
}