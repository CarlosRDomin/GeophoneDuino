/******      WiFi connectivity      ******/
#ifndef WIFI_H_
#define WIFI_H_

#include "main.h"						// HotTub global includes and definitions
#include <EEPROM.h>						// EEPROM is used to store WLAN configuration (SSID, pass, IP, etc.)
#include <IPAddress.h>					// IPAddresses are used for WLAN configuration

#define SOFT_AP_IP			IPAddress(192, 168, 0, 1)
#define SOFT_AP_MASK		IPAddress(255, 255, 255, 0)
#define SOFT_AP_PASS		"geophone"
#define WLAN_CONFIG_OK_STR	"Ok"
#define WIFI_T_GIVE_UP		1500	// (ms) If createWiFiAP is false but can't connect to any known network, we'll create our own AP and try to connect to known networks again after WIFI_T_RECONNECT ms

extern char SOFT_AP_SSID[32], wlanSSID[32], wlanPass[32];
extern IPAddress wlanMyIP, wlanGateway, wlanMask;


/***************************************************/
/******            SETUP FUNCTIONS            ******/
/***************************************************/
void setupWiFi();		// "Publicly-visible" initialization routine for WiFi connection. Calls internal _setupWiFi passing the value of createWiFiAP
void setupAP();			// Configures softAP
void connectAP();		// Enables softAP
void connectToWLAN();	// Connect to saved WLAN network


/**********************************************/
/******      WiFi related functions      ******/
/**********************************************/
void loadDefaultWiFiConfig();	// Loads default WLAN credentials (if couldn't load them from the EEPROM)
void loadWLANConfig();			// Load WLAN credentials from EEPROM
void saveWLANconfig();			// Save WLAN credentials to EEPROM
void processWiFi();				// "WiFi.loop()" function: tries to reconnect to known networks if haven't been able to do so for the past WIFI_T_RECONNECT ms and createWiFiAP == false

#endif

