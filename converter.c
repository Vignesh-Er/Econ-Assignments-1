#include "header_tmp.c"

#define TMP102_RESOLUTION_C 0.0625f

float tmp102_raw_to_celsius(uint16_t reg)
{
	/* Cast to signed 16-bit integer and perform arithmetic right shift by 4 */
	int16_t count = (int16_t)reg >> 4;

	return (float)count * TMP102_RESOLUTION_C;
}

uint16_t tmp102_celsius_to_raw(float temp_c)
{
	int16_t count;

	if (temp_c >= 0.0f) {
		count = (int16_t)((temp_c / TMP102_RESOLUTION_C) + 0.5f);
	} else {
		count = (int16_t)((temp_c / TMP102_RESOLUTION_C) - 0.5f);
	}

	return (uint16_t)((count & 0x0FFF) << 4);
}

float tmp102_raw_to_celsius_extended(uint16_t reg)
{
	int16_t count = (int16_t)reg >> 3;

	return (float)count * TMP102_RESOLUTION_C;
}

uint16_t tmp102_celsius_to_raw_extended(float temp_c)
{
	int16_t count;

	if (temp_c >= 0.0f) {
		count = (int16_t)((temp_c / TMP102_RESOLUTION_C) + 0.5f);
	} else {
		count = (int16_t)((temp_c / TMP102_RESOLUTION_C) - 0.5f);
	}

	return (uint16_t)(((count & 0x1FFF) << 3) | 0x0001);
}