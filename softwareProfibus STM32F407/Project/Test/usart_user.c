#include "usart_user.h"
#include "profibus4.h"
#include "stm32f4xx_dma.h"
#include "spi_user.h"
#include "spi_user.h"
#include "main.h"

uint8_t uart_buffer[BUFFER_SIZE],uart_bufferTX[BUFFER_SIZE];
uint16_t rx_index=0, tx_index = 0,tx_counter=0;

extern uint8_t profibus_status;
extern uint8_t uart1_rx_buf[100];
extern uint8_t reqBuffer[10][8];

void EXTILine_Config(void)
{
  EXTI_InitTypeDef   EXTI_InitStructure;
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOA clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  /* Enable SYSCFG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* Connect EXTI Line0 to PA0 pin */
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource6);

  /* Configure EXTI Line0 */
  EXTI_InitStructure.EXTI_Line = EXTI_Line6;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  
  NVIC_SetPriority (EXTI9_5_IRQn, 0);
  NVIC_EnableIRQ (EXTI9_5_IRQn);
  
}

int InitUSART2(uint32_t baud) { 
    
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); 
  
  GPIO_InitTypeDef      GPIO_InitStructureUSART;
  GPIO_InitStructureUSART.GPIO_Pin = GPIO_Pin_5;
  GPIO_InitStructureUSART.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructureUSART.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructureUSART.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructureUSART.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructureUSART);
  
  GPIO_InitStructureUSART.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructureUSART.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructureUSART.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init(GPIOD, &GPIO_InitStructureUSART);
  
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_USART2); //PC10 to TX USART2
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_USART2); //PC11 to RX USART2
    
  USART_OneBitMethodCmd(USART2,ENABLE);
  USART_OverSampling8Cmd(USART2,ENABLE);
  
  USART_InitTypeDef USART_InitStructureUSART;  
  USART_InitStructureUSART.USART_BaudRate = baudSpeed[baud];
  USART_InitStructureUSART.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructureUSART.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_InitStructureUSART.USART_Parity = USART_Parity_Even;
  USART_InitStructureUSART.USART_StopBits = USART_StopBits_1;
  USART_InitStructureUSART.USART_WordLength = USART_WordLength_9b;
  USART_Init(USART2, &USART_InitStructureUSART);

  
  /* Configure PD4 as rs485 tx select           */
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  
  NVIC_SetPriority (USART2_IRQn, 0);
  NVIC_EnableIRQ (USART2_IRQn);

    
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
  USART_Cmd(USART2, ENABLE);    
  
  GPIO_SetBits(GPIOD,GPIO_Pin_3);                                               //На плате не подтянут
  
  return 0; 
   
}

void USART2_put_char(uint8_t c) {
  while (tx_counter == BUFFER_SIZE);                                            //если буфер переполнен, ждем
  USART_ITConfig(USART2, USART_IT_TXE, DISABLE);                                 //запрещаем прерывание, чтобы оно не мешало менять переменную
  
  if (tx_counter || (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET))     //если в буфере уже что-то есть или если в данный момент что-то уже передается
  {
    uart_bufferTX[tx_index++] = c;                                              //то кладем данные в отдельный буфер отправки 
    if (tx_index == BUFFER_SIZE) tx_index=0;                                    //идем по кругу
    ++tx_counter;                                                               //увеличиваем счетчик количества данных в буфере
  }
  else                                                                          //если UART свободен
    USART_SendData(USART2, c);                                                  //передаем данные без прерывания
  
  USART_ITConfig(USART2, USART_IT_TXE, ENABLE);                                  //разрешаем прерывание
}

void USART2_put_string_2(unsigned char *string, uint32_t l) {
  TX485EN;
  tx_index=0;                                                                   //Передаём cначала
  tx_counter=l;//это ЭЛЬ! А не единица
  while (l != 0){
    USART2_put_char(*string++);
    l--;
  }
}

void USART2_put_string(unsigned char *string, uint32_t l){
  TX485EN;
  memcpy(uart_bufferTX,string+1,l-1);
  tx_counter=l-1;
  USART_SendData(USART2, *string);
  USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}

void uart_process(uint8_t byte){
 
  uart_buffer[rx_index] = byte;

  TIM3->CNT = 0;
  GPIOD->ODR ^= GPIO_Pin_15;
  // TSYN истек, находимся в режиме ожидания данных. Приём первого байта
  if (profibus_status == PROFIBUS_WAIT_DATA){   
    profibus_status = PROFIBUS_GET_DATA;                                        //Меняем статус на ПРОСЕСС ПОЛУЧЕНИЯ ДАННЫХ
  }
  
  if (profibus_status == PROFIBUS_GET_DATA)  {
    rx_index++;
    if(rx_index==BUFFER_SIZE) rx_index=0;
  }

}
void uart_error(){
  spiSendByte(rx_index);
}
//modbus
void  InitUSART1(void){
  
  GPIO_InitTypeDef      GPIO_InitStructureUSART;
  USART_InitTypeDef USART_InitStructureUSART; 
  DMA_InitTypeDef DMA_InitStructure;
    
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); 
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
  
 GPIO_InitStructureUSART.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructureUSART.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructureUSART.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructureUSART.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructureUSART.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructureUSART);
  
  GPIO_InitStructureUSART.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructureUSART.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructureUSART.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init(GPIOA, &GPIO_InitStructureUSART);
  
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1); 
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1); 
  
  USART_OneBitMethodCmd(USART1,ENABLE);
  USART_OverSampling8Cmd(USART1,ENABLE);
 
  USART_InitStructureUSART.USART_BaudRate = 115200;
  USART_InitStructureUSART.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructureUSART.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_InitStructureUSART.USART_Parity = USART_Parity_No;
  USART_InitStructureUSART.USART_StopBits = USART_StopBits_1;
  USART_InitStructureUSART.USART_WordLength = USART_WordLength_8b;
  USART_Init(USART1, &USART_InitStructureUSART);

  USART_Cmd(USART1, ENABLE);   
  USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
  USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
  //TX DMA
  DMA_DeInit(DMA2_Stream7);
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);           //Адрес регистра данных USART:
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&reqBuffer[0][0];             //адрес нулевого элемента массива:
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;                       //Шлем в периферию, а не из нее:
  DMA_InitStructure.DMA_BufferSize = BUFFER1_SIZE;                                        //Размер буфера – 16 байт
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;              //В периферии не инкрементируем
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                       //в памяти – инкрементируем
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;       //Пересылаем данные байтами:
  DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;           //Пересылаем данные байтами:
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                               //
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream7, &DMA_InitStructure);
  
  NVIC_SetPriority (DMA2_Stream7_IRQn, 0);
  NVIC_EnableIRQ(DMA2_Stream7_IRQn);
  DMA_ITConfig(DMA2_Stream7,DMA_IT_TC,ENABLE);
  
  //DMA_Cmd(DMA2_Stream7, ENABLE);
 
  //RX DMA
  DMA_DeInit(DMA2_Stream5);
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);           //Адрес регистра данных USART:
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&uart1_rx_buf[0];           //адрес нулевого элемента массива:
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;                       //Шлем в периферию, а не из нее:
  DMA_InitStructure.DMA_BufferSize = 100;                                       //Размер буфера – 16 байт
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;              //В периферии не инкрементируем
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                       //в памяти – инкрементируем
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;       //Пересылаем данные байтами:
  DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;           //Пересылаем данные байтами:
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                               //
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream5, &DMA_InitStructure);
  
  
  NVIC_SetPriority (DMA2_Stream5_IRQn, 5);
  NVIC_EnableIRQ(DMA2_Stream5_IRQn);
  DMA_ITConfig(DMA2_Stream5,DMA_IT_TC,ENABLE);
  DMA_ClearFlag(DMA2_Stream5, DMA_IT_TCIF5);
  //DMA_Cmd(DMA2_Stream5, ENABLE);
  
}
