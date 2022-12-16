/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx_it.h"
#include "time_user.h"
#include "usart_user.h"
#include "LED_user.h"
#include "profibus4.h"

extern  uint8_t uart_buffer[BUFFER_SIZE], uart_bufferTX[BUFFER_SIZE];;
extern  uint16_t rx_index, tx_index,tx_counter;
extern uint8_t profibus_status;
uint16_t txrd_index=0;
uint32_t mcs=0;
uint32_t counter=0;
void HardFault_Handler(void) {
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1) {
    }
}

void SysTick_Handler(void) {
    /* Decrement the timeout value */
    TimingDelay_1ms_Decrement();
      
}

void USART2_IRQHandler(void) {  
  
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);                         //очистка признака прерывания       
        if ((USART2->SR & (USART_FLAG_NE|USART_FLAG_FE|USART_FLAG_PE|USART_FLAG_ORE)) == 0) {              //нет ошибок
              TIM3->CNT = 0;                                                    // Сброс таймера Profibus
              uart_process(USART_ReceiveData(USART2) & 0xFF);
        }                                                                       //Сообщение из USART пришло без ошибок
        else{
           USART_ReceiveData(USART2);
        }
    }
        
    if (USART_GetITStatus(USART2, USART_IT_ORE_RX) == SET) {                    //прерывание по переполнению буфера
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

//Таймер для сброса непринятой посылки
void TIM3_IRQHandler(void) {
  if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
   
   TIM_ClearFlag(TIM3, TIM_IT_Update);
      // Timer A Stop  
      TIM_Cmd(TIM3,DISABLE);
      TIM3->CNT = 0;
      //GPIOD->ODR ^= GPIO_Pin_0;

      switch (profibus_status) {
        case PROFIBUS_WAIT_SYN: 
            profibus_status = PROFIBUS_WAIT_DATA;
            timers_init((uint32_t)(TIMEOUT_MAX_SDR_TIME+0.5));
            rx_index = 0;
            break;
            
        case PROFIBUS_WAIT_DATA:                                                  // TSDR истёк
            break;
            
        case PROFIBUS_GET_DATA:                                                   // TSDR истёк, а данные есть
          profibus_status = PROFIBUS_WAIT_SYN;
          timers_init((uint32_t)(TIMEOUT_MAX_SYN_TIME+0.5));                      //Время синхронизации пошло       
          TIM_Cmd(TIM3,ENABLE);
          GPIO_SetBits(GPIOD,GPIO_Pin_0);
          profibus_RX();
          GPIO_ResetBits(GPIOD,GPIO_Pin_0);
          break;
            
        case PROFIBUS_SEND_DATA:                                                  // Время ожидания отправки истекло, переводим на получение
          GPIO_ResetBits(GPIOD,GPIO_Pin_1);
          profibus_status = PROFIBUS_WAIT_SYN;
          timers_init((uint32_t)(TIMEOUT_MAX_SYN_TIME+0.5));                      //Время синхронизации пошло   
          //RX485EN;           
          break;
        
        default:          
            break;

      }
      
      
      TIM_Cmd(TIM3,ENABLE); 
      
  //    setRxi(0);                      //1 мс не приходило нового байта. Значит был затык
  //    TIM_Cmd(TIM2, DISABLE);         //Перестали считать. Ждем следующей посылки.
  //    TIM2->CNT = 0;
    }
}
//Таймер для задержек
void TIM5_IRQHandler(void) {
  // Clear TIM5 counter
   TIM_ClearFlag(TIM5, TIM_IT_Update);  
   TimingDelay_1mcs_Decrement();    
}

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
