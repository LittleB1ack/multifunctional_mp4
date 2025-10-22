#ifndef __LED_H__
#define __LED_H__
#include "main.h"


typedef enum{
	On = 1,
	Off = 2
}LED_STATE;

typedef enum{
	LED_R = 1,
	LED_G = 2,
	LED_B = 3
}LED_TYPE;


void LED_Ctrl(LED_TYPE LED ,LED_STATE Ctrl);




#endif
