#include "time_user.h"
#include "stm32f4xx_pwr.h"

volatile uint32_t TimingDelay_1mcs,TimingDelay_1ms;


void timers_init(uint32_t period2)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_DeInit(TIM3);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
//  /***********************************************************************
//                                TIMER2              
//  ***********************************************************************/
  TIM_InternalClockConfig(TIM3);
  //TIM_TimeBaseStructure.TIM_Prescaler = 168-1;                                    //Таймер настроен на 500 кГц
  TIM_TimeBaseStructure.TIM_Prescaler = 7-1;                                    //Таймер настроен на 12 МГц
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = period2-1;                                    //1мс
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);
   
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
  TIM_ClearFlag(TIM3,TIM_IT_Update);
  
  NVIC_SetPriority (TIM3_IRQn, 1);
  NVIC_EnableIRQ (TIM3_IRQn);

  TIM_Cmd(TIM3,ENABLE); 

}
void timer5_init(void){
 /***********************************************************************
                                TIMER5              
  ***********************************************************************/
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);

  TIM_InternalClockConfig(TIM5);
  
  TIM_TimeBaseStructure.TIM_Prescaler = 8400-1;                                 //0.1 мс
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = 5000-1;                                    //4 раза в секунду
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM5,&TIM_TimeBaseStructure);
  
  TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
  TIM_ClearFlag(TIM5,TIM_IT_Update);
   
  NVIC_SetPriority (TIM5_IRQn, 0);
  NVIC_EnableIRQ (TIM5_IRQn);
  
  TIM_Cmd(TIM5,ENABLE);
}

void update_Time(RTC_TimeTypeDef *time, unsigned char* RTC_time)
{ 
  unsigned char h1,h2,m1,m2,s1,s2;
  
      if( SysTick->CTRL & (1<<16))
      {
        RTC_GetTime(RTC_Format_BIN,time);
        h1=48+time->RTC_Hours/10;
        h2=48+time->RTC_Hours%10;
        m1=48+time->RTC_Minutes/10;
        m2=48+time->RTC_Minutes%10;
        s1=48+time->RTC_Seconds/10;
        s2=48+time->RTC_Seconds%10;
        
        memcpy(RTC_time,&h1,1);
        memcpy(RTC_time+1,&h2,1);
        memcpy(RTC_time+2,":",1);
        memcpy(RTC_time+3,&m1,1);
        memcpy(RTC_time+4,&m2,1);
        memcpy(RTC_time+5,":",1);
        memcpy(RTC_time+6,&s1,1);
        memcpy(RTC_time+7,&s2,1);
              
      }
}

/*Задержка в nTime 1 мс*/
void delay_1_ms(volatile uint32_t nTime){
  TimingDelay_1ms = nTime;
  while(TimingDelay_1ms != 0);
}
void TimingDelay_1ms_Decrement(void){
  if (TimingDelay_1ms != 0x00)
    TimingDelay_1ms--;
}


/*Задержка в nTime 1 мкс*/
void delay_1_mcs(volatile uint32_t nTime){
  TimingDelay_1mcs = nTime;
  while(TimingDelay_1mcs != 0);
}
void TimingDelay_1mcs_Decrement(void){
  if (TimingDelay_1mcs != 0x00)
    TimingDelay_1mcs--;
}