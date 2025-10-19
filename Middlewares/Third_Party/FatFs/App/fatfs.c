/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
#include "fatfs.h"

#include <stdio.h>

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */
#include "./sd/file_opera.h"
/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* Keep a concise log message; put any verbose diagnostics in USER CODE */
  printf("[FATFS] ===== FATFS INIT =====\r\n");
  printf("[FATFS] LinkDriver status = %d\r\n", retSD);
  printf("[FATFS] -----------------------------\r\n");

  /* USER CODE BEGIN Init */
  /* additional user code for init (verbose diagnostics) */
  /* USER CODE BEGIN VerboseInit */
  /* Example verbose output (kept in USER CODE so CubeMX won't overwrite):
    printf("MX_FATFS_Init: FATFS_LinkDriver returned %d, SDPath=%s\r\n", retSD, SDPath);
  */
  /* USER CODE END VerboseInit */
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
	
  return fat_GetFatTimeFromRTC();
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */
