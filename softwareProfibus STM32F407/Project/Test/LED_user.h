#include "stm32f4xx_gpio.h"

#define TOSENDSIZE 10000

void LEDInit(void);
void LEDOn(void);
void LEDOff(void);
void LED_On(uint8_t diod_silk);
void LED_Off(uint8_t diod_silk);
void LED_Toggle(uint8_t diod_silk);
void LEDToggle(void);
void Blink (int x, int on, int off);
