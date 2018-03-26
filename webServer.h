/******      Web server config      ******/
#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_

#include "main.h"						// Global includes and definitions
#include "GPIO.h"						// GPIO library so HTTP server can change IO state upon request
#include "WiFi.h"						// WiFi library needed for secret settings (to detect which interface a client is connected on, WLAN or AP)
#include "FFT.h"						// FFT library in case we also want to stream the FFT of the ADC data
#include <ESPAsyncWebServer.h>			// HTTP web server
#include <ESP8266HTTPUpdateServer.h>	// OTA (upload firmware through HTTP browser over WiFi)
#include <FS.h>							// SPIFFS file system (to read/write to flash)
#include <SPIFFSEditor.h>				// Helper that provides the resources to view&edit SPIFFS files through HTTP
#include <WebSocketsServer.h>			// WebSockets

#define consolePrintF(s, ...)		consolePrintf(String(F(s)).c_str(), ##__VA_ARGS__)
#define CONT(x)						String(FPSTR(contentType_P[x]))
#define PORT_WEBSERVER				80		// Port for the webServer
#define PORT_WEBSOCKET_GEOPHONE		81		// Port for the webSocket for real-time geophone streaming purposes
#define PORT_WEBSOCKET_CONSOLE		82		// Port for the webSocket to which debug Serial.print messages are forwarded
#define PORT_WEBSOCKET_FFT			90		// Port for the webSocket for real-time geophone FFT streaming purposes
#define PORT_ARDUINO_OTA			8266	// Port recognized by Arduino IDE which allows remote firmware flashing
#define WEB_FILE_EDITOR_USERNAME	SF("PEILab")
#define WEB_FILE_EDITOR_PASS		SF("geophone")
#define USE_ARDUINO_OTA				false	// Whether or not to use Arduino's native IDE remote firmware flasher
#define USE_MDNS					false	// Whether or not to use mDNS (allows access to the arduino through a name without knowing its IP)
#define UNIQUE_HOSTNAME				true	// If true, use ESP.getChipId() to create a unique hostname; Otherwise, use "GeophoneDuino"
#define UPLOAD_TEMP_FILENAME		"/tmp.file"	// Temporary file name given to a file uploaded through the web server. Once we receive its desired path, we'll rename it (move it)

#if USE_ARDUINO_OTA
#include <ArduinoOTA.h>				// OTA (upload firmware through Arduino IDE over WiFi)
#endif

#if USE_MDNS
#include <ESP8266mDNS.h>			// DNS (allows access to the arduino through a name without knowing its IP)
#endif


enum {TYPE_PLAIN=0, TYPE_HTML, TYPE_JSON, TYPE_CSS, TYPE_JS, TYPE_PNG, TYPE_GIF, TYPE_JPG, TYPE_ICO, TYPE_XML, TYPE_PDF, TYPE_ZIP, TYPE_GZ, TYPE_DLOAD};
const char* const PROGMEM contentType_P[] = {"text/plain", "text/html", "text/json", "text/css", "application/javascript", "image/png", "image/gif", "image/jpeg", "image/x-icon", "text/xml", "application/x-pdf", "application/x-zip", "application/x-gzip", "application/octet-stream"};


/***************************************************/
/******            SETUP FUNCTIONS            ******/
/***************************************************/
void setupWebServer();	// Initializes hostName, mDNS, HTTP server, OTA methods (HTTP-based and IDE-based) and webSockets


/*********************************************************/
/******      	Web server related functions      	******/
/*********************************************************/
void addNoCacheHeaders(AsyncWebServerResponse* response);	// Add specific headers to an http response to avoid caching
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);	// Allows user to upload a file to the SPIFFS (so we don't have to write the whole Flash via USB)
bool renameFileUpload(String fileName);	// Renames the last file uploaded to the new file name provided
void webServerWLANscan(AsyncWebServerRequest* request);	// Handles secret HTTP page that scans WLAN networks
void webServerWLANsave(AsyncWebServerRequest* request);	// Handles secret HTTP page that saves new WLAN settings
void webSocketGeophoneEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght);	// webSocketGeophone event callback function
void webSocketConsoleEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght);	// webSocketConsole event callback function
void consolePrintf(const char * format, ...);	// Log messages through webSocketConsole and Serial
void processWebServer();	// "webServer.loop()" function: handle incoming OTA connections (if any), http requests and webSocket events

#endif

