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
uint8_t iteratorModbus=1;
uint8_t maxIterator=4;                                                          //Количество запросов в круге запросов (Зависит от устройства)
extern uint16_t devModel;
extern uint8_t reqBuffer[10][8];                                                //Буфер запросов к мастер-контроллеру
extern uint8_t ansSize[10];                                                     //Массив размеров ответов на запросы...(((
extern uint8_t uart1_rx_buf[100];                                               //Буфер приемный от мастрер-контроллера
extern uint8_t deviceDataBuffer[5][50];                                         //Буфер с регистрами всех девайсов от мастер контроллера                          

uint8_t byteWait=0, func = 0;
uint8_t baudModbusOk=0;
uint8_t baudModbusNum=-1;
uint8_t modbus6_counter_old=0;

void HardFault_Handler(void) {
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1) {   }
}

void SysTick_Handler(void) {
    /* Decrement the timeout value */
    TimingDelay_1ms_Decrement();
}
//Обмне с профибасом
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
       RX485RE;
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
void TIM5_IRQHandler(void) {
  if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET){
    DMA_Cmd(DMA2_Stream5, DISABLE);                                             //Отрубаем прием и передачу на всякий
    DMA_Cmd(DMA2_Stream7, DISABLE);
    unsigned short CRC16 = 0;
    if( model==NULL_device){                                                    //Если модель еще неизвестна, перед началом работы необходимо её выяснить
      if(baudModbusOk==0){
        baudModbusNum++;  
        if(baudModbusNum>4) baudModbusNum=0;
        InitUSART1(baudModbusNum);
      }
      DMA2_Stream7->M0AR = (uint32_t)&reqBuffer[0][0];                          //Отправляем нулевой запрос на установление модели прибора
      byteWait=ansSize[0];  
      DMA2_Stream5->NDTR = ansSize[0];                                          //В ответ ждем 2 байта данных + 5 байт заголовка
    }
    else{                                                                       //Если номер модели уже установлен, можно начинть обмен
           
      if(data_in_register[0]!=modbus6_counter_old)                           //Был инкрементирован счетчик,  значит команда на отправку 6-й функции
        iteratorModbus=2;
        
      if(iteratorModbus!=1){                                                    //Формируется запрос на 6-ю функцию
              
          if(Module_cnt>2){                                                      //Отладочный проект  (В TIA Portal вытащили больше 1 прибора)  
            reqBuffer[(model-1)*4+iteratorModbus][4] = data_in_register[(model-1)*7+(iteratorModbus-2)*2 + 1];            //Вставляем значение регистра по 6-й функции
            reqBuffer[(model-1)*4+iteratorModbus][5] = data_in_register[(model-1)*7+(iteratorModbus-2)*2 + 2];
          }
          else{                                                                 //Заводской проект
            reqBuffer[(model-1)*4+iteratorModbus][4] = data_in_register[(iteratorModbus-2)*2+1];            //Вставляем значение регистра по 6-й функции
            reqBuffer[(model-1)*4+iteratorModbus][5] = data_in_register[(iteratorModbus-2)*2 + 2];
          }
          
          CRC16 = crc16(&reqBuffer[(model-1)*4+iteratorModbus][0],6);        //Считаем CRC
          reqBuffer[(model-1)*4+iteratorModbus][6] = CRC16&0xFF;  //CRC L
          reqBuffer[(model-1)*4+iteratorModbus][7] = CRC16>>8;     //CRC H
          
                                                       
                                   //Отправили все запросы - начинаем сначала
          
          modbus6_counter_old = data_in_register[0];
      }

      
      DMA2_Stream7->M0AR = (uint32_t)&reqBuffer[(model-1)*4+iteratorModbus][0];//Ставим указатель на соответствующий запрос в массиве запросов.
      byteWait = ansSize[(model-1)*4+iteratorModbus];
      DMA2_Stream5->NDTR = byteWait;                                            //Сколько байт ждать в ответ. В зависимости от типа запроса и модели девайса ждать будем определенное количество байт                        
      if(iteratorModbus!=1) iteratorModbus++;                                     //Для каждой модели определен перечень и порядок запросов. ID: 3-6-6-6 ZASI: 3-6-6-6 LDM: 3
      if(iteratorModbus>maxIterator)
            iteratorModbus=1; 
    }
    
    DMA_Cmd(DMA2_Stream7, ENABLE);                                              //Запуск отправки запроса  
    LED_Off(0);
    LED_Off(1);
    LED_Off(2);
    TIM_ClearFlag(TIM5, TIM_IT_Update); 
  }
}
//USART1 TX
void DMA2_Stream7_IRQHandler(){
 if (DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7) != RESET){
   DMA_Cmd(DMA2_Stream7, DISABLE);
   DMA_Cmd(DMA2_Stream5, ENABLE); 
   DMA_ClearFlag(DMA2_Stream7, DMA_IT_TCIF7);
 }
}
//USART1 RX
void DMA2_Stream5_IRQHandler(){
 if (DMA_GetITStatus(DMA2_Stream5, DMA_IT_TCIF5) != RESET){
   DMA_ClearFlag(DMA2_Stream5, DMA_IT_TCIF5);
   DMA_Cmd(DMA2_Stream5, DISABLE);
   
   unsigned short CRC16 = crc16(&uart1_rx_buf[0],byteWait-2);
   if((uart1_rx_buf[byteWait-2] == (CRC16&0xFF)) && (uart1_rx_buf[byteWait-1] == (CRC16>>8)))
   {    
     LED_On(0);
     //Ответ на первый запрос модели       
     if(model==0){                                                                                                              
       model=uart1_rx_buf[3];                                                  //Записываем модель
       profibusSetAddress(uart1_rx_buf[6]);
       setModbusAddres(uart1_rx_buf[6]);
       if(model == 3) maxIterator =1;                                          //У ЛДМ только 3 функция, поэтому в круге запросов только 1 запрос
       baudModbusOk=1;
     }

     //Ответ на запрос в круге запросов       
     else{
       func = uart1_rx_buf[1];
       if(model == 1){                                                        //ИД
         if(func==3){                                                           //Обновляем выходные регистры, если это ответ на 3 функцию
           memcpy(&deviceDataBuffer[1][0],&uart1_rx_buf[3],16);
           if (Module_cnt==1 || Module_cnt==2)                                    //Заводской проект
             memcpy(&data_out_register[0],&deviceDataBuffer[1][0],16);            //Кладем данные девайса на первое место
           else if(Module_cnt>2)                                                  //Отладочный проект  (В TIA Portal вытащили больше 1 прибора)   
             memcpy(&data_out_register[0],&deviceDataBuffer[1][0],16);            //Кладем данные девайса (ИД) на свое место в буфере профибаса
         }
       }
       else if(model == 2){                                                    //ЗАСИ
         if(func==3){
           memcpy(&deviceDataBuffer[2][0],&uart1_rx_buf[7],4);
           memcpy(&deviceDataBuffer[2][4],&uart1_rx_buf[23],14);
           if (Module_cnt==1 || Module_cnt==2)                                    //Заводской проект
             memcpy(&data_out_register[0],&deviceDataBuffer[2][0],18);            //Кладем данные девайса на первое место
           if(Module_cnt>2)                                                       //Отладочный проект  (В TIA Portal вытащили больше 1 модулька)      
             memcpy(&data_out_register[16],&deviceDataBuffer[2][0],18);           //Кладем данные девайса (ЗАСИ) на свое место в буфере профибаса
         }
       }
       else if(model == 3){                                                    //LDM
         if(func==3){
           memcpy(&deviceDataBuffer[3][0],&uart1_rx_buf[3],12);
           memcpy(&deviceDataBuffer[3][12],&uart1_rx_buf[19],8);
           memcpy(&deviceDataBuffer[3][20],&uart1_rx_buf[30],1);
           if (Module_cnt==1 || Module_cnt==2)                                      //Заводской проект
             memcpy(&data_out_register[0],&deviceDataBuffer[3][0],21);              //Кладем данные девайса на первое место       
           else if (Module_cnt>2)                                                   //Отладочный проект (В TIA Portal вытащили больше 1 модулька)       
             memcpy(&data_out_register[34],&deviceDataBuffer[3][0],21);             //Кладем данные девайса (ЛДМ) на свое место в буфере профибаса
         }
       }    
     }
   }
   _cnt++;
   
 }
}
/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
