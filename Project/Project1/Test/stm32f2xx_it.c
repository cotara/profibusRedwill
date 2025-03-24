/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx_it.h"
#include "time_user.h"
#include "usart_user.h"
#include "LED_user.h"
#include "profibus4.h"
#include "stm32f4xx_dma.h"
#include "main.h"
//#define MODBUS6_ON
//Профибас
extern  uint8_t uart_buffer[BUFFER_SIZE], uart_bufferTX[BUFFER_SIZE];           //Буферы профибаса
extern  uint16_t rx_index, tx_index,tx_counter;
uint16_t txrd_index=0;

extern uint8_t profibus_status;
extern uint32_t tBit;
extern uint8_t Module_cnt;

extern uint8_t  data_out_register[OUTPUT_DATA_SIZE];                            //Регистры профибаса на выход
extern uint8_t data_in_register [INPUT_DATA_SIZE];                              //Регистры профибаса на вход    
extern DeviceModel model;
//Мастер-контроллер
uint32_t _cnt=0;
extern uint8_t reqBuffer[10][8];                                                //Буфер запросов к мастер-контроллеру
extern uint8_t ansSize[10];                                                     //Массив размеров ответов на запросы...(((
extern uint8_t uart1_rx_buf[100],uart1_tx_buf[100];                            //Буфер приемный от мастрер-контроллера
extern uint16_t rx1_index,tx1_index,txrd1_index,tx1_counter;
extern uint8_t modbus_rcv_flag;
extern uint8_t deviceDataBuffer[5][50];                                         //Буфер с регистрами всех девайсов от мастер контроллера                          

uint8_t byteWait=0, func = 0;
uint8_t baudModbusOk=0;
uint8_t baudModbusNum=-1;
uint8_t modbus6_status=0;                                                       //0 - можно отправлять, 1 - отправлено. Ждем нуля
uint8_t reqIdx = 0; 
void HardFault_Handler(void) {
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1) {   }
}

void SysTick_Handler(void) {
    /* Decrement the timeout value */
    TimingDelay_1ms_Decrement();
}
//Обмен с профибасом
void USART2_IRQHandler(void) {  
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);                         //очистка признака прерывания       
        if ((USART2->SR & (USART_FLAG_NE|USART_FLAG_FE|USART_FLAG_PE|USART_FLAG_ORE)) == 0) {              //нет ошибок
              //TIM3->CNT = 0;                                                    // Сброс таймера Profibus
              uart_process(USART_ReceiveData(USART2) & 0xFF);
        }                                                                       //Сообщение из USART пришло без ошибок
        else{
           USART_ReceiveData(USART2);
            uart_error();
           TIM3->CNT = 0; 
        }
    }
        
    if (USART_GetITStatus(USART2, USART_IT_ORE_RX) == SET) {                     //прерывание по переполнению ошибке четности
      USART_ReceiveData(USART2);                                              //в идеале пишем здесь обработчик переполнения буфера, но мы просто сбрасываем этот флаг прерывания чтением из регистра данных.
    }
    
    
    if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {                      //прерывание по передаче
      USART_ClearITPendingBit(USART2, USART_IT_TXE);                           //очищаем признак прерывания
        
        TIM3->CNT = 0;                                                          // Сброс таймера Profibus  
        if (tx_counter) {                                                       //если есть что передать
            --tx_counter;                                                       // уменьшаем количество не переданных данных
            USART_SendData(USART2, uart_bufferTX[txrd_index++]);                  //передаем данные инкрементируя хвост буфера
        }
        else{
            USART_ITConfig(USART2, USART_IT_TXE, DISABLE);                       //если нечего передать, запрещаем прерывание по передаче
            USART_ITConfig(USART2, USART_IT_TC, ENABLE);
            txrd_index=0;
        }
    }
    if (USART_GetITStatus(USART2, USART_IT_TC) != RESET) {  
       USART_ClearITPendingBit(USART2, USART_IT_TC); 
       RX485EN;
       USART_ITConfig(USART2, USART_IT_TC, DISABLE);
    }
}

//Таймер для тактирования профибаса.
void TIM3_IRQHandler(void) {
  if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
   
   TIM_ClearFlag(TIM3, TIM_IT_Update);
      // Timer A Stop  
      TIM_Cmd(TIM3,DISABLE);
      TIM3->CNT = 0;

      switch (profibus_status) {
        case PROFIBUS_WAIT_SYN: 
            profibus_status = PROFIBUS_WAIT_DATA;
            timers_init((uint32_t)(TIMEOUT_MAX_SDR_TIME * tBit));
            rx_index = 0;
            break;
            
        case PROFIBUS_WAIT_DATA:                                                  // TSDR истёк
          break;
            
        case PROFIBUS_GET_DATA:                                                   // TSDR истёк, а данные есть
          profibus_status = PROFIBUS_WAIT_SYN;
          timers_init((uint32_t)(TIMEOUT_MAX_SYN_TIME * tBit));                      //Время синхронизации пошло    
          profibus_RX();
          rx_index = 0;
          break;
            
        case PROFIBUS_SEND_DATA:                                                  // Время ожидания отправки истекло, переводим на получение
          profibus_status = PROFIBUS_WAIT_SYN;         
          timers_init((uint32_t)(TIMEOUT_MAX_SYN_TIME * tBit));                      //Время синхронизации пошло            
          break;
        
        default:          
            break;

      }
      TIM_Cmd(TIM3,ENABLE); 
    }
}

//Периодический запрос 3 и 6 к мастер-контроллеру
//В OUT регистрах профибаса 1-й байт - управляющий отправкой. В него надо записать адрес регистра для записи
//Чтобы отправить следующий регистр надо записать следующий адрес.
//Чтобы два раза записать один и тот же регистр, надо записать промежуточный нуль.

void TIM5_IRQHandler(void) {
  if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET){
    TIM_ClearITPendingBit(TIM5, TIM_IT_Update); 
    
    unsigned short CRC16 = 0;
    if( model==NULL_device){                                                    //Если модель еще неизвестна, перед началом работы необходимо её выяснить
      if(baudModbusOk==0){
        baudModbusNum++;  
        if(baudModbusNum>4) baudModbusNum=0;
        InitUSART1(baudModbusNum);
      }
      reqIdx = 0;                                                               //Отправляем нулевой запрос на установление модели прибора.В ответ ждем 2 байта данных + 5 байт заголовка
    }
    else{                                                                       //Если номер модели уже установлен, можно начинть обмен
      reqIdx = 4*model - 3;                                                     //Предварительно настраиваем на запрос 3-й функции.
      
      if(data_in_register[0] == 0)      modbus6_status=0;                       //Обнуление командного статуса
      
      if(data_in_register[0] != 0 && modbus6_status == 0){                      //Есть команда на отправку и можно отправлять
        modbus6_status=1;                                                       //отправка совершена. Чтобы отправить следующий, ждем нуль в управляющем регистре
        
        reqIdx += data_in_register[0];                                          //Рассчитываем какой по счету запрос отправлять 
        if(reqIdx > 12) return;
        
        reqBuffer[reqIdx][4] = data_in_register[2 * data_in_register[0] -1];    //Вставляем значение регистра по 6-й функции
        reqBuffer[reqIdx][5] = data_in_register[2 * data_in_register[0]];
        CRC16 = crc16(&reqBuffer[reqIdx][0],6);                                 //Считаем CRC
        reqBuffer[reqIdx][6] = CRC16&0xFF;                                      //CRC L
        reqBuffer[reqIdx][7] = CRC16>>8;                                        //CRC H
      }    
    }
    USART1_put_string_2(&reqBuffer[reqIdx][0],8);                               //Отправляем запрос контроллеру
                                         
    LED_Off(0);
    LED_Off(1);
    LED_Off(2);
  }
}


//Обмен с прибором
void USART1_IRQHandler(void) {  
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);                         //очистка признака прерывания       
        if ((USART1->SR & (USART_FLAG_NE|USART_FLAG_FE|USART_FLAG_PE|USART_FLAG_ORE)) == 0) {              //нет ошибок                                            
              uart1_rx_buf[rx1_index++] = USART_ReceiveData(USART1) & 0xFF;
              TIM2->CNT = 0;
              TIM_Cmd(TIM2, ENABLE);
              
        }                                                                       //Сообщение из USART пришло без ошибок
        else{
            USART_ReceiveData(USART1);
            TIM2->CNT = 0;
        }
    }
        
    if (USART_GetITStatus(USART1, USART_IT_ORE_RX) == SET) {                    //прерывание по переполнению ошибке четности
      USART_ReceiveData(USART1);                                                //в идеале пишем здесь обработчик переполнения буфера, но мы просто сбрасываем этот флаг прерывания чтением из регистра данных.
    }
    
    
    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {                     //прерывание по передаче
      USART_ClearITPendingBit(USART1, USART_IT_TXE);                            //очищаем признак прерывания
        
        if (tx1_counter) {                                                      //если есть что передать
            --tx1_counter;                                                      // уменьшаем количество не переданных данных
            USART_SendData(USART1, uart1_tx_buf[txrd1_index++]);               //передаем данные инкрементируя хвост буфера
            if (txrd1_index == BUFFER_SIZE) 
              txrd1_index=0;                                                    //Идем по кругу
        }
        else{
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);                       //если нечего передать, запрещаем прерывание по передаче
            USART_ITConfig(USART1, USART_IT_TC, ENABLE);
        }
    }
    if (USART_GetITStatus(USART1, USART_IT_TC) != RESET) {  
       USART_ClearITPendingBit(USART1, USART_IT_TC); 
       RXUART485EN;
       USART_ITConfig(USART1, USART_IT_TC, DISABLE);
    }

}
//Модбас щит контрол
void TIM2_IRQHandler(void) {
  if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    
   unsigned short CRC16 = crc16(&uart1_rx_buf[0],rx1_index-2);
   if((uart1_rx_buf[rx1_index-2] == (CRC16&0xFF)) && (uart1_rx_buf[rx1_index-1] == (CRC16>>8)))
   {    

     LED_On(0);
     _cnt++;
     modbus_rcv_flag=1;
    //Ответ на первый запрос модели       
    if(model==0){                                                                                                              
     model=uart1_rx_buf[3] & 0x0f;                                                  //Записываем модель
     profibusSetAddress(uart1_rx_buf[6]);
     setModbusAddres(uart1_rx_buf[6]);
     baudModbusOk=1;
    }

    
   }

    rx1_index=0;
    TIM_Cmd(TIM2, DISABLE);
  }
}
/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
