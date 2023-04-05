#ifndef TestLib_h
#define TestLib_h

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "RTClib.h"
#include "SdFat.h"
#include "Adafruit_MCP9600.h"

#define I2C_ADDRESS (0x67)

class TestLib {
    public:
        // Constructor
        TestLib(bool displayMsg=true);

        // Methods
        int begin(String header_ = "");
        void run(String (*f)(void), unsigned long logInterval);

        // Data reading
        int logStr(String val);
        void addDataPoint(String (*update)(void));
        void initLogFile();

        float getBatVoltage();
        String getTime();
        double getTCvoltage();
        String getSoilvoltage();

        // Data logging
        void logSoil(int logInterval, int logTimes, String (*update)(void));
        void loopSap(int logInterval, int logTimes, String (*update)(void));

        // String getRain();
        
        // Pin definitions
		uint8_t SD_CS = 5;
        uint8_t BlueLED = 7;
        uint8_t intPin = TX;
        uint8_t heatPin = A5;
        uint8_t soilRelayPin = D7;      // DOUBLE CHECK RELAY PIN

        // Heating and Excitation
        void heating(int heatval);
        void Soilsetup();
        void heatingoff();
        void Soiloff();

    private:
    
        RTC_PCF8523 rtc;
        SdFat SD; 
        Adafruit_MCP9600 mcp;

        // Linwood (all 3), Orchard's Park (top and mid), Highland (top and top)
        const uint8_t topPinIn = A0;
        const uint8_t midPinIn = A1;
        const uint8_t botPinIn = A2; 

        // // Dayton's Bluff
        // const uint8_t topPinIn = A1;
        // const uint8_t midPinIn = A0;
        // const uint8_t botPinIn = A2;

        const char* SN; //Used to store device serial number, 19 chars + null terminator
        String Header = "";
        String LogTimeDate = "2063/04/05 20:00:00";
        DateTime now;
        const char HexMap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}; //used for conversion to HEX of string

        void RTCsetup();
        void SDsetup();
        void dirsetup();
        void TCsetup();
        // void RGsetup();

        // Rain gauge
        volatile unsigned int TipCount = 0; //Global counter for tipping bucket 
        unsigned long Holdoff = 25; //25ms between bucket tips minimum

        double getVoltage(int pinInput);
        // void RGtip();
        static TestLib* selfPointer;

        void blinkGood();

        void I2Ctest();
		void clockTest();
		void batTest();
		void powerTest();
        String getOnBoardVals();

		bool OBError = false;
		bool SensorError = false;
		bool TimeError = false;
		bool SDError = false; //USE??
		bool BatError = false;
		bool BatWarning = false;
		float BatVoltageError = 3.3; //Low battery alert will trigger if voltage drops below this value
		float BatPercentageWarning = 50; //Percentage at which a warning will be indicated
        double TCvoltage = NAN;
        double Soilvoltage = 0;
        int Time_Date[6];

        static void dateTimeSD(uint16_t* date, uint16_t* time);

        volatile bool LogEvent = false; //Used to test if logging should begin yet
		volatile bool NewLog = false; //Used to tell system to start a new log
		volatile int AwakeCount = 0;

        char FileNameC[11]; //Used for file handling
};


#endif