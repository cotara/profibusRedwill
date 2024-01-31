#include "LED_user.h"
#include "time_user.h"
//LED0 - обмен между мастер-контроллером (прибором) и платой
//LED1 - RX profibus
//LED2 - TX profibus
static uint16_t led_remap[3] = {GPIO_Pin_2, GPIO_Pin_3, GPIO_Pin_4};
uint8_t led_state=0;
void LEDInit(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  /* Enable the GPIO_LED Clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
    LED_Off(0);
    LED_Off(1);
    LED_Off(2);
}


void LED_Off(uint8_t diod_silk)
{
  GPIO_SetBits(GPIOE, led_remap[diod_silk]);
}
void LED_On(uint8_t diod_silk)
{
  GPIO_ResetBits(GPIOE, led_remap[diod_silk]);
}

void LED_Toggle(uint8_t diod_silk)
{
  GPIO_ToggleBits(GPIOE, led_remap[diod_silk]);
}

//
//void Blink (int x, int on, int off)                      
//{      
//        while (x--)
//        {
//          LEDOn();
//          delay_1_mcs (on);
//          LEDOff();
//          delay_1_mcs (off);
//        }  
//}
