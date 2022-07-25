/*
 * Simple data logger.
 */
#include <SPI.h>
#include "TestLibrary.h"

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

// Variables
String Header; //Information header

void setup() {
  // Header = "Soil Top [V], Soil Mid [V], Soil Bottom [V], Rain [in], ";
  Header = "TC [uV]";
  // Header = "Test"; // DEBUG!!
  MyLogger.begin(Header); //Pass header info to logger
  Serial.println("Finish initialization!"); // DEBUG!!
  MyLogger.initLogFile();
  pinMode(intPin, INPUT); // For rain gauge
  if (Header.substring(0, 2) == "TC") {
    Serial.println("Heat up now");
    delay(5*1000*60);
  } 
  
}

void loop() {
  cycle(10);
}

void cycle(int minutes) {
  MyLogger.addDataPoint(calc);
  delay(10000);
  MyLogger.addDataPoint(calc);
  delay(10000);
  MyLogger.addDataPoint(calc);
  delay(10000);
  // SystemSleepConfiguration config;
  // config.mode(SystemSleepMode::ULTRA_LOW_POWER)
  //       .gpio(D2, FALLING);
  // System.sleep(config);
  Serial.println("Sleep now");
  delay(minutes*60*1000-30000);

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


// Rain gauge code
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