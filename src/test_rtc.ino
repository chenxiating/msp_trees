/*
 * Simple data logger.
 */
#include <SPI.h>
#include "TestLibrary.h"

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler;
SystemSleepConfiguration config;

// Define data logger
TestLib MyLogger;
SdFat SD;

// Rain gauge
volatile unsigned int TipCount = 0; //Global counter for tipping bucket 
unsigned long Holdoff = 25; //25ms between bucket tips minimum
const uint8_t intPin = D2;
// const uint8_t intPin = TX; // Dayton's Bluff
static unsigned long TimeLocal = millis(); //Protect the system from rollover errors

// Variables
String Header; //Information header
bool relayState = false;

std::chrono::milliseconds publishPeriod = 10min;
unsigned long lastPublishMs;

int readmin = 10;
int heatmin = 10;
int dursec = 10;

// // PIN Def
// const uint8_t topPinOut = D3;
// const uint8_t midPinOut = D4;
// const uint8_t botPinOut = A3;

// void setup() {
//     config.mode(SystemSleepMode::ULTRA_LOW_POWER)
//         .duration(1min);
// }

// void loop() {
//   digitalWrite(D7, HIGH);
//   delay(10*1000);
//   digitalWrite(D7, LOW);
//   Serial.println(millis()); // DEBUG!
//   System.sleep(config);
// }

void setup() {
  Header = "Soil Top [mV], Soil Mid [mV], Soil Bottom [mV], Rain [in], ";
  // Header = "Rain [in], ";
  // Header = "TC [uV]";
  // Header = "Test"; // DEBUG!!
  MyLogger.begin(Header); //Pass header info to logger
  Serial.println("Finish initialization!"); // DEBUG!!
  MyLogger.initLogFile();
  pinMode(intPin, INPUT); // For rain gauge
  MyLogger.getTime();
  RGsetup();
  config.mode(SystemSleepMode::STOP)
    .gpio(intPin, FALLING)
    .duration(6h);
}

void loop() {
  if (Header.substring(0, 2) == "TC") { 
    // MyLogger.addDataPoint(calc);
    // delay(100);
    delay(20min);
    MyLogger.relayOn(true);
    digitalWrite(D7, HIGH);
    delay(3*1000);
    MyLogger.addDataPoint(calc);
    digitalWrite(D7, LOW);
    MyLogger.relayOn(false);
    delay(27*1000);
    MyLogger.relayOn(true);
    digitalWrite(D7, HIGH);
    delay(3*1000);
    MyLogger.addDataPoint(calc);
    digitalWrite(D7, LOW);
    MyLogger.relayOn(false);
    delay(27*1000);
    delay(10min);
  } else {
    Serial.println(millis()); // DEBUG!
    MyLogger.relayOn(true);
    digitalWrite(D7, HIGH);
    for (int i = 0; i < 10; i++) {
      delay(3*1000);
      MyLogger.addDataPoint(calc);
    }
    digitalWrite(D7, LOW);
    MyLogger.relayOn(false);
    // delay(57*1000);
    Serial.println(millis()); // DEBUG!
    System.sleep(config);
  }  


  // if (Header.substring(0, 2) == "TC") { 
  //   Log.info("Heating stops");
  //   // Sleep
  //   int sleepms = (readmin)*60*1000-dursec*1000*3;
  //   MyLogger.sleep(sleepms);
  // } 
  

}

String calc(){
  if (Header.substring(0, 2) == "TC") {
    double volt;
    volt = MyLogger.getTCvoltage();
    return String(volt);
  } else if (Header.substring(0, 4) == "Rain")
  {
    String rain;
    rain = getRain();
    return rain;
  } else {
    String rain;
    String soil;
    soil = MyLogger.getSoilvoltage();
    // Log.info("Soil: "); // DEBUG!
    // Serial.println(soil); // DEBUG!
    rain = getRain();
    // Log.info("Rain: "); // DEBUG!
    // Serial.println(rain); // DEBUG!
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
  //Protect the system from rollover errors
	if((millis() - TimeLocal) > Holdoff) {
		TipCount++; //Increment global counter 
    // Serial.println(TipCount); // DEBUG!
	}
	//Else do nothing
	TimeLocal = millis(); //Update the local time
}

String getRain() 
{
	float Val1 = TipCount*0.01;  //Account for per tip (inch)
	TipCount = 0; //Clear count with each update 
	return String(Val1);
}