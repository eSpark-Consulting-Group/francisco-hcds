#pragma once

#include "Arduino.h"
#include "Wire.h"

// Constant macros
#define FAILED 0
#define SUCCESS 1
#define TRUE 1
#define FALSE 0

#define DOUT_HX711  32
#define CLK_HX711  33

extern "C"
{
	// Public functions
	
	// HX711 functions
	uint8_t initHX711(uint32_t DOUT = DOUT_HX711, uint32_t CLK = CLK_HX711);
	uint8_t getValue(double* value);
	uint8_t tare(uint8_t times);
}
