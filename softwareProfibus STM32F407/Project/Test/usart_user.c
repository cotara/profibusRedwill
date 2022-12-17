#include "usart_user.h"
#include "profibus4.h"

uint8_t uart_buffer[BUFFER_SIZE];
uint16_t rx_index=0, tx_index = 0,tx_counter=0;
uint8_t uart_bufferTX[BUFFER_SIZE];
extern uint8_t profibus_status;

int InitUSART2() { 
    
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
  
  USART_InitTypeDef USART_InitStructureUSART;  
  USART_InitStructureUSART.USART_BaudRate = 1500000;
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
 
  // TSYN истек, находимся в режиме ожидания данных. Приём первого байта
  if (profibus_status == PROFIBUS_WAIT_DATA){   
    GPIO_ResetBits(GPIOD,GPIO_Pin_4); 
    profibus_status = PROFIBUS_GET_DATA;                                        //Меняем статус на ПРОСЕСС ПОЛУЧЕНИЯ ДАННЫХ
    TIM3->CNT = 0;
    GPIO_SetBits(GPIOD,GPIO_Pin_2);
  }
  
  
  if (profibus_status == PROFIBUS_GET_DATA)  {
    rx_index++;
    if(rx_index==BUFFER_SIZE) rx_index=0;
    TIM3->CNT = 0;
  }
}
