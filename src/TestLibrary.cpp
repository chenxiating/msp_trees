# include "TestLibrary.h"       // Quotes - same folder, angle bracket - wider scope

TestLib* TestLib::selfPointer;

TestLib::TestLib(bool displayMsg) {
    // Anything you need when instantiatiating your object goes here
    // Pin definition
}

// Pretend this is one or more complext and involved functions you have written

// Public Functions
// ----------------------------------,----------
int TestLib::begin(String header_) {
    Log.info("Beginning logger initialization");
    // RTCsetup(); // ONLY USE DURING INITIAL SETUP!
    rtc.begin();
    
    String id = System.deviceID();
    SN = id.c_str();

    Log.info("\n\n\nInitializing...\n");
    delay(1000);
    getTime();
    Log.info("\nTimestamp = "+LogTimeDate);

    Header = header_;
    Log.info(Header);

    //Sets up basic initialization required for the system
	selfPointer = this;

    SDsetup();
    clockTest();
    // SdFile::dateTimeCallback(dateTimeSD); //Setup SD file time setting

    batTest();
    blinkGood();
    initLogFile();

    if (Header.substring(0, 2) == "TC") {
        TCsetup();
        pinMode(heatPin, OUTPUT);
        analogWrite(heatPin,0);  // Turn off heating
        Log.info("Set up for sap flux");
    } else {
        Log.info("Set up for soil moisture or rain gauge");
    }
    return 1;
}

void TestLib::RTCsetup() {
    #ifndef ESP8266
    while (!Serial); // wait for serial port to connect. Needed for native USB
    #endif

    Serial.println("Setting up RTC");

    if (! rtc.begin()) {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1) delay(10);
    }

    if (! rtc.initialized()) {
        Serial.println("RTC is NOT initialized, let's set the time!");
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // This line sets the RTC with an explicit date & time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
        //
        // Note: allow 2 seconds after inserting battery or applying external power
        // without battery before calling adjust(). This gives the PCF8523's
        // crystal oscillator time to stabilize. If you call adjust() very quickly
        // after the RTC is powered, lostPower() may still return true.
    }

    // When time needs to be re-set on a previously configured device, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // UNCOMMENT THIS
    // Serial.print("Did this adjust outside of initialze loop"); // DEBUG!
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

    // When the RTC was stopped and stays connected to the battery, it has
    // to be restarted by clearing the STOP bit. Let's do this to ensure
    // the RTC is running.
    rtc.begin();

    // The PCF8523 can be calibrated for:
    //        - Aging adjustment
    //        - Temperature compensation
    //        - Accuracy tuning
    // The offset mode to use, once every two hours or once every minute.
    // The offset Offset value from -64 to +63. See the Application Note for calculation of offset values.
    // https://www.nxp.com/docs/en/application-note/AN11247.pdf
    // The deviation in parts per million can be calculated over a period of observation. Both the drift (which can be negative)
    // and the observation period must be in seconds. For accuracy the variation should be observed over about 1 week.
    // Note: any previous calibration should cancelled prior to any new observation period.
    // Example - RTC gaining 43 seconds in 1 week
    float drift = 43; // seconds plus or minus over oservation period - set to 0 to cancel previous calibration.
    float period_sec = (7 * 86400);  // total obsevation period in seconds (86400 = seconds in 1 day:  7 days = (7 * 86400) seconds )
    float deviation_ppm = (drift / period_sec * 1000000); //  deviation in parts per million (μs)
    float drift_unit = 4.34; // use with offset mode PCF8523_TwoHours
    // float drift_unit = 4.069; //For corrections every min the drift_unit is 4.069 ppm (use with offset mode PCF8523_OneMinute)
    int offset = round(deviation_ppm / drift_unit);
    // rtc.calibrate(PCF8523_TwoHours, offset); // Un-comment to perform calibration once drift (seconds) and observation period (seconds) are correct
    // rtc.calibrate(PCF8523_TwoHours, 0); // Un-comment to cancel previous calibration

    Serial.print("Offset is "); Serial.println(offset); // Print to control offset
    Serial.println("Time is set!");
}

void TestLib::clockTest()
{
	int Error = 1;
	uint8_t TestSeconds = 0;
	bool OscStop = false;

	Serial.print("Clock: ");
	Wire.beginTransmission(0x68);
  	Wire.write(0xFF); // DEBUG: Margay address!
    Wire.write(0x0F); // DEBUG!! PCF8523_CLKOUTCONTROL
	Error = Wire.endTransmission();    

	if(Error == 0) {
		getTime(); //FIX!
		TestSeconds = now.second();
	  	delay(1100);
        DateTime later = rtc.now();
	  	if(later.second() == TestSeconds) {
	  		OBError = true; //If clock is not incrementing
	  	}
	}

	unsigned int YearNow = now.year();

	if(YearNow == 2000) {  //If value is 2000, work around Y2K bug by setting time to Jan 1st, midnight, 2049
		// if(YearNow <= 00) RTC.setTime(2018, 01, 01, 00, 00, 00);  //Only reset if Y2K
		// getTime(); //Update local time
		TimeError = true;
		Serial.println(" PASS, BAD TIME");
	}

	if(Error != 0) {
		Serial.println(" FAIL");
		OBError = true;
	}

	else if(Error == 0 && OscStop == false && TimeError == false) {
		Serial.println(" PASS");
	}
}

void TestLib::SDsetup() {
    getTime();
    SdFile::dateTimeCallback(dateTimeSD); //Setup SD file time setting
    bool SDError = false;
	// bool SD_Test = true;

    // SD_CS is pulled up: HIGH=1 if not present
    // SD card being inserted closes a switch to pull it LOW
	pinMode(SD_CS, INPUT);

	Serial.print("SD: ");
	delay(5); //DEBUG!
	if (!SD.begin(SD_CS)) {
        Log.warn(" NO CARD");
  	    OBError = true;
  	    SDError = true;
	} else {
    randomSeed(analogRead(D1)); 
    int RandVal = random(30557); //Generate a random number between 0 and 30557 (the number of words in Hamlet)
    char RandDigits[6] = {0};
    sprintf(RandDigits, "%d", RandVal); //Convert RandVal into a series of digits
    int RandLength = (int)((ceil(log10(RandVal))+1)*sizeof(char)); //Find the length of the values in the array
    SD.remove("SDtest.txt");

File DataWrite = SD.open("SDtest.txt", FILE_WRITE);
    if(DataWrite) {
        DataWrite.println(RandVal);
        DataWrite.println("\nHe was a man. Take him for all in all.");
        DataWrite.println("I shall not look upon his like again.");
        DataWrite.println("-Hamlet, Act 1, Scene 2");
    }
    DataWrite.close();
    char TestDigits[6] = {0};
	  File DataRead = SD.open("SDtest.txt", FILE_READ);
	  if(DataRead) {
    	DataRead.read(TestDigits, RandLength);
		  for(int i = 0; i < RandLength - 1; i++){ //Test random value string
		    if(TestDigits[i] != RandDigits[i]) {
		      SDError = true;
		      OBError = true;
		    }
		  }
	  }
	  DataRead.close();
}
if(SDError) Serial.println("FAIL");  //If card is inserted and still does not connect propperly, throw error
	else if(!SDError) Serial.println("PASS");  //If card is inserted AND connectects propely return success
}

void TestLib::TCsetup() {
    /* Initialise the driver with I2C_ADDRESS and the default I2C bus. */
    if (! mcp.begin(I2C_ADDRESS)) {
        Serial.println("Thermocouple amplifier not found. Check wiring!");
        while (1);
    }

    Serial.println("Found MCP9600!");

    mcp.setADCresolution(MCP9600_ADCRESOLUTION_18);
    Serial.print("ADC resolution set to ");
    switch (mcp.getADCresolution()) {
        case MCP9600_ADCRESOLUTION_18:   Serial.print("18"); break;
        case MCP9600_ADCRESOLUTION_16:   Serial.print("16"); break;
        case MCP9600_ADCRESOLUTION_14:   Serial.print("14"); break;
        case MCP9600_ADCRESOLUTION_12:   Serial.print("12"); break;
    }
    Serial.println(" bits");

    mcp.setThermocoupleType(MCP9600_TYPE_T);
    Serial.print("Thermocouple type set to ");
    switch (mcp.getThermocoupleType()) {
        case MCP9600_TYPE_K:  Serial.print("K"); break;
        case MCP9600_TYPE_J:  Serial.print("J"); break;
        case MCP9600_TYPE_T:  Serial.print("T"); break;
        case MCP9600_TYPE_N:  Serial.print("N"); break;
        case MCP9600_TYPE_S:  Serial.print("S"); break;
        case MCP9600_TYPE_E:  Serial.print("E"); break;
        case MCP9600_TYPE_B:  Serial.print("B"); break;
        case MCP9600_TYPE_R:  Serial.print("R"); break;
    }
    Serial.println(" type");

    mcp.setFilterCoefficient(3);
    Serial.print("Filter coefficient value set to: ");
    Serial.println(mcp.getFilterCoefficient());

    mcp.setAlertTemperature(1, 30);
    Serial.print("Alert #1 temperature set to ");
    Serial.println(mcp.getAlertTemperature(1));
    mcp.configureAlert(1, true, true);  // alert 1 enabled, rising temp

    mcp.enable(true);

    Serial.println(F("------------------------------"));
}

void TestLib::heating(int val){
    if (SDError) {
        Log.error("No SD card. Pause heating to conserve power.");
    } else {
        analogWrite(heatPin, val);
        // Log.info("%s Heating begins and will continue for %.2f min", (const char*) getTime(), heatmin);
        // Serial.println("In TestLib::heating");
        // delay(60000);
        // Serial.println("In TestLib::heating, EOS");
    }
}

void TestLib::batTest()
{
	if(getBatVoltage() < BatVoltageError) BatError = true; //Set error flag if below min voltage
	Serial.print("Bat = ");
	Serial.print(getBatVoltage());
	Serial.print("V\t");
}

void TestLib::blinkGood() {
    pinMode(BlueLED, OUTPUT);
    delay(2000);
    digitalWrite(BlueLED, HIGH);
    delay(5000);
    digitalWrite(BlueLED, LOW);
}

String TestLib::getTime() 
{
    now = rtc.now();
    char temp[50];
    sprintf(temp, "%04d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), 
    now.day(), now.hour(), now.minute(), now.second());    
    LogTimeDate = String(temp);
    Time_Date[0] = now.year();
    Time_Date[1] = now.month();
    Time_Date[2] = now.day();
    Time_Date[3] = now.hour();
    Time_Date[4] = now.minute();
    Time_Date[5] = now.second();
    // Serial.println("The time is: "+ LogTimeDate); // DEBUG!
    return LogTimeDate;
}

void TestLib::dateTimeSD(uint16_t* date, uint16_t* time)
{   
	// DateTime now = RTC.now();
	// sprintf(timestamp, "%02d:%02d:%02d %2d/%2d/%2d \n", now.hour(),now.minute(),now.second(),now.month(),now.day(),now.year()-2000);
	// Serial.println("yy");
	// Serial.println(timestamp);
	// return date using FAT_DATE macro to format fields
	// Serial.println(selfPointer->Time_Date[0]); //DEBUG!
	*date = FAT_DATE(selfPointer->Time_Date[0], selfPointer->Time_Date[1], selfPointer->Time_Date[2]);

	// return time using FAT_TIME macro to format fields
	*time = FAT_TIME(selfPointer->Time_Date[3], selfPointer->Time_Date[4], selfPointer->Time_Date[5]);
}

int TestLib::addDataPoint(String (*update)(void)) //Reads new data and writes data to SD
{
	String data = "";
	data = (*update)(); //Run external update function
	data = getOnBoardVals() + data; //Prepend on board readings
	// Serial.println("Got OB Vals");  //DEBUG!
	Log.info(data);
    publishData(data);
    return logStr(data);
}

void TestLib::publishData(String addDataPoint_output) // publish data on Particle Cloud
{
  // Make sure we're cloud connected before publishing
  if (Particle.connected())
  { 
    String myID = System.deviceID();
    addDataPoint_output = addDataPoint_output.replace("/","");
    addDataPoint_output = addDataPoint_output.replace(":","");
    addDataPoint_output = addDataPoint_output.replace(" ","");
    char eventData [100];
    sprintf(eventData, "[%s]", addDataPoint_output.c_str());
    Serial.println(eventData);
    Particle.publish(myID, eventData, PRIVATE);
    // Particle.publish(myID, addDataPoint_output, PRIVATE);
  }
  else
  {
    Log.info("not cloud connected");
  }
}

String TestLib::getOnBoardVals() 
{   
    float BatVoltage = getBatVoltage(); //Get battery voltage, Include voltage divider in math
    LogTimeDate = getTime();
    return LogTimeDate + "," + String(BatVoltage) + ",";
}

float TestLib::getBatVoltage()
{   
    float BatVoltage = fuel.getVCell();
	return BatVoltage;
}

String TestLib::getTCvoltage() 
{
    // Serial.print("ADC: "); Serial.print(mcp.readADC() * 2); Serial.println(" uV");
    if (!(mcp.readThermocouple() == NAN)) {
        TCvoltage = mcp.readADC() * 2;
        // Serial.print("hot jct: ");
        // Serial.print(mcp.readThermocouple());
        // Serial.print(", cold jct: ");
        // Serial.println(mcp.readAmbient());
    }
    return String(TCvoltage) + "," + String(mcp.readAmbient()) + "," + String(digitalRead(D7));
}

void TestLib::Soilsetup(){
    pinMode(soilRelayPin, OUTPUT);
    digitalWrite(soilRelayPin, HIGH); // HIGH - turns on excitation bc it's "Normally On"
    delay(1*1000); // Wait 1 seconds for the excitation power
}

String TestLib::getSoilvoltage() {   
    Soilsetup();
    double topVolt = getVoltage(topPinIn);
    double midVolt = getVoltage(midPinIn); 
    double botVolt = getVoltage(botPinIn);
    digitalWrite(soilRelayPin,LOW); // Turn off soil excitation power
    return String(topVolt) + "," + String(midVolt) + "," + String(botVolt);
}

double TestLib::getVoltage(int pinInput)
{
    pinMode(pinInput, INPUT);
    double volt = analogRead(pinInput) * (3.3 / 4096); 
    return volt;
}

void TestLib::dirsetup(){
    if (!SD.exists("Particle")) {
        SD.mkdir("Particle");
        SD.chdir("Particle");
        if (!SD.exists(SN)) {
            SD.mkdir(SN);
            SD.chdir(SN);
            if (!SD.exists("Logs")) {
                SD.mkdir("Logs");
            }
        }   
    } 
}

void TestLib::initLogFile()
{
    dirsetup();
	SD.chdir("Particle");  //Move into northern widget folder from root
    SD.chdir(SN);  //Move into specific numbered sub folder    
    SD.chdir("Logs"); //Move into the logs sub-folder

    // Serial.println("Supposed to be in Logs");
	//Perform same search, but do so inside of "SD:NW/sn/Logs"
    char NumCharArray[6];
    String FileName = "Log";
    int FileNum = 1;
    sprintf(NumCharArray, "%05d", FileNum);
    //Serial.print("NumString: ");
    //Serial.println(ns);
    (FileName + String(NumCharArray) + ".txt").toCharArray(FileNameC, 13);
    while(SD.exists(FileNameC)) {
      FileNum += 1;
      sprintf(NumCharArray, "%05d", FileNum);
      (FileName + String(NumCharArray) + ".txt").toCharArray(FileNameC, 13);
    }
    LogTimeDate = getTime();
    Serial.print(LogTimeDate);
    Serial.print(" FileNameC: ");
    Serial.println(FileNameC);
  	String InitData = " SN = " + System.deviceID();  //Make string of onboard characteristics
  	logStr(InitData); //Log as first line of data
  	// logStr("Drink. Drink. Drink. Drink. Don't Think. Drive. Kill. Get drunk a lot. And work 40 hours a week. Drink. Drink. Drink. Drink. Don't Think. Drive. Kill. Get drunk a lot. And work 40 hours a week. "); //DEBUG!
    logStr("Time [UTC], Bat [V], " + Header); //Log concatonated header (for new loggers)
}

int TestLib::logStr(String val)
{
	// SD.begin(SD_CS); //DEBUG!
	// SD.chdir("/"); //Return to root to define starting state
	SD.chdir("Particle");  //Move into northern widget folder from root
	SD.chdir(SN);  //Move into specific numbered sub folder
	SD.chdir("Logs"); //Move into the logs sub-folder
    File DataFile = SD.open(FileNameC, FILE_WRITE);
 	
    if (DataFile) {
		DataFile.println(val);
        DataFile.close();
        Log.info("Logged data in %s", FileNameC); //DEBUG!
	    return 1;
	}
	// if the file isn't open, pop up an error:
	else {
        Log.warn("Print Error");
	    return 0;
	}
}

// void TestLib::run(String (*update)(void), unsigned long logInterval) //Pass in function which returns string of data; This is NOT IN USE
// {
// 	Serial.println("BANG!"); //DEBUG!
// 	// Serial.println(millis()); //DEBUG!
// 	if(NewLog) {
// 		Serial.println("Log Started!"); //DEBUG
// 		// LogEvent = true;
// 		// unsigned long TempLogInterval = logInterval; //ANDY, Fix with addition of function??
// 		delay(logInterval*1000); //DEBUG!
// 		initLogFile(); //Start a new file each time log button is pressed

// 		//Add inital data point
// 		addDataPoint(*update);
// 		NewLog = false;  //Clear flag once log is started
//     	blinkGood();  //Alert user to start of log
// 	}

// 	if(LogEvent) {
// 		Serial.println("Log!"); //DEBUG!
// 		// RTC.setAlarm(logInterval);  //Set/reset alarm //DEBUG!
// 		addDataPoint(*update); //Write values to SD
// 		LogEvent = false; //Clear log flag
// 		//Serial.println("BANG!"); //DEBUG!
// 	}

// 	AwakeCount++;
//     delay(1);

//     // //NEED TO DEBUG SLEEP
// 	// if(AwakeCount > 5) {
// 	// //    AwakeCount = 0;
// 	// 	// Serial.println(millis()); //DEBUG!
// 	// 	// sleepNow();
// 	// }
// }