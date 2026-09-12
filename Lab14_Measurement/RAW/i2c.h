#ifndef I2C_H
#define I2C_H

#include <stdint.h>


void I2C1_Init(void);


void I2C1_Send(uint8_t addr7, const uint8_t *buf, uint32_t len);


void I2C1_SendFrag(uint8_t addr7, uint8_t pre, const uint8_t* data, uint32_t len);


#endif
