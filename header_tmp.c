#ifndef TMP102_H
#define TMP102_H

#include <stdint.h>

/* I2C Target Addresses (7-bit) */
#define TMP102_ADDR_GND             0x48u /* ADD0 connected to GND */
#define TMP102_ADDR_VCC             0x49u /* ADD0 connected to V+  */
#define TMP102_ADDR_SDA             0x4Au /* ADD0 connected to SDA */
#define TMP102_ADDR_SCL             0x4Bu /* ADD0 connected to SCL */
#define TMP102_ADDR_ALERT_RESPONSE  0x0Cu /* SMBus Alert Response Address */
#define TMP102_ADDR_GENERAL_CALL    0x00u /* General Call Address */

/* Pointer Registers */
#define TMP102_REG_TEMP             0x00u /* Temperature Register (Read-only) */
#define TMP102_REG_CONFIG           0x01u /* Configuration Register (Read/Write) */
#define TMP102_REG_TLOW             0x02u /* T_LOW Limit Register (Read/Write) */
#define TMP102_REG_THIGH            0x03u /* T_HIGH Limit Register (Read/Write) */

/*Default Temperature Values*/
#define TMP102_TLOW_DEFAULT         0x4B00u    /* 75.0 deg C */
#define TMP102_THIGH_DEFAULT        0x5000u    /* 80.0 deg C */

/* Conversion Functions */
float    tmp102_raw_to_celsius(uint16_t reg);
uint16_t tmp102_celsius_to_raw(float temp_c);
float    tmp102_raw_to_celsius_extended(uint16_t reg);
uint16_t tmp102_celsius_to_raw_extended(float temp_c);

#endif /* TMP102_H */