#include "./led/led.h"
#include "gpio.h"


void LED_Ctrl(LED_TYPE LED ,LED_STATE Ctrl)
{
	switch (LED)
		{
			case 1: //Red LED
				if(Ctrl == On)
				{
					HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_RESET);
				}
				else if(Ctrl == Off)
				{
					HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, GPIO_PIN_SET);
				}
				break;
			case 2: //Green LED
				if(Ctrl == On)
				{
					HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, GPIO_PIN_RESET);
				}
				else if(Ctrl == Off)
				{
					HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, GPIO_PIN_SET);
				}
				break;
			case 3: //Blue LED
				if(Ctrl == On)
				{
					HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, GPIO_PIN_RESET);
				}
				else if(Ctrl == Off)
				{
					HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, GPIO_PIN_SET);
				}
				break;
		}
}