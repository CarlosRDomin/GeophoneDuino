/******      HotTub main file      ******/
#include "main.h"
#include "GPIO.h"
#include "WiFi.h"
#include "webServer.h"

uint32_t curr_time;


/***************************************************/
/******            SETUP FUNCTIONS            ******/
/***************************************************/
void setup() {
	curr_time = millis();
	Serial.begin(115200);
	Serial.setDebugOutput(true);	// Print debug messages through Serial (for debugging)
	while(!Serial);	// Wait for serial port to connect

	setupWiFi();
	setupWebServer();
	setupIOpins();
}


/*********************************************/
/******            MAIN LOOP            ******/
/*********************************************/
void loop() {
	curr_time = millis();
	processGPIO();
	processWiFi();
	processWebServer();
	delay(20);
}

