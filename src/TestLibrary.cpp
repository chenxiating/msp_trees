# include "TestLibrary.h"       // Quotes - same folder, angle bracket - wider scope

TestLib* TestLib::selfPointer;

TestLib::TestLib(bool displayMsg) {
    // Anything you need when instantiatiating your object goes here
    // Pin definition
    SD_CS = 5;

}

// Pretend this is one or more complext and involved functions you have written

// Public Functions
// --------------------------------------------
int TestLib::begin(String header_) {
    Log.info("Beginning logger initialization");
    RTCsetup(); // ONLY USE DURING INITIAL SETUP!
    rtc.begin();
    
    String id = System.deviceID();
    SN = id.c_str();

    Log.info("\n\n\nInitializing...\n");
    delay(100);
    getTime();
    Log.info("\nTimestamp = "+LogTimeDate);

    Header = header_;
    Log.info(Header);
    if (Header.substring(0, 2) == "TC") {
        TCsetup();
    } else {
        Serial.println("Set up for soil moisture");
        // RGsetup();
    }

    // //Sets up basic initialization required for the system
	selfPointer = this;

    SDsetup();
    clockTest();
    // SdFile::dateTimeCallback(dateTimeSD); //Setup SD file time setting

    batTest();
    blinkGood();
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
    bool SDErrorTemp = false;
	// bool SD_Test = true;

    // SD_CS is pulled up: HIGH=1 if not present
    // SD card being inserted closes a switch to pull it LOW
	pinMode(SD_CS, INPUT);

	Serial.print("SD: ");
	delay(5); //DEBUG!
	if (!SD.begin(SD_CS)) {
        Log.warn(" NO CARD");
  	    OBError = true;
  	    SDErrorTemp = true;
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
		      SDErrorTemp = true;
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
    Serial.println("In TC Setup");
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

// void TestLib::RGsetup() {
//     pinMode(intPin, INPUT); // DEBUG!
//     attachInterrupt(digitalPinToInterrupt(intPin), TestLib::RGtip, FALLING);
//     pinMode(intPin, INPUT_PULLUP);
// }

// void TestLib::logSoil(int logMinutes, int logTimes, String (*update)(void)){
//     unsigned long currentMillis = millis();
//     int logInterval = logMinutes * 1000 * 60;
//   if(currentMillis - previousMillis > logInterval) {
//     // save the last time you logged data
//     previousMillis = currentMillis;   
    
//     // turn on relay pin
//     digitalWrite(soilRelayPin, HIGH);
//     String rain;
//     String soil;
//     soil = getSoilvoltage();
//     Log.info("Soil: ");
//     Serial.println(soil);
//     rain = getRain();
//     Log.info("Rain: ");
//     Serial.println(rain);
//     return soil+","+rain;
//     if (ledState == LOW)
//       ledState = HIGH;
//     else
//       ledState = LOW;

//     // set the LED with the ledState of the variable:
//     digitalWrite(ledPin, ledState);
//   }
// }


void TestLib::heating(int val){
    pinMode(heatPin, OUTPUT);
    analogWrite(heatPin, val);
    Serial.print(getTime());
    Serial.println(" Heating begins and will continue for 10 min");
    delay(10*60*1000); // Heating for 10 minutes
}

void TestLib::batTest()
{
	if(getBatVoltage() < BatVoltageError) BatError = true; //Set error flag if below min voltage
	// if(getBatPer() < BatPercentageWarning) BatWarning = true; //Set warning flag is below set percentage
	Serial.print("Bat = ");
	Serial.print(getBatVoltage());
	Serial.print("V\t");
	// Serial.print(getBatPer());
	// Serial.println("%");
}

void TestLib::blinkGood() {
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

void TestLib::addDataPoint(String (*update)(void)) //Reads new data and writes data to SD
{
	String data = "";
	data = (*update)(); //Run external update function
	// pinMode(BlueLED, OUTPUT);
	// digitalWrite(BlueLED, LOW); //ON
	data = getOnBoardVals() + data; //Prepend on board readings
	// Serial.println("Got OB Vals");  //DEBUG!
	logStr(data);
    Serial.println(data);
	Serial.println("Logged Data"); //DEBUG!
}

String TestLib::getOnBoardVals() 
{   
    float BatVoltage = getBatVoltage(); //Get battery voltage, Include voltage divider in math
    LogTimeDate = getTime();
    return LogTimeDate + "," + String(BatVoltage) + ",";
}

float TestLib::getBatVoltage()
{
	float BatVoltage = analogRead(BATT) * 0.0011224;
	return BatVoltage;
}



double TestLib::getTCvoltage() 
{
    Serial.print("ADC: "); Serial.print(mcp.readADC() * 2); Serial.println(" uV");
    if (!(mcp.readThermocouple() == NAN)) {
        TCvoltage = mcp.readADC() * 2;
    }
    return TCvoltage;
}

void TestLib::Soilsetup(){
    pinMode(soilRelayPin, OUTPUT);
    delay(5*1000); // Wait 5 seconds for the excitation power
}

String TestLib::getSoilvoltage() 
{
    double topVolt = getVoltage(topPinIn);
    double midVolt = getVoltage(midPinIn); 
    double botVolt = getVoltage(botPinIn);
    return String(topVolt) + "," + String(midVolt) + "," + String(botVolt);
}

double TestLib::getVoltage(int pinInput)
{
    pinMode(pinInput, INPUT);
    double volt = analogRead(pinInput) * (3.3 / 4096); 
    return volt;
}

// // NEED TO TEST getBatPer()
// float TestLib::getBatPer()
// {
// 	//NOTE: Fit developed for Duracell AA, should work well for most alkalines, but no gaurentee given on accuracy
// 	//From 305 to 100% capacity, should be accurate to within 1% (for data taken at 25C)
// 	float A = -1.9809;
// 	float B = 6.2931;
// 	float C = -4.0063;
// 	float val = getBatVoltage()/3.0; //Divide to get cell voltage
// 	float Per = ((A*pow(val, 2) + B*val + C)*2 - 1)*100.0; //Return percentage of remaining battery energy
// 	if(Per < 0) return 0;  //Do not allow return of non-sensical values
// 	if(Per > 100) return 100;  //Is this appropriate? Float voltage could be higher than specified and still be correct
// 	return Per;
// }

void TestLib::heatingoff(){
    analogWrite(heatPin,0);
}

void TestLib::Soiloff(){
    digitalWrite(soilRelayPin,LOW);
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

    Serial.println("Supposed to be in Logs");
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
    Serial.print("FileNameC: ");
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

	// if the file is available, write to it:
	if (DataFile) {
		DataFile.println(val);
        DataFile.close();
	    return 0;
	}
	// if the file isn't open, pop up an error:
	else {
        Serial.println("Print Error");
	    return -1;
	}

	// DataFile.close();
}

void TestLib::run(String (*update)(void), unsigned long logInterval) //Pass in function which returns string of data
{
	Serial.println("BANG!"); //DEBUG!
	// Serial.println(millis()); //DEBUG!
	if(NewLog) {
		Serial.println("Log Started!"); //DEBUG
		// LogEvent = true;
		// unsigned long TempLogInterval = logInterval; //ANDY, Fix with addition of function??
		delay(logInterval*1000); //DEBUG!
		initLogFile(); //Start a new file each time log button is pressed

		//Add inital data point
		addDataPoint(*update);
		NewLog = false;  //Clear flag once log is started
    	blinkGood();  //Alert user to start of log
	}

	if(LogEvent) {
		Serial.println("Log!"); //DEBUG!
		// RTC.setAlarm(logInterval);  //Set/reset alarm //DEBUG!
		addDataPoint(*update); //Write values to SD
		LogEvent = false; //Clear log flag
		//Serial.println("BANG!"); //DEBUG!
	}

	AwakeCount++;
    delay(1);

    // //NEED TO DEBUG SLEEP
	// if(AwakeCount > 5) {
	// //    AwakeCount = 0;
	// 	// Serial.println(millis()); //DEBUG!
	// 	// sleepNow();
	// }
}