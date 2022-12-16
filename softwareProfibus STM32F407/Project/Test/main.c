﻿#include <stdio.h>
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
    
    timers_init(1000);
    LEDInit();
    init_Profibus ();
    InitUSART2();
    //I2C_EE_Init();
    
    while (1){  
      //profibus_TX(_buf,10);
      //delay_1_ms(1000);
    }   
}



#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line){
    while (1){  
    }
}
#endif
