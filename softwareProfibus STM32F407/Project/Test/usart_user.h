#ifndef USART_USER_H
#define USART_USER_H 

#include "includes.h"
#include <stdio.h>
//#include "main.h"
#define BUFFER_SIZE 100 
#define BUFFER1_SIZE 8
#define TX485EN GPIO_SetBits(GPIOD, GPIO_Pin_7)    
#define RX485EN GPIO_ResetBits(GPIOD, GPIO_Pin_7)   


static const uint32_t baudSpeed[8]={
  9600,
  19200,
  45450,
  93750,
  187500,
  500000,
  1500000,
  3000000
};

enum baudEnum{
  BAUD9_6=0,
  BAUD19_2=1,
  BAUD45_45=2,
  BAUD93_75=3,
  BAUD187_5=4,
  BAUD500=5,
  BAUD1500=6,
  BAUD3000=7
};

void USART2_put_string_2(unsigned char *string, uint32_t l);
void USART2_put_string(unsigned char *string, uint32_t l);
void USART1_put_string_2(unsigned char *string, uint32_t l);
void USART1_put_char(uint8_t c);
int InitUSART2();
void EXTILine_Config(void);
void DMA_RX_Reinit();
void USART2_put_char(uint8_t c);
void uart_process(uint8_t byte);
void InitUSART1(void);

#endif