/******      Web server config      ******/
#include "webServer.h"

bool shouldReboot = false;
bool sendBinDataAsString = false;
bool sendFFT = true;
char hostName[32];
AsyncWebServer webServer(PORT_WEBSERVER);
#if USE_NATIVE_WEBSOCKET_LIB
AsyncWebSocket wsGeophone("/geophone"), wsFFT("/fft"), wsConsole("/console");
#else
WebSocketsServer wsGeophone(PORT_WEBSOCKET_GEOPHONE), wsFFT(PORT_WEBSOCKET_FFT), wsConsole(PORT_WEBSOCKET_CONSOLE);
#endif


/***************************************************/
/******            SETUP FUNCTIONS            ******/
/***************************************************/
void setupWebServer() {	// Initializes hostName, mDNS, HTTP server, OTA methods (HTTP-based and IDE-based) and webSockets
	// Initialize file system, so we can read/write config and web files
	if (!SPIFFS.begin()) {
		consolePrintF("Failed to mount file system!!\n");
	} else {
		consolePrintF("SPIFFS loaded!\n");
	}

	// Initialize hostName
	if (UNIQUE_HOSTNAME) {
		sprintf(hostName, "Geophone-%06X", ESP.getChipId());
	} else {
		sprintf(hostName, "GeophoneDuino");
	}

	// Setup mDNS so we don't need to know its IP
	#if USE_MDNS
		consolePrintF("Starting mDNS... ");
		if (MDNS.begin(hostName)) {
			consolePrintF("Started! you can now also contact me through 'http://%s.local'\n", hostName);
			MDNS.addService(F("http"), F("tcp"), PORT_WEBSERVER);
			MDNS.addService(F("ws"), F("tcp"), PORT_WEBSOCKET_GEOPHONE);
			MDNS.addService(F("ws"), F("tcp"), PORT_WEBSOCKET_CONSOLE);
		} else {
			consolePrintF("Unable to start mDNS :(\n");
		}
	#endif

	// Start servers and websockets
	webServer.rewrite("/WiFi", "/WiFi.html");
	webServer.on(SF("/WiFiNets").c_str(), HTTP_GET, webServerWLANscan);
	webServer.on(SF("/WiFiSave").c_str(), HTTP_POST, webServerWLANsave);
	webServer.on(SF("/heap").c_str(), HTTP_GET, [](AsyncWebServerRequest* request) { AsyncWebServerResponse* response = request->beginResponse(200, CONT(TYPE_PLAIN), String(ESP.getFreeHeap()) + F(" B")); addNoCacheHeaders(response); response->addHeader(F("Refresh"), F("2")); request->send(response); });
	webServer.on(SF("/restart").c_str(), HTTP_GET, [](AsyncWebServerRequest* request) { AsyncWebServerResponse* response = request->beginResponse(200, contentType_P[TYPE_PLAIN], F("Restarting!")); addNoCacheHeaders(response); response->addHeader(F("Refresh"), F("15; url=/heap")); request->send(response); shouldReboot = true; });

	webServer.serveStatic("/", SPIFFS, "/www/", "public, max-age=1209600").setDefaultFile("index.html");	// Cache for 2 weeks :)

	webServer.on("/OTA", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(200, CONT(TYPE_HTML), F("<form method='POST' action='/OTA' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>"));
	});
	webServer.on("/OTA", HTTP_POST, [](AsyncWebServerRequest* request) {
		shouldReboot = !Update.hasError();
		AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot? "Succssfully got new firmware! Restarting...":"Something went wrong uploading the firmware, sorry try again :(");
		addNoCacheHeaders(response);
		response->addHeader(F("Refresh"), F("15; url=/heap"));
		request->send(response);
	}, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
		if (!index) {
			consolePrintF("\n\t---> OTA update start! %s\n", filename.c_str());
			//Update.runAsync(true);
			if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
				Update.printError(Serial);
			}
		}
		if (!Update.hasError()) {
			if (Update.write(data, len) != len) {
				Update.printError(Serial);
			}
		}
		if (final) {
			if (Update.end(true)) {
				consolePrintF("\n\t---> Successful OTA upload: %uB!\n", index+len);
			} else {
				Update.printError(Serial);
			}
		}
	});

	webServer.on(SF("/upload").c_str(), HTTP_GET, [](AsyncWebServerRequest* request){ if (SPIFFS.exists(F("/upload.html"))) { request->send(SPIFFS, F("/upload.html")); } else { request->send(200, contentType_P[TYPE_HTML], F("<html><head><link rel=\"stylesheet\" href=\"css/styles.css\"></head><body><h1>Secret file uploader</h1><form method=\"POST\" action=\"upload\" enctype=\"multipart/form-data\"><p>New file name: <input type=\"text\" placeholder=\"/\" name=\"fileName\" value=\"\" /><br><input type=\"file\" name=\"fileContent\" value=\"\" /><br><input type=\"submit\" value=\"Upload file\" /></p></form></body></html>")); } });
	webServer.on(SF("/upload").c_str(), HTTP_POST, [](AsyncWebServerRequest* request){ bool ok = renameFileUpload(request->arg(F("fileName"))); request->redirect(SF("upload?f=") + request->arg(F("fileName")) + F("&ok=") + ok); }, handleFileUpload);
	webServer.addHandler(new SPIFFSEditor(WEB_FILE_EDITOR_USERNAME, WEB_FILE_EDITOR_PASS));

	webServer.onNotFound([](AsyncWebServerRequest* request) { request->send(404, CONT(TYPE_PLAIN), SF("Not found: ") + request->url()); });

	#if USE_NATIVE_WEBSOCKET_LIB
		wsGeophone.onEvent(onWsEvent);
		webServer.addHandler(&wsGeophone);
		wsConsole.onEvent(onWsEvent);
		webServer.addHandler(&wsConsole);
		#if DO_FFT
			wsFFT.onEvent(onWsEvent);
			webServer.addHandler(&wsFFT);
		#endif
	#else
		wsGeophone.begin();
		wsGeophone.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t len) { onWsEvent(&wsGeophone, num, type, payload, len); });
		wsConsole.begin();
		wsConsole.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t len) { onWsEvent(&wsConsole, num, type, payload, len); });
		#if DO_FFT
			wsFFT.begin();
			wsFFT.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t len) { onWsEvent(&wsFFT, num, type, payload, len); });
		#endif
	#endif

	webServer.begin();

	// Also, setup Arduino's OTA (only works from their IDE)
	#if USE_ARDUINO_OTA
		ArduinoOTA.setHostname(hostName);
		ArduinoOTA.setPort(PORT_ARDUINO_OTA);
		ArduinoOTA.setPassword(WEB_FILE_EDITOR_PASS);
		ArduinoOTA.onStart([]() {
			consolePrintF("OTA: Start!\n");
			// Fade in and out the builtin led
			for(int i=PWM_RANGE; i>0; i--) { analogWrite(LED_BUILTIN, i); delay(1); }
			for(int i=0; i<=PWM_RANGE; i++) { analogWrite(LED_BUILTIN, i); delay(1); }
			pinMode(LED_BUILTIN, OUTPUT);	// ArduinoOTA blinks the LED_BUILTIN on progress, but digitalWrite doesn't work well after analogWrite. Solution is to reset the pin as output
		});
		ArduinoOTA.onEnd([]() {
			consolePrintF("OTA: Firmware update succeeded!\n");
			for (int i=0; i<30; i++) { analogWrite(LED_BUILTIN, (i*100) % 1001); delay(50); }
		});
		ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
			consolePrintF("OTA: %3u%% completed (%4u KB de %4u KB)\r", progress / (total/100), progress>>10, total>>10);
			// analogWrite(LED_BUILTIN, int((total-progress) / (total/PWM_RANGE)));	// Recuerda que el builtin led es active low -> Luz apagada (0%) quiere decir escribir PWM_RANGE; Luz encendida (100%) -> Escribir 0
		});
		ArduinoOTA.onError([](ota_error_t error) {
			consolePrintF("\nOTA: Error[%u]: ", error);
			if (error == OTA_AUTH_ERROR) consolePrintF("Authentication failed\n");
			else if (error == OTA_BEGIN_ERROR) consolePrintF("Begin failed\n");
			else if (error == OTA_CONNECT_ERROR) consolePrintF("Connection failed\n");
			else if (error == OTA_RECEIVE_ERROR) consolePrintF("Reception failed\n");
			else if (error == OTA_END_ERROR) consolePrintF("End failed\n");
	
			consolePrintF("Rebooting Arduino...\n");
			ESP.restart();
		});
		ArduinoOTA.begin();
	#endif
}


/*********************************************************/
/******      	Web server related functions      	******/
/*********************************************************/
void addNoCacheHeaders(AsyncWebServerResponse* response) {	// Add specific headers to an http response to avoid caching
	response->addHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	response->addHeader(F("Pragma"), F("no-cache"));
	response->addHeader(F("Expires"), F("-1"));
}

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {	// Allows user to upload a file to the SPIFFS (so we don't have to write the whole Flash via USB)
	static File fUpload;

	if (!index) {
		consolePrintF("\nUploading new SPIFFS file (to the temporary path %s) with filename %s\n", UPLOAD_TEMP_FILENAME, filename.c_str());
		fUpload = SPIFFS.open(UPLOAD_TEMP_FILENAME, "w");
	}

	if (fUpload)
		fUpload.write(data, len);
	if (final) {
		if (fUpload)
			fUpload.close();
		consolePrintF("Successfully uploaded new SPIFFS file with filename %s (%u B)!\n", filename.c_str(), index+len);
	}
}

bool renameFileUpload(String fileName) {	// Renames the last file uploaded to the new file name provided
	if (!fileName.startsWith("/"))
		fileName = "/" + fileName;
	if (SPIFFS.exists(fileName)) {
		SPIFFS.remove(fileName);
		consolePrintF("\t(File already existed, removing old version -> Overwriting)\n");
	}

	bool r = SPIFFS.rename(UPLOAD_TEMP_FILENAME, fileName);
	consolePrintF("%s temporary file %s -> %s\n\n", (r? SF("Successfully renamed"):SF("Couldn't rename")).c_str(), UPLOAD_TEMP_FILENAME, fileName.c_str());

	return r;
}

void webServerWLANscan(AsyncWebServerRequest* request) {	// Handles secret HTTP page that scans WLAN networks
	String json = SF("{\"currAP\":{\"ssid\":\"") + SOFT_AP_SSID + F("\",\"ip\":\"") + WiFi.softAPIP().toString() + F("\"},"
		"\"currWLAN\":{\"ssid\":\"") + String(wlanSSID) + F("\",\"pass\":\"") + String(wlanPass) + F("\",\"ip\":\"") + wlanMyIP.toString() + F("\",\"gateway\":\"") + wlanGateway.toString() + F("\",\"mask\":\"") + wlanMask.toString() + F("\"},"
		"\"nets\":[");
	int n = WiFi.scanComplete();

	if (n == -2) {
		WiFi.scanNetworks(true);
	} else {
		for (int i=0; i<n; ++i) {
			if (i) json += ",";
			json += SF("{"
				"\"rssi\":") + String(WiFi.RSSI(i)) + F(","
				"\"ssid\":\"") + WiFi.SSID(i) + F("\","
				"\"bssid\":\"") + WiFi.BSSIDstr(i) + F("\","
				"\"channel\":") + String(WiFi.channel(i)) + F(","
				"\"secure\":") + String(WiFi.encryptionType(i)) + F(","
				"\"hidden\":") + String(WiFi.isHidden(i)) + F(
			"}");
		}
		WiFi.scanDelete();
		if(WiFi.scanComplete() == -2){
			WiFi.scanNetworks(true);
		}
	}

	json += "]}";
	AsyncWebServerResponse* response = request->beginResponse(200, CONT(TYPE_JSON), json);
	addNoCacheHeaders(response);
	request->send(response);
	json = String();
}

void webServerWLANsave(AsyncWebServerRequest* request) {	// Handles secret HTTP page that saves new WLAN settings
	consolePrintF("Received request to save new WLAN settings!\n");
	bool bSSIDmanual = false;
	if (request->hasArg(F("ssidManualChk"))) bSSIDmanual = (request->arg(SF("ssidManualChk"))=="on");
	if (request->hasArg(bSSIDmanual? CF("ssidManualTxt"):CF("ssidDropdown"))) request->arg(bSSIDmanual? SF("ssidManualTxt"):SF("ssidDropdown")).toCharArray(wlanSSID, sizeof(wlanSSID)-1);
	if (request->hasArg(F("pass"))) request->arg(SF("pass")).toCharArray(wlanPass, sizeof(wlanPass)-1);
	if (request->hasArg(F("ip"))) wlanMyIP.fromString(request->arg(SF("ip")));
	if (request->hasArg(F("gateway"))) wlanGateway.fromString(request->arg(SF("gateway")));
	if (request->hasArg(F("mask"))) wlanMask.fromString(request->arg(SF("mask")));
	saveWLANconfig();	// Write new settings to EEPROM

	AsyncWebServerResponse* response = request->beginResponse(200, contentType_P[TYPE_HTML], SF("<html><head><link rel=\"stylesheet\" href=\"css/styles.css\"></head><body><h1>WiFi config successfully saved!</h1><p>SSID: ") + wlanSSID + F("<br>IP: ") + wlanMyIP.toString() + F("<br>Gateway: ") + wlanGateway.toString() + F("<br>Mask: ") + wlanMask.toString() + F("</p></body></html>"));
	addNoCacheHeaders(response);
	request->send(response);
}

#if USE_NATIVE_WEBSOCKET_LIB
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t * data, size_t len) {	// webSocket event callback function
	switch (type) {
	case  WS_EVT_CONNECT:
		consolePrintF("[WebSocket '%s'] Client connected from %s:%d (clientId %u)\n", server->url(), client->remoteIP().toString().c_str(), client->remotePort(), client->id());
		client->text(SOFT_AP_SSID);	// send message to client to confirm connection ok
		break;
	case WS_EVT_ERROR:
    	consolePrintf("[WebSocket '%s'] ClientId %u error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
    	break;
    default:
    	break;
	}
}
#else
void onWsEvent(WebSocketsServer* ws, uint8_t num, WStype_t type, uint8_t * payload, size_t len) {	// webSocket event callback function
	switch(type) {
	case WStype_CONNECTED:
		consolePrintF("[WebSocket '%s'] Client connected from %s (clientID %u)\n", payload, ws->remoteIP(num).toString().c_str(), num);
		ws->broadcastTXT(SOFT_AP_SSID);	// send message to client to confirm connection ok
		break;
	case WStype_ERROR:
		consolePrintF("[WebSocket] ClientID %u error: %s\n", num, payload);
		break;
	case WStype_DISCONNECTED:
	case WStype_TEXT:
	case WStype_BIN:
	default:
		break;
	}
}
#endif

void consolePrintf(const char * format, ...) {	// Log messages through webSocketConsole and Serial
	char buf[1024];
	va_list args;
    va_start(args, format);
    size_t len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    #if USE_NATIVE_WEBSOCKET_LIB
    	wsConsole.textAll(buf, len);
    #else
    	wsConsole.broadcastTXT(buf, len);
    #endif
	Serial.printf(buf);
}

void processWebServer() {	// "webServer.loop()" function: handle incoming OTA connections (if any), http requests and webSocket events
	uint32_t t_msec = curr_time%1000, t_sec = curr_time/1000, t_min = t_sec/60, t_hr = t_min/60; t_sec %= 60; t_min %= 60;
	static uint32_t last_t_sec = 0;
	if (t_sec != last_t_sec) {	// Every second, log that we are alive
		last_t_sec = t_sec;
		consolePrintF("Still alive (t=%3d:%02d'%02d\"); HEAP: %5d B\n", t_hr, t_min, t_sec, ESP.getFreeHeap());
	}

	if (geophone_buf_got_full) {
		geophone_buf_got_full = false;	// Remember to reset this flag so we only send when the next buffer is full ;)
		unsigned int buf_id = !geophone_buf_id_current;	// Use the *opposite* buffer id of the one being filled currently (so we send the one that's already full)

		if (sendBinDataAsString) {
			String strWebSocket = "[" + String(geophone_buf[buf_id][0]);
			for (unsigned int i=1; i<GEOPHONE_BUF_SIZE; ++i) {
				strWebSocket += "," + String(geophone_buf[buf_id][i]);
			}
			strWebSocket += "]";

			#if USE_NATIVE_WEBSOCKET_LIB
				wsGeophone.textAll(strWebSocket);
			#else
				wsGeophone.broadcastTXT(strWebSocket);
			#endif
			strWebSocket = String();
		} else {
			#if USE_NATIVE_WEBSOCKET_LIB
				wsGeophone.binaryAll(reinterpret_cast<uint8_t*>(geophone_buf[buf_id]), sizeof(geophone_buf[0]));
			#else
				wsGeophone.broadcastBIN(reinterpret_cast<uint8_t*>(geophone_buf[buf_id]), sizeof(geophone_buf[0]));
			#endif
		}

		#if DO_FFT
			performFFT(buf_id);
	
			if (sendBinDataAsString) {
				String strWebSocket = "[" + String(fft_real[0]);
				for (uint16_t i=1; i<=N_FFT/2; ++i) {
					strWebSocket += "," + String(fft_real[i]);
				}
				strWebSocket += "]";

				#if USE_NATIVE_WEBSOCKET_LIB
					wsFFT.textAll(strWebSocket);
				#else
					wsFFT.broadcastTXT(strWebSocket);
				#endif
				strWebSocket = String();
			} else {
				#if USE_NATIVE_WEBSOCKET_LIB
					wsFFT.binaryAll(reinterpret_cast<uint8_t*>(fft_real), sizeof(fft_real[0])*(1 + N_FFT/2));	// FFT is symmetrical, no need to send the 2nd half of the buffer, only [0 - N_FFT/2])
				#else
					wsFFT.broadcastBIN(reinterpret_cast<uint8_t*>(fft_real), sizeof(fft_real[0])*(1 + N_FFT/2));	// FFT is symmetrical, no need to send the 2nd half of the buffer, only [0 - N_FFT/2])
				#endif
			}
		#endif
	}
	#if !USE_NATIVE_WEBSOCKET_LIB && WEBSOCKETS_NETWORK_TYPE!=NETWORK_ESP8266_ASYNC
		wsFFT.loop();
		wsGeophone.loop();
		wsConsole.loop();
	#endif

	if (shouldReboot) ESP.restart();	// AsyncWebServer doesn't suggest rebooting from async callbacks, so we set a flag and reboot from here :)
	
	#if USE_ARDUINO_OTA
		ArduinoOTA.handle();
	#endif
}

