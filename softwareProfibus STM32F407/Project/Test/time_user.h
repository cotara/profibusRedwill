#include "stm32f4xx_tim.h"
#define DLY_100US  16800

void timers_init(uint32_t period2);
int RTC_init(void);
void update_Time(RTC_TimeTypeDef *time, unsigned char* RTC_time);
void TimingDelay_1ms_Decrement(void);
void delay_1_ms(volatile uint32_t nTime);
void delay_1_mcs(volatile uint32_t nTime);
void TimingDelay_1mcs_Decrement(void);

