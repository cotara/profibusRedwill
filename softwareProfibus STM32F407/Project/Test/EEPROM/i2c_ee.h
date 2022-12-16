#ifndef __I2CEE_H__
#define __I2CEE_H__

#include <stdint.h>
#define TEMPER_MAX_BUF 256
void I2C_EE_Init(void);
void I2C_EE_ByteWrite(uint8_t val, uint8_t WriteAddr);
uint8_t I2C_EE_ByteRead( uint8_t ReadAddr);
uint8_t setToEE(uint16_t val);
void getFromEE(uint16_t* buf,uint16_t len);
uint16_t getEEWrIndex();

#endif /* __I2CEE_H__ */