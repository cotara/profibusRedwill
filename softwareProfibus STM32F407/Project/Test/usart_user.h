#include "includes.h"
#include <stdio.h>
//#include "main.h"
#define BUFFER_SIZE 100 
#define TX485EN GPIO_SetBits(GPIOD, GPIO_Pin_7)    
#define RX485EN GPIO_ResetBits(GPIOD, GPIO_Pin_7)   
void USART2_put_string_2(unsigned char *string, uint32_t l);
int InitUSART2();
void USART2_put_char(uint8_t c);
void uart_process(uint8_t byte);