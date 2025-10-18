#include "led.h"
#include "gpio.h"

void LED_Ctrl(uint8_t Ctrl)
{
	if(Ctrl == On)
	{
		HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_RESET);
	}
	else if(Ctrl == Off)
	{
		HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_SET);
	}
}

