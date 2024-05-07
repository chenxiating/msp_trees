/*
 * Simple data logger.
 */
#include <SPI.h>
#include "TestLibrary.h"

SYSTEM_MODE(AUTOMATIC);// 
// SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;
TestLib MyLogger; // Define data logger


//****** TO EDIT ******* //
//** REMEMBER to edit eventName **//
// This is the event name to publish
String eventName = "Sap"; // "Sap", "Rain", "Soil"
// Heating/logging to update
int heatohm = 14;           // Heater wire's resistance
int heat_min = 1;        // Heating duration, 20, when testing 1
int heat_off_min = 1;     //
float log_min = 0.5;            // Logging interval (min), 5 for sap, 15 for rain & soil, when testing 1,

// Sap Flux Sensor Calculations
// int heatval = sqrt(0.2*heatohm)/2.5*255; // FOR HOME-BREWED SENSORS
int heatval = 20; // TEST: FOR HOME-BREWED SENSORS, Keep temp diff to be 550-600 uV (120 should be 0.2 mW)
// int heatval = 232;        // FOR DYNAMAX SENSORS, 232
float pwm = heatval*100/255;  
//***** END OF EDIT *****//
//** REMEMBER to edit eventName **//


// Data Logger & State Machine Constants
String Header; //Information header
int logData_success = 0;
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
const uint8_t intPin = D2; //D2 for Orchard, DB (Canopy + Open), Linwood (Canopy); TX for some, 

void setup() {
  if (eventName == "Soil") {
    Header = "Soil Top [V], Soil Mid [V], Soil Bottom [V], Rain [in]";
  } else if (eventName == "Sap") {
    Header = "TC [uV], Cold Jctn [C], Heating, ";
  } else if (eventName == "Rain") {
    Header = "Rain [in]";
  }
  
  MyLogger.begin(Header); //Pass header info to logger
  RGsetup();
  Log.info("Data are logged every %.1f minutes. ", log_min);
  Log.info("Finish initialization!"); // DEBUG!!
}

void loop() {
   if (Header.substring(0, 2) == "TC") {
    // Sap flux: Log data every 10 minutes, and 3 times each log. 
    SM_sap_heating(heat_min, heat_off_min);
    SM_logData(log_min, 1);
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
        // Log.info("Data are logged for %d times every %.1f minutes. ", logTimes, logMinutes);
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
      Log.info("Power ON for %d minutes. A5 val: %d", excitation_min, heatval);
      digitalWrite(D7, HIGH);
      state_excitation = 1;
    break;
    case 1:
      MyLogger.heating(heatval); // turn on heating
      if ((millis()-prev_excitation_millis) > excitation_interval_millis){
        prev_excitation_millis = millis();
        Log.info("Power OFF for %d minutes, A5 val: %d", off_min, heatval);
        digitalWrite(D7, LOW);
        state_excitation = 2;
      }
    break;
    case 2:
      analogWrite(A5,0);  // Turn off heating
      if ((millis()-prev_excitation_millis) > off_interval_millis){
        prev_excitation_millis = millis();
        Log.info("Power ON for %d minutes. ", excitation_min);
        digitalWrite(D7, HIGH);
        state_excitation = 1;
      }
    break;
  }
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
  } else {
    return "Error";
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
    Serial.println("Tip");
	}
	//Else do nothing
	TimeLocal = millis(); //Update the local time
}

String getRain() 
{
	float Val1 = TipCount*0.01;  //Account per tip (in)
  // Serial.print("Tip Count: "); // DEBUG!
  // Serial.println(TipCount); // DEBUG!
	TipCount = 0; //Clear count with each update 
	return String(Val1);
}