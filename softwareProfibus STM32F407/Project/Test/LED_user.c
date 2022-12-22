#include "LED_user.h"
#include "time_user.h"

static uint16_t led_remap[6] = {GPIO_Pin_1, GPIO_Pin_0, GPIO_Pin_2, GPIO_Pin_3, GPIO_Pin_4};
uint8_t led_state=0;
void LEDInit(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  /* Enable the GPIO_LED Clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_8 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
}


void LEDOn(void)
{
  GPIOC->BSRRH=GPIO_Pin_13;  
}
void LEDOff(void)
{
  GPIOC->BSRRL=GPIO_Pin_13;  
}


void LED_On(uint8_t diod_silk)
{
  GPIO_SetBits(GPIOD, led_remap[diod_silk - 2]);
}
void LED_Off(uint8_t diod_silk)
{
  GPIO_ResetBits(GPIOD, led_remap[diod_silk - 2]);
}

void LED_Toggle(uint8_t diod_silk)
{
  GPIO_ToggleBits(GPIOD, led_remap[diod_silk - 2]);
}

void LEDToggle(void)
{
  GPIOC->ODR ^= GPIO_Pin_13;
}

void Blink (int x, int on, int off)                      
{      
        while (x--)
        {
          LEDOn();
          delay_1_mcs (on);
          LEDOff();
          delay_1_mcs (off);
        }  
}
