#include <stdio.h>
#include <stdlib.h>
#include "includes.h"
#include "time_user.h"
#include "usart_user.h"
#include "LED_user.h"

#include "profibus4.h"


__IO uint32_t TimeOut = 0x00;

RCC_ClocksTypeDef RCC_Clocks;
extern  uint8_t uart_buffer[BUFFER_SIZE];
extern  uint16_t rx_index, tx_index,tx_counter;

/********************************************************
* MAIN
********************************************************/

void main(void) {
  uint8_t _buf[10] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A};
    
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(SystemCoreClock/1000);
    
    //timers_init(1000);
    LEDInit();
    //timers_init((uint32_t)(2));
    init_Profibus ();
    InitUSART2();
    
    while (1){  

    }   
}

