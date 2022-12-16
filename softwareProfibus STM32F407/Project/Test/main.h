#ifndef __MAIN_H
#define __MAIN_H

#define PI 3.14159265359
#define RAD 0.01745329251

#define DATA_BUFFER_SIZE 10000

typedef union{
  float f;
  uint8_t i[4];
} f_to_i_t;

typedef union{
  uint32_t int32;
  uint8_t i[4];
} i_to_i_t;

typedef union{
  uint16_t int16;
  uint8_t int8[2];
} union_int16;



void Delay_100mcs(uint32_t nTime);
void assert_failed(uint8_t* file, uint32_t line);

void generateRandomBuffer();
uint8_t bufferIsEmpy();
void setBufferEmpty();
void resetBufferEmpty();
#endif