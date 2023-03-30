#include <stdio.h>
#include <stdlib.h>
#include "includes.h"
#include "time_user.h"
#include "usart_user.h"
#include "LED_user.h"

#include "profibus4.h"
#include "spi_user.h"

__IO uint32_t TimeOut = 0x00;

RCC_ClocksTypeDef RCC_Clocks;
extern  uint8_t uart_buffer[BUFFER_SIZE];
extern  uint16_t rx_index, tx_index,tx_counter;
uint8_t baudNum=-1;
uint8_t buadOK=0;
uint8_t dataBuffer[16] = {0x01, 0x03, 0x00, 0x10, 0x00, 0x22};
uint8_t dataInBuffer[100];
/********************************************************
* MAIN
********************************************************/
unsigned short crc16(unsigned char *data_p, unsigned short len)
{
  unsigned short  crc = 0xFFFF;
 for (int i=0;i<len;i++)
 {
 crc=crc^data_p[i];
 for (int j=0;j<8;++j)
 if (crc&0x01) crc =(crc>>1)^0xA001;
 else crc=crc>>1;
 }
 return(crc);
 }
void main(void) {
    uint8_t _buf[10] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A};
    unsigned short CRC16 = 0;
    
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(SystemCoreClock/1000);
    CRC16 = crc16(dataBuffer,6);
    dataBuffer[6] = CRC16&0xFF;  //CRC L
    dataBuffer[7] = CRC16>>8;     //CRC H
   
    LEDInit();
    InitUSART1();
    timer5_init();

    GPIO_SetBits(GPIOD,GPIO_Pin_3);
     
    while(!buadOK){
      baudNum++;
      if(baudNum>7) baudNum=0;
      InitUSART2(baudNum);
      init_Profibus (baudNum);
      delay_1_ms(100);
      
    }

  
    
    SPI_Config();
    
    while (1){  
//        spiSendByte(0x1);
    //    delay_1_ms(100);
     //    GPIO_WriteBit(GPIOA,GPIO_Pin_9,(BitAction)(1 -GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_9) ))  ;
 
    }   
}

