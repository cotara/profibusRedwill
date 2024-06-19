﻿#include <stdio.h>
#include <stdlib.h>
#include "includes.h"
#include "time_user.h"
#include "usart_user.h"
#include "LED_user.h"

#include "profibus4.h"
#include "spi_user.h"
#include "stm32f4xx_dma.h"

extern  uint8_t uart_buffer[BUFFER_SIZE];                                       //Буфер профибаса
extern  uint16_t rx_index, tx_index,tx_counter;                                 //иттераторы юарта профибаса
uint8_t baudNum=-1;
uint8_t buadOK=0;
//****************************************************************************//
//                            КАРТА РЕГИСТРОВ                                 //
//****************************************************************************//
//*********************************ИД*****************************************//
///ВХОДЯЩИЕ (6 функция) 12 байт                                               //
//      0xF0 -> ЧТО_ТО                                                        //
//      0xF1 -> ЧТО_ТО                                                        //
//      0xF2 -> ЧТО_ТО                                                        //
//      Стартовая позиция в массиве 0                                         //
//ИСХОДЯЩИЕ (3-я функция) 16 байт                                             //
//      Стартовая позиция в массиве [0]                                       //
//****************************************************************************//
//*********************************ЗАСИ***************************************//
///ВХОДЯЩИЕ (6 функция) 12 байт                                               //
//      0x0A -> ЧТО_ТО                                                        //
//      0x10 -> ЧТО_ТО                                                        //
//      0x0C -> ЧТО_ТО                                                        //
//      Стартовая позиция в массиве 24                                        //
///ИСХОДЯЩИЕ (3-я функция) 18 байт                                            //
//      Стартовая позиция в массиве [16]                                      //
//****************************************************************************//
//*********************************ЛДМ****************************************//
///ВХОДЯЩИЕ (6 функция) 0 байт                                                //
//      -                                                                     //
///ИСХОДЯЩИЕ (3-я функция) 21 байт                                            //
//      Стартовая позиция в массиве [36]                                      //
//****************************************************************************//
uint8_t reqBuffer[10][8] = {{0xF9, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00},              //Стартовый запрос
                            {0x00, 0x03, 0x10, 0x00, 0x00, 0x08, 0x00, 0x00},               //ИД 3
                            {0x00, 0x06, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00},               //ИД 6(F0)
                            {0x00, 0x06, 0x00, 0xf1, 0x00, 0x00, 0x00, 0x00},               //ИД 6(F1)
                            {0x00, 0x06, 0x00, 0xf2, 0x00, 0x00, 0x00, 0x00},               //ИД 6(F2)
                            {0x00, 0x03, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00},               //ЗАСИ 3
                            {0x00, 0x06, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00},               //ЗАСИ 6(A)
                            {0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00},               //ЗАСИ 6(10)
                            {0x00, 0x06, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00},               //ЗАСИ 6(C)
                            {0x00, 0x03, 0x00, 0x10, 0x00, 0x0E, 0x00, 0x00},               //LDM 3
};

uint8_t deviceDataBuffer[5][50] = {{0}};                                                //Буфер с данными от девайсов с запасом на 5 модульков
//uint8_t *idDataBuffer;
//uint8_t *zasiDataBuffer;
//uint8_t *ldmDataBuffer;

uint8_t uart1_rx_buf[100];                                                      //Буфер общения с мастер-контроллером
uint16_t devModel=0;
uint8_t ansSize[10]={4+5,16+5,8,8,8,34+5,8,8,8,28+5};
unsigned short CRC16 = 0;
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
void setModbusAddres(uint8_t add){
   for(int i=0;i<10;i++){
     reqBuffer[i][0]=add;
   }
       //Расчёт CRC для всех запросов
   for(int i=0;i<10;i++){
        CRC16 = crc16(&reqBuffer[i][0],6);
        reqBuffer[i][6] = CRC16&0xFF;  //CRC L
        reqBuffer[i][7] = CRC16>>8;     //CRC H
   }
}
/********************************************************
* MAIN
********************************************************/
void main(void) {
     
  
    
    
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(SystemCoreClock/1000);
    
    delay_1_ms(5000);
    
    
    LEDInit();

    //Расчёт CRC для всех запросов
    for(int i=0;i<10;i++){
        CRC16 = crc16(&reqBuffer[i][0],6);
        reqBuffer[i][6] = CRC16&0xFF;  //CRC L
        reqBuffer[i][7] = CRC16>>8;     //CRC H
    }
    
    
    timer5_init();
    
    //Автоопределение скорости
    while(!buadOK){
      baudNum++;
      if(baudNum>7) baudNum=0;
      InitUSART2(baudNum);
      init_Profibus (baudNum);
      delay_1_ms(100);
      
    }
    //SPI_Config();
    
    while (1){    }   
}