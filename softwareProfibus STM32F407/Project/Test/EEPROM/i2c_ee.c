#include "i2c_ee.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_rcc.h"
#include "time_user.h"

//#define I2C_FLASH_PAGESIZE     32 //not used yet
//PAGE0 - 0xA0 ***  PAGE1 - 0xA2 *** PAGE2 - 0xA4 *** PAGE3 - 0xA6
#define EEPROM_HW_ADDRESS      0xA0   /* E0 = E1 = E2 = 0 0x50<<1*/

void Delay_ms(uint32_t ms);
uint16_t eeWrIndex=0;
uint8_t eePage = 0;
/***************************************************************************//**
 *  @brief  I2C Configuration
 ******************************************************************************/
void I2C_EE_Init(void)
{
   	   I2C_InitTypeDef  I2C_InitStruct;
	   GPIO_InitTypeDef  GPIO_InitStructure;

	   RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);

           RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
           
          // настройка I2C
          I2C_InitStruct.I2C_ClockSpeed = 100000; 
          I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
          I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
          // адрес отфанарный
          I2C_InitStruct.I2C_OwnAddress1 = 0x01;
          I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;
          I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
          
            I2C_Init(I2C1, &I2C_InitStruct);          
          // I2C использует две ноги микроконтроллера, их тоже нужно настроить
          GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
          GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;			
          GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
          GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
          GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
          GPIO_Init(GPIOB, &GPIO_InitStructure);
 
          GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_I2C1);
          GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);
 
          // стартуем модуль I2C
          I2C_Cmd(I2C1, ENABLE);      
}

//*******************************************************************8
//***************************************************************

void I2C_EE_ByteWrite(uint8_t val, uint8_t WriteAddr)
{
    /* Send START condition */
    I2C_GenerateSTART(I2C1, ENABLE);

    /* Test on EV5 and clear it */
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    /* Send EEPROM address for write */
    I2C_Send7bitAddress(I2C1, EEPROM_HW_ADDRESS + eePage, I2C_Direction_Transmitter);

    /* Test on EV6 and clear it */
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));


    /* Send the EEPROM's internal address to write to : MSB of the address first */
    //I2C_SendData(I2C1, (uint8_t)((WriteAddr & 0xFF00) >> 8));

    /* Test on EV8 and clear it */
    //while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    /* Send the EEPROM's internal address to write to : LSB of the address */
    //I2C_SendData(I2C1, (uint8_t)(WriteAddr & 0x00FF));
      I2C_SendData(I2C1, WriteAddr);
    /* Test on EV8 and clear it */
    while(! I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

     I2C_SendData(I2C1, val);

        /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    /* Send STOP condition */
    I2C_GenerateSTOP(I2C1, ENABLE);

    //delay between write and read...not less 4ms
    delay_1_ms(2);
}
//*********************************************************************************
uint8_t I2C_EE_ByteRead( uint8_t ReadAddr)
{
    uint8_t tmp;

	/* While the bus is busy */
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    /* Send START condition */
    I2C_GenerateSTART(I2C1, ENABLE);

    /* Test on EV5 and clear it */
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    /* Send EEPROM address for write */
    I2C_Send7bitAddress(I2C1, EEPROM_HW_ADDRESS + eePage, I2C_Direction_Transmitter);

    /* Test on EV6 and clear it */
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));


    /* Send the EEPROM's internal address to read from: MSB of the address first */
    //I2C_SendData(I2C1, (uint8_t)((ReadAddr & 0xFF00) >> 8));

    /* Test on EV8 and clear it */
    //while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    /* Send the EEPROM's internal address to read from: LSB of the address */
    //I2C_SendData(I2C1, (uint8_t)(ReadAddr & 0x00FF));
    I2C_SendData(I2C1, ReadAddr);
    /* Test on EV8 and clear it */
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));


    /* Send STRAT condition a second time */
    I2C_GenerateSTART(I2C1, ENABLE);

    /* Test on EV5 and clear it */
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    /* Send EEPROM address for read */
    I2C_Send7bitAddress(I2C1, EEPROM_HW_ADDRESS + eePage, I2C_Direction_Receiver);

    /* Test on EV6 and clear it */
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_BYTE_RECEIVED));//I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    I2C_AcknowledgeConfig(I2C1, DISABLE);
    tmp=I2C_ReceiveData(I2C1);

    /* Send STOP Condition */
    I2C_GenerateSTOP(I2C1, ENABLE);

    return tmp;
}

uint8_t setToEE(uint16_t val){
  uint8_t status;
  uint8_t msb,lsb;
  uint8_t temp1,temp2;
  
  lsb=(uint8_t)(val & 0x00FF);
  msb=(uint8_t)((val & 0xFF00)>> 8);
  I2C_EE_ByteWrite(msb,eeWrIndex++);
  I2C_EE_ByteWrite(lsb,eeWrIndex++);
  
  temp1=I2C_EE_ByteRead(eeWrIndex-2);
  temp2=I2C_EE_ByteRead(eeWrIndex-1);
 
  if( temp1==msb && temp2==lsb)
         status=0;
  else{ 
         status=1;
         eeWrIndex-=2;
  }
       
  if (eeWrIndex==TEMPER_MAX_BUF){                                             //Конец страницы
    eeWrIndex=0;
    eePage+=2;                                                                  //Переход на следующую страницу
    if(eePage>6)  eePage=0;
  }
  
  return status;
}


void getFromEE(uint16_t *buf,uint16_t len){
  uint16_t i;;
  uint8_t msb,lsb;
  int16_t pos;
  if(eeWrIndex!=0)                                                              //Мы не в начале страницы
    pos=eeWrIndex-1;                                             
  else{                                                                         //Мы были в начале страницы
    pos=TEMPER_MAX_BUF-1;                                                
    eePage-=2;
    if(eePage>6)                                                                //Если это была последняя страница
      eePage=6;
  }    
    for(i=len;i>0;i--){
      lsb=I2C_EE_ByteRead(pos--);
      msb=I2C_EE_ByteRead(pos--);
      buf[i-1] = ((uint16_t)msb << 8) + lsb;
      if(pos < 0){                                                              //Дочитали до конца страницы
        pos=TEMPER_MAX_BUF-1;                                                
        eePage-=2;
        if(eePage>6)                                                                //Если это была последняя страница
        eePage=6;                                                            //Переключаемся на нулевую
      }
    } 
}
uint16_t getEEWrIndex(){
  return eeWrIndex;
}

void detEEPage(uint8_t p){
  eePage=p;
}
uint8_t getEEPage(){
  return eePage;
}