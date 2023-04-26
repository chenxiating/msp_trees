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
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;

// Define data logger
TestLib MyLogger;

// Rain gauge
volatile unsigned int TipCount = 0; //Global counter for tipping bucket 
unsigned long Holdoff = 25; //25ms between bucket tips minimum
const uint8_t intPin = D2; //TX for some, D2 for Orchard

// Heating
int heatval = 133;      // This depends on the heater wire's resistance
float heatmin = 0.1;        // Heating duration

// Variables
String Header; //Information header

// Data logger
long previousMillis = 0;        // will store last time data were logged

void setup() {
  // Header = "Soil Top [V], Soil Mid [V], Soil Bottom [V], Rain [in], ";
  Header = "TC [uV]";
  MyLogger.begin(Header); //Pass header info to logger
  Serial.println("Finish initialization!"); // DEBUG!!
  pinMode(intPin, INPUT); // For rain gauge
  RGsetup();
}

void loop() {
  int k = 0;
   if (Header.substring(0, 2) == "TC") {
    // Sap flux: Log data every 20 minutes, and 3 times each log. 
    k = logData(0.5, 3);
  } else {
    // Soil moisture: Log data every 15 minutes, and 3 times each log. 
    k = logData(15, 3);
  }
  check_SD(k);
}

void check_SD(int k) {
  if (k == 1) {
  } else {
    Serial.println("We don't have a SD card, reset!!");
    setup();
  } 
}

int logData(unsigned int logMinutes, int logTimes) {
  char strBuf[50];
  sprintf(strBuf, "Data are logged for %d times every %d minutes. ", logTimes, logMinutes);
  unsigned long currentMillis = millis();
  unsigned int logInterval = logMinutes * 1000 * 60;
  int logData_success = 1;
  if ((currentMillis - previousMillis) > logInterval) {
    // save the last time you logged data
    previousMillis = currentMillis; 
    excitation();       // Turn on heating or soil moisture relay
    for (int i = 0; i < logTimes; i++) {
      logData_success = MyLogger.addDataPoint(calc);  // Add data point
      delay(5000);     // wait 10 seconds for a second data point
    }
    poweroff();
  }
  return logData_success;
}

void excitation(){
  digitalWrite(D7, HIGH);
  Serial.println("Power ON");
  if (Header.substring(0, 2) == "TC") {
    Serial.print("PWM is ");
    float pwm = heatval*100/255;
    Serial.println(pwm);
    MyLogger.heating(heatval, heatmin); // default to heat for 20 minutes in the cpp file. 
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
  Serial.println("Power OFF");

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
    rain = getRain();
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
	float Val1 = TipCount*0.01;  //Account per tip (in)
	TipCount = 0; //Clear count with each update 
	return String(Val1);
}