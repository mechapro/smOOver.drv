#ifndef SMOOVER_H_
#define SMOOVER_H_

// Arduino APIs
#include "Arduino.h"
#include <SPI.h>

// standard libaries
#include <stdbool.h>
#include <stdint.h>

class SmOOver
{
	SPISettings spisettings;

	uint8_t pin_cs_sreg;
	uint8_t pin_step;
	uint8_t pin_enbl;
	uint8_t pin_chipselect;

public:
	enum Register {
		CTRL    = 0x00,
		TORQUE  = 0x01,
		OFF     = 0x02,
		BLANK   = 0x03,
		DECAY   = 0x04,
		STALL   = 0x05,
		DRIVE   = 0x06,
		STATUS  = 0x07,
	};

	// CS_SREG and STEP pins are required to indicate SPI mode to the smOOver.drv board on startup
	// you need to call SPI.begin() to initialize SPI in general
	SmOOver(int pin_cs_sreg, int pin_step, int pin_enbl, int pin_chipselect = 10);

	void enable_current(bool enable);

	// current in Amperes
	float get_current();
	float set_current(float desired = 0.0f);

	// configure 1..256 microsteps
	int get_microsteps();
	int set_microsteps(int desired = 0);

	// get raw register value
	uint16_t get_register(uint8_t address);
	// use this function with care!
	uint16_t set_register(uint8_t address, uint16_t value);
};

#endif
