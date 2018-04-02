/******      WiFi connectivity      ******/
#include "WiFi.h"

char SOFT_AP_SSID[32], wlanSSID[32], wlanPass[32];
IPAddress wlanMyIP, wlanGateway, wlanMask;
const String strWlanConfigOk(WLAN_CONFIG_OK_STR);
uint32_t tNextWiFiReconnectAttempt = -1;
uint8_t giveUpOnWLAN = 0;	// If we haven't been able to connect to the saved WiFi network after WIFI_T_GIVE_UP, give up trying and create our own AP


/***************************************************/
/******            SETUP FUNCTIONS            ******/
/***************************************************/
void setupWiFi() {	// "Publicly-visible" initialization routine for WiFi connection. Calls internal _setupWiFi passing the value of createWiFiAP
	setupAP();
	loadWLANConfig();	// Load settings from EEPROM like which network we want to connect to
	connectToWLAN();	// And then try to connect to it
}

void setupAP() {	// Configures softAP
	sprintf(SOFT_AP_SSID, "Geophone_%06X", ESP.getChipId());
	WiFi.softAP(SOFT_AP_SSID, SOFT_AP_PASS);
	WiFi.softAPConfig(SOFT_AP_IP, SOFT_AP_IP, SOFT_AP_MASK);
	WiFi.enableAP(false);
	//WiFi.setPhyMode(WIFI_PHY_MODE_11B);
}

void connectAP() {	// Enables softAP
	WiFi.enableSTA(false);
	WiFi.enableAP(true);
	giveUpOnWLAN = 2;
	consolePrintF("\nWiFi AP setup as '%s', IP is %s\n", SOFT_AP_SSID, WiFi.softAPIP().toString().c_str());
}

void connectToWLAN() {	// Connect to saved WLAN network
	consolePrintF("Trying to connect to WLAN '%s' with IP %s\n", wlanSSID, wlanMyIP.toString().c_str());
	WiFi.disconnect();
	WiFi.config(wlanMyIP, wlanGateway, wlanMask);
	WiFi.begin (wlanSSID, wlanPass);
	uint8_t connRes = WiFi.waitForConnectResult();
	
	if (WiFi.isConnected()) {
		consolePrintF("WiFi successfully connected to '%s' with IP %s!\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
	} else {
		consolePrintF("Couldn't connect to '%s' (WiFi status %d) =(\n", wlanSSID, connRes);
		giveUpOnWLAN++;
		if (giveUpOnWLAN >= 2) connectAP();	// Create our own AP (hotspot) so we can wireless control the Arduino
	}
	tNextWiFiReconnectAttempt = curr_time + WIFI_T_GIVE_UP;	// Regardless of whether we were able to successfully connect to the WLAN, don't try to reconnect for WIFI_T_RECONNECT ms
}


/**********************************************/
/******      WiFi related functions      ******/
/**********************************************/
void loadDefaultWiFiConfig() {	// Loads default WLAN credentials (if couldn't load them from the EEPROM)
	sprintf_P(wlanSSID, PSTR("PEILab"));
	sprintf_P(wlanPass, PSTR("just4now"));
	/*wlanMyIP	= IPAddress( 10,  0,  0,100);
	wlanGateway	= IPAddress( 10,  0,  0,  1);*/
	wlanMyIP	= IPAddress(192,168, 43,100);
	wlanGateway	= IPAddress(192,168, 43,  1);
	wlanMask	= IPAddress(255,255,255,  0);
}

void loadWLANConfig() {	// Load WLAN credentials from EEPROM
	uint16_t memStart = 0;
	char ok[2+1];
	
	EEPROM.begin(512);
	EEPROM.get(memStart, wlanSSID);
	memStart += sizeof(wlanSSID);
	EEPROM.get(memStart, wlanPass);
	memStart += sizeof(wlanPass);
	EEPROM.get(memStart, wlanMyIP);
	memStart += sizeof(wlanMyIP);
	EEPROM.get(memStart, wlanGateway);
	memStart += sizeof(wlanGateway);
	EEPROM.get(memStart, wlanMask);
	memStart += sizeof(wlanMask);
	EEPROM.get(memStart, ok);
	EEPROM.end();
	
	if (String(ok) != strWlanConfigOk) {
		loadDefaultWiFiConfig();
	}
	consolePrintF("Recovered WLAN credentials:\n\tSSID: %s\n\tPass: %s\n\tIP: %s\n\tGateway: %s\n\tMask: %s\n", strlen(wlanSSID)>0? wlanSSID:SF("<No SSID>").c_str(), strlen(wlanPass)>0? wlanPass:SF("<No password>").c_str(), wlanMyIP.toString().c_str(), wlanGateway.toString().c_str(), wlanMask.toString().c_str());
}

void saveWLANconfig() {	// Save WLAN credentials to EEPROM
	uint16_t memStart = 0;
	char ok[2+1] = WLAN_CONFIG_OK_STR;
	
	EEPROM.begin(512);
	EEPROM.put(memStart, wlanSSID);
	memStart += sizeof(wlanSSID);
	EEPROM.put(memStart, wlanPass);
	memStart += sizeof(wlanPass);
	EEPROM.put(memStart, wlanMyIP);
	memStart += sizeof(wlanMyIP);
	EEPROM.put(memStart, wlanGateway);
	memStart += sizeof(wlanGateway);
	EEPROM.put(memStart, wlanMask);
	memStart += sizeof(wlanMask);
	EEPROM.put(memStart, ok);
	EEPROM.commit();
	EEPROM.end();

	giveUpOnWLAN = 0;	// Force a reconnect on next loop iteration
}

void processWiFi() {	// "WiFi.loop()" function: tries to reconnect to known networks if haven't been able to do so for the past WIFI_T_RECONNECT ms and createWiFiAP == false
	if (!WiFi.isConnected() && curr_time>tNextWiFiReconnectAttempt && giveUpOnWLAN<2) {
		connectToWLAN();// If we haven't been able to successfully connect to the WLAN, retry after tNextWiFiReconnectAttempt
	}
}

