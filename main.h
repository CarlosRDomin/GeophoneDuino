/******      HotTub main include      ******/
#ifndef MAIN_H_
#define MAIN_H_

#include <Arduino.h>	// Aruino general includes and definitions
#include "webServer.h"	// webServer library in case we want to print debug messages through a webSocket (consolePrintF method)

#define SF(literal)		String(F(literal))			// Macro to save a string literal in Flash memory and convert it to String when reading it
#define CF(literal)		String(F(literal)).c_str()	// Macro to save a string literal in Flash memory and convert it to char* when reading it
#define SFPSTR(progmem)	String(FPSTR(progmem))		// Macro to convert a PROGMEM string (in Flash memory) to String

extern uint32_t curr_time;

#endif

