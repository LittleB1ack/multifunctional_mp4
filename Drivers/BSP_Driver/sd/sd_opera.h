/**
  ******************************************************************************
  * @file           : sd_opera.h
  * @author         : Kevin_WWW
  * @brief          : None
  * @attention      : None
  * @date           : 2024/5/5
  ******************************************************************************
  */
#ifndef DEMO13_1SDRAW_SD_OPERA_H
#define DEMO13_1SDRAW_SD_OPERA_H

#include "main.h"
#include "sdio.h"
#include <stdio.h>

void SDCard_ShowInfo(void);
void SDCard_EraseBlocks(void);
void SDCard_TestWrite(void);
void SDCard_TestRead(void);

#endif //DEMO13_1SDRAW_SD_OPERA_H
