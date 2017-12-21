/******      GPIO      ******/
#ifndef GPIO_H_
#define GPIO_H_

#include "main.h"				// HotTub global includes and definitions
#include <ESPAsyncWebServer.h>	// HTTP web server to handle requests to turn on/off lights, sound, etc.
#include <SPI.h>

#define GEOPHONE_BUF_SIZE	1000
#define PWM_RANGE			1023

extern uint16_t geophone_buf[2][GEOPHONE_BUF_SIZE];	// ADC data buffer, double buffered
extern unsigned int geophone_buf_id_current;	// Which data buffer is being used for the ADC (the other is being sent)
extern unsigned int geophone_buf_pos;	// Position (index) in the ADC data buffer
extern uint16_t geophone_buf_num_sent;
extern bool geophone_buf_got_full;	// Flag to signal that a buffer is ready to be sent


/***************************************************/
/******            SETUP FUNCTIONS            ******/
/***************************************************/
void setupIOpins();			// Setup all IO pins
void setupTimer1();
static inline void setDataBits(uint16_t bits);
void spiBegin();
static inline ICACHE_RAM_ATTR uint16_t transfer16();


/**********************************************/
/******      GPIO related functions      ******/
/**********************************************/
void ICACHE_RAM_ATTR sample_isr();
void processGPIO();								// "GPIO.loop()" function: reads inputs, processes them and writes outputs

/**** HTTP way to change settings (START) ****/
extern bool lightsOn, soundOn;
void turnReplyHtml(uint8_t turnWhat, bool state, AsyncWebServerRequest *request);
void turnSound(bool on, AsyncWebServerRequest* request=NULL);
/**** HTTP way to change settings (END) ****/

#endif

