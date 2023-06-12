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
void SM_logData(float logMinutes, int logTimes);
void SM_sap_heating(int excitation_min, int off_min);
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

// Heating/logging to update
int heatohm = 14;           // Heater wire's resistance
int heat_min = 20;        // Heating duration, 20, when testing 1
int heat_off_min = 10;     //
float log_min = 0.5;            // Logging interval (min), 15 for rain & sap, 30 for soil, when testing 1

// Variables
String Header; //Information header
int logData_success = 0;

// Sap Flux Sensor Calculations
// int heatval = sqrt(0.2*heatohm)/2.5*255; // FOR HOME-BREWED SENSORS
int heatval = 100; // TEST::::   FOR HOME-BREWED SENSORS, Keep temp diff to be 550-600 uV
// int heatval = 232;        // FOR DYNAMAX SENSORS, 232
float pwm = heatval*100/255;  

// State Machine Variables
long prev_log_millis = -log_min * 1000 * 60;        // will store last time data were logged
unsigned int log_temp_millis = 0;
long prev_excitation_millis = 0;        // will store last time data were logged
int state_log = 0;
int state_prev_log = 0;
int state_excitation = 0;
int state_prev_excitation = 0;

// Rain gauge
volatile unsigned int TipCount = 0; //Global counter for tipping bucket 
unsigned long Holdoff = 25; //25ms between bucket tips minimum
const uint8_t intPin = D2; //D2 for Orchard, DB (Canopy + Open), Linwood (Canopy) TX for some, 

void setup() {
  Header = "Soil Top [V], Soil Mid [V], Soil Bottom [V], Rain [in]";
  // Header = "TC [uV], Cold Jctn [C], Heating, ";
  // Header = "Rain [in]";
  MyLogger.begin(Header); //Pass header info to logger
  RGsetup();
  Log.info("Finish initialization!"); // DEBUG!!
}

void loop() {
   if (Header.substring(0, 2) == "TC") {
    // Sap flux: Log data every 10 minutes, and 3 times each log. 
    SM_sap_heating(heat_min, heat_off_min);
    SM_logData(log_min, 3);
    
    // SM_excitation(heat_min, heat_off_min);
  } else if (Header.substring(0, 4) == "Soil") {
    // Soil moisture: Log data every 15 minutes, and 3 times each log. 
    SM_logData(log_min, 3);
  } else if (Header.substring(0, 4) == "Rain") {
    SM_logData(log_min, 1);
  }
}

void SM_logData(float logMinutes, int logTimes){
  state_prev_log = state_log; 
  unsigned int log_interval_millis = logMinutes * 1000 * 60;
  switch (state_log) {
    case 0:
      state_log = 1;
    break;
    case 1:
      if ((millis() - prev_log_millis) > log_interval_millis) {
        Log.info("Data are logged for %d times every %.1f minutes. ", logTimes, logMinutes);
        prev_log_millis = millis();
        for (int i=0; i<logTimes; i++) {
          logData_success = MyLogger.addDataPoint(calc);  // Add data point
        }
        if (!logData_success){
          Log.error("We don't have a SD card, reset!!"); 
          setup();
        }
      }
    break;
  }
}

void SM_sap_heating(int excitation_min, int off_min) {
  state_prev_excitation = state_excitation;
  unsigned int excitation_interval_millis = excitation_min * 1000 * 60;
  unsigned int off_interval_millis = off_min * 1000 * 60;
  switch (state_excitation) {
    case 0:
      Log.info("Power ON for %d minutes. ", excitation_min);
      state_excitation = 1;
    break;
    case 1:
      excitation();
      if ((millis()-prev_excitation_millis) > excitation_interval_millis){
        prev_excitation_millis = millis();
        Log.info("Power OFF for %d minutes. ", off_min);
        state_excitation = 2;
      }
    break;
    case 2:
      poweroff();
      if ((millis()-prev_excitation_millis) > off_interval_millis){
        prev_excitation_millis = millis();
        Log.info("Power ON for %d minutes. ", excitation_min);
        state_excitation = 1;
      }
    break;
  }
}

void excitation(){
  digitalWrite(D7, HIGH);
  if (Header.substring(0, 2) == "TC") {
    MyLogger.heating(heatval); // default to heat for 20 minutes in the cpp file. 
//   } else if (Header.substring(0, 4) == "Soil") {
//     MyLogger.Soilsetup();
  } else {}
}

void poweroff(){
  digitalWrite(D7, LOW);
  if (Header.substring(0, 2) == "TC") {
    MyLogger.heatingoff(); // turn off heating
  } else if (Header.substring(0, 4) == "Soil") {
    MyLogger.Soiloff(); // turn off relay pin
  } else {}
}

String calc(){
  if (Header.substring(0, 2) == "TC") {
    String sap_outputs;
    sap_outputs = MyLogger.getTCvoltage();
    return sap_outputs;
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
  } else {}
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