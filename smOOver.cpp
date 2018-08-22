#include "smOOver.h"

static int8_t ilog2(uint16_t number)
{
  int8_t p = -1;
  while (number)
    p += 1, number >>= 1;
  return p;
}

float const TORQUE_magic = (sqrt(2) * 256 * 5 * 0.04) / 2.75;

SmOOver::SmOOver(int pin_cs_sreg, int pin_step, int pin_enbl, int pin_chipselect)
	: spisettings(1000000, MSBFIRST, SPI_MODE3),
	  pin_cs_sreg(pin_cs_sreg),
	  pin_step(pin_step),
	  pin_enbl (pin_enbl),
	  pin_chipselect (pin_chipselect)
{
	// startup mode selection: SPI slave (2)
	pinMode(pin_cs_sreg, OUTPUT);
	digitalWrite(pin_cs_sreg, LOW);

	pinMode(pin_step, OUTPUT);
	digitalWrite(pin_step, HIGH);

	// power to motor disabled
	pinMode(pin_enbl, OUTPUT);
	digitalWrite(pin_enbl, LOW);

	// preparing SPI
	pinMode(pin_chipselect, OUTPUT);
	digitalWrite(pin_chipselect, HIGH);
}

uint16_t SmOOver::get_register(uint8_t address)
{
	uint16_t result = 0;

	SPI.beginTransaction(spisettings);
	digitalWrite(pin_chipselect, LOW);

	SPI.transfer((address & 0x0F) | 0x80); // READ bit set
	delayMicroseconds(200); // plenty of time to load data
	result |= SPI.transfer(0x00) << 8;
	delayMicroseconds(200);
	result |= SPI.transfer(0x00);

	digitalWrite(pin_chipselect, HIGH);
	SPI.endTransaction();

	return result;
}

uint16_t SmOOver::set_register(uint8_t address, uint16_t value)
{
	uint16_t result = 0;

	SPI.beginTransaction(spisettings);
	digitalWrite(pin_chipselect, LOW);

	SPI.transfer(address & 0x0F); // no READ bit
	delayMicroseconds(200); // plenty of time to load data
	result |= SPI.transfer((value >> 8) & 0xFF) << 8;
	delayMicroseconds(200);
	result |= SPI.transfer(value & 0xFF);

	digitalWrite(pin_chipselect, HIGH);
	delayMicroseconds(200);

	SPI.endTransaction();

	return result;
}

void SmOOver::enable_current(bool enable)
{
	digitalWrite(pin_enbl, enable ? HIGH : LOW);
}

float SmOOver::get_current()
{
	// bits 0-7 in TORQUE register
	// read back
	uint16_t regval = get_register(Register::TORQUE);
	return (regval & 0xFF) / TORQUE_magic;
}

float SmOOver::set_current(float desired)
{
	// bits 0-7 in TORQUE register

	// acceptable range: up to 4.0 Ampere
	if (desired > 0.0f && desired <= 4.0f)
	{
		// read, modify, write
		uint8_t rawval = desired * TORQUE_magic + 0.5; // +0.5 for rounding

		uint16_t regval = get_register(Register::TORQUE);
		regval &= ~0xFF;
		regval |= rawval & 0xFF;
		set_register(Register::TORQUE, regval);
	}

	return get_current();
}

int SmOOver::get_microsteps()
{
	// bits 3-6 in CTRL register
	// read back
	uint16_t regval = get_register(Register::CTRL);
	int val = ((regval >> 3) & 0x0F);

	if (val >= 0 && val <= 8)
		return 1 << val;
	else
		return -1;
}

int SmOOver::set_microsteps(int desired)
{
	// bits 3-6 in CTRL register

	if (desired > 0 && desired <= 256)
	{
		// read, modify, write
		uint16_t regval = get_register(Register::CTRL);
		regval &= ~(0b1111 << 3);
		regval |= (ilog2(desired) & 0xF) << 3;
		set_register(Register::CTRL, regval);
	}

	return get_microsteps();
}


