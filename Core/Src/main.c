/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"  /* for osKernelInitialize/osKernelStart (CMSIS-RTOS2) */
#include "gpio.h"
#include "dma.h"
#include "sdio.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */


/* 包含系统相关头文件 */
#include "sys.h"
#include "rtc.h"
#include "usart.h"
#include "delay.h"
#include "./led/led.h"
#include "bsp_nt35510_lcd.h"
#include "bsp_debug_usart.h"
#include "../../Middlewares/Third_Party/FatFs/Target/bsp_driver_sd.h"
#include "./sdram/bsp_sdram.h"  

/* 音频相关头文件 */
#include "./wm8978/bsp_wm8978.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	DEBUG_USART_Config();               /* 先初始化串口，保证后续日志可见 */

  printf("\r\n\r\n");
  printf("------------------------------------------------------------\r\n");
  printf("STM32F407 Multi-Functional MP4 Player - System Initializing\r\n");
  printf("------------------------------------------------------------\r\n");
  printf("\r\n");
  
  delay_init(168);                    /* 延时初始化（基于 168MHz） */
  printf("[INIT] Clock:        168 MHz\r\n");
  printf("[INIT] USART:        Ready (115200 8N1)\r\n");
  
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  printf("\r\n------------------------------------------------------------\r\n");
  printf("System Peripheral Initialization\r\n");
  printf("------------------------------------------------------------\r\n");
  MX_GPIO_Init();
  printf("[GPIO] All GPIO ports initialized\r\n");

  /* 打印内存布局摘要 - 用于资源分配诊断 */
  printf("\r\n");
  // sram_print_init_summary(); // 暂时注释，因为EXTSRAM测试失败

	
  /* USER CODE BEGIN 2 */
  /* 按 HAL 要求先初始化 DMA 与 SDIO（配置 hsd、打开 SDIO 时钟/DMA 时钟/中断），再调用 BSP_SD_Init */
  MX_DMA_Init();
  printf("[DMA]  DMA channels initialized\r\n");
  
  MX_SDIO_SD_Init();
  printf("[SDIO] SDIO interface initialized\r\n");
  
  printf("\r\n------------------------------------------------------------\r\n");
  printf("SD Card Initialization\r\n");
  printf("------------------------------------------------------------\r\n");
  if(BSP_SD_Init() != MSD_OK) {
    printf("[SD] Initialization: FAIL\r\n");
  } else {
    printf("[SD] Initialization: PASS\r\n");
    printf("\r\n");
    BSP_SD_PrintCardInfo();           /* 打印 SD 卡信息 */
    printf("\r\n[SD] Running read/write test...\r\n");
    BSP_SD_Test(1);                   /* 可选：做一次读写自检 */
    printf("[SD] Test completed\r\n");
  }

  /* USER CODE END 2 */

  /* Init scheduler */
  printf("\r\n------------------------------------------------------------\r\n");
  printf("FreeRTOS Kernel Initialization\r\n");
  printf("------------------------------------------------------------\r\n");
  osKernelInitialize();
  printf("[RTOS] Kernel initialized\r\n");

  printf("[RTOS] Creating tasks and semaphores...\r\n");
  MX_FREERTOS_Init();
  printf("[RTOS] Tasks created successfully\r\n");
  
  printf("\r\n");
  printf("------------------------------------------------------------\r\n");
  printf("Starting FreeRTOS Kernel Scheduler\r\n");
  printf("------------------------------------------------------------\r\n");
  printf("\r\n");
  
  osKernelStart();
  
  /* If code continues past here, the scheduler did not start */
  printf("\r\n[RTOS][ERROR] Scheduler failed to start!\r\n");

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
  }
    

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  /* HSI 16MHz: VCO = 16/8*168 = 336MHz, SYSCLK=168MHz, PLLQ=336/7=48MHz */
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called when TIM4 interrupt took place, inside
  *         HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  *         a global variable used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* 在Error_Handler中快速闪烁红色LED，方便判断是否进入了错误处理 */
  __disable_irq();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  GPIO_InitTypeDef gi = {0};
  gi.Pin = GPIO_PIN_6;
  gi.Mode = GPIO_MODE_OUTPUT_PP;
  gi.Pull = GPIO_NOPULL;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &gi);
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_6);
    HAL_Delay(100);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
