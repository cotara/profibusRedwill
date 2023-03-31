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
uint8_t dataBuffer[10][8] = {{0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00},              //Стартовый запрос
                            {0x00, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00},               //ИД 3
                            {0x00, 0x06, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00},               //ИД 6(F0)
                            {0x00, 0x06, 0x00, 0xf1, 0x00, 0x00, 0x00, 0x00},               //ИД 6(F1)
                            {0x00, 0x06, 0x00, 0xf2, 0x00, 0x00, 0x00, 0x00},               //ИД 6(F2)
                            {0x00, 0x03, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00},               //ЗАСИ 3
                            {0x00, 0x06, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00},               //ЗАСИ 6(A)
                            {0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00},               //ЗАСИ 6(10)
                            {0x00, 0x06, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00},               //ЗАСИ 6(C)
                            {0x00, 0x03, 0x00, 0x10, 0x00, 0x13, 0x00, 0x00},               //LDM 3
};
uint8_t uart1_tx_buf[100],uart1_rx_buf[100];
uint8_t dataInBuffer[100];
uint16_t devModel=0;
uint 8_t ansSize[10]={0,16+5,34+5,26+5,
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
    //uint8_t _buf[10] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A};
    unsigned short CRC16 = 0;
    
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(SystemCoreClock/1000);
    LEDInit();
    InitUSART1();
    
    //Расчёт CRC для всех запросов
    for(int i=0;i<10;i++){
        CRC16 = crc16(&dataBuffer[i][0],10);
        dataBuffer[i][6] = CRC16&0xFF;  //CRC L
        dataBuffer[i][7] = CRC16>>8;     //CRC H
    }
    
    memcpy(uart1_tx_buf,&dataBuffer[0][0],8);
    DMA_Cmd(DMA2_Stream7, ENABLE);                                              //Отправляем нулевой запрос на установление модели прибора
    DMA2_Stream5->NDTR = DMA_InitStruct->DMA_BufferSize;                        //Сколько байт ждать
    DMA_Cmd(DMA2_Stream5, ENABLE);                                              //Запускаем ожидание на приём

    while (devModel==0){}
    
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

