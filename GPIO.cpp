/******      GPIO      ******/
#include "GPIO.h"

uint16_t geophone_buf[2][GEOPHONE_BUF_SIZE];	// ADC data buffer, double buffered
unsigned int geophone_buf_id_current = 0;	// Which data buffer is being used for the ADC (the other is being sent)
unsigned int geophone_buf_pos = 0;	// Position (index) in the ADC data buffer
uint16_t geophone_buf_num_sent = 0;
bool geophone_buf_got_full = false;	// Flag to signal that a buffer is ready to be sent
bool printGeophoneDebug = false;
bool haveSetTimer1 = false;


/***************************************************/
/******            SETUP FUNCTIONS            ******/
/***************************************************/

void setupIOpins() {	// Setup all IO pins, including on-board pins and MCP23017
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);	// Turn on LED to show we're turning on...
	analogWriteRange(PWM_RANGE);	// Set the PWM range (0 to PWM_RANGE) so we can blink "heartbeat" LED
	spiBegin();		// Start SPI (so we can read ADC samples)
	setupTimer1();	// And start the timer to sample ADC
}

void setupTimer1() {
	timer1_isr_init();
	timer1_attachInterrupt(sample_isr);
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
	timer1_write(clockCyclesPerMicrosecond() / 16 / 2 * 100); //100us = 10kHz sampling freq
}

static inline void setDataBits(uint16_t bits) {
    const uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));
    bits--;
    SPI1U1 = ((SPI1U1 & mask) | ((bits << SPILMOSI) | (bits << SPILMISO)));
}

void spiBegin() {
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV8); 
  SPI.setHwCs(1);
  setDataBits(16);
}

/* SPI code based on the SPI library */
static inline ICACHE_RAM_ATTR uint16_t transfer16() {
	union {
		uint16_t val;
		struct {
			uint8_t lsb;
			uint8_t msb;
		};
	} out;


	// Transfer 16 bits at once, leaving HW CS low for the whole 16 bits 
	while(SPI1CMD & SPIBUSY) {}
	SPI1W0 = 0;
	SPI1CMD |= SPIBUSY;
	while(SPI1CMD & SPIBUSY) {}

	/* Follow MCP3201's datasheet: return value looks like this:
	xxxBA987 65432101
	We want 
	76543210 0000BA98
	So swap the bytes, select 12 bits starting at bit 1, and shift right by one.
	*/

	out.val = SPI1W0 & 0xFFFF;
	uint8_t tmp = out.msb;
	out.msb = out.lsb;
	out.lsb = tmp;

	out.val &= (0x0FFF << 1);
	out.val >>= 1;
	return out.val;
}


/**********************************************/
/******      GPIO related functions      ******/
/**********************************************/
void ICACHE_RAM_ATTR sample_isr() {
	// Read a sample from ADC and write it to the buffer
	uint16_t val = transfer16();
	geophone_buf[geophone_buf_id_current][geophone_buf_pos] = val;
	geophone_buf_pos++;

	// If the buffer is full, switch to the other one and signal that it's ready to be sent
	if (geophone_buf_pos > sizeof(geophone_buf[0])/sizeof(geophone_buf[0][0])) {
		geophone_buf_pos = 0;
		geophone_buf_id_current = !geophone_buf_id_current;
		geophone_buf_got_full = true;
	}
}

void processGPIO() {	// "GPIO.loop()" function: reads inputs, processes them and writes outputs
	// Blink built-in LED to show we're alive :)
	//analogWrite(LED_BUILTIN, (((curr_time>>10) & 0x01)? curr_time:~curr_time) & 0x3FF);	// Fading heartbeat every ~1s [for binary heartbeat use instead: digitalWrite(LED_BUILTIN, (curr_time>>10) & 0x01);]
	digitalWrite(LED_BUILTIN, (curr_time>>10) & 0x01);	// Analog write uses CPU to fake a PWM -> Because we use high CPU it doesn't work -> Use traditional "binary" toggle
	
	// Read inputs (geophone)
	static uint32_t t_sample = micros();
	if (printGeophoneDebug) {
		consolePrintF("@t=%8d ms -> Latest ADC sample:%4d\n", millis(), geophone_buf[geophone_buf_id_current][geophone_buf_pos]);
	}

	if (!haveSetTimer1) {
		haveSetTimer1 = true;
		setupTimer1();	// For some reason, WiFi or something else resets timer1 settings, so we gotta keep setting it up...
	}
}

