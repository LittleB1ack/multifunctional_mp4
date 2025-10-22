/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32f4xx_it.h"
#include "sdio.h"
#include "dma.h"
#include "./wm8978/bsp_wm8978.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>  /* fault diagnostics */
#include <stdint.h>
#include "stm32f4xx.h"  /* SCB and core intrinsics */
#include "core_cm4.h"    /* Ensure SCB, __disable_irq, __NOP are visible to the compiler */
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
#include "bsp_i2c_touch.h"
#include "gt9xx.h"
/* USER CODE END TD */

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
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim4;
/* SDIO + DMA external handles */
extern SD_HandleTypeDef hsd;
extern DMA_HandleTypeDef hdma_sdio_rx;
extern DMA_HandleTypeDef hdma_sdio_tx;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
__asm void HardFault_Handler(void)
{
  IMPORT  HardFault_HandlerC
  TST     LR, #4
  ITE     EQ
  MRSEQ   R0, MSP
  MRSNE   R0, PSP
  B       HardFault_HandlerC
}

void HardFault_HandlerC(uint32_t *stacked_sp)
{
  uint32_t stacked_r0  = stacked_sp[0];
  uint32_t stacked_r1  = stacked_sp[1];
  uint32_t stacked_r2  = stacked_sp[2];
  uint32_t stacked_r3  = stacked_sp[3];
  uint32_t stacked_r12 = stacked_sp[4];
  uint32_t stacked_lr  = stacked_sp[5];
  uint32_t stacked_pc  = stacked_sp[6];
  uint32_t stacked_psr = stacked_sp[7];

  printf("[FAULT] HardFault!\r\n");
  printf(" r0  = 0x%08lX\r\n", (unsigned long)stacked_r0);
  printf(" r1  = 0x%08lX\r\n", (unsigned long)stacked_r1);
  printf(" r2  = 0x%08lX\r\n", (unsigned long)stacked_r2);
  printf(" r3  = 0x%08lX\r\n", (unsigned long)stacked_r3);
  printf(" r12 = 0x%08lX\r\n", (unsigned long)stacked_r12);
  printf(" lr  = 0x%08lX\r\n", (unsigned long)stacked_lr);
  printf(" pc  = 0x%08lX\r\n", (unsigned long)stacked_pc);
  printf(" psr = 0x%08lX\r\n", (unsigned long)stacked_psr);
  printf(" HFSR=0x%08lX CFSR=0x%08lX BFAR=0x%08lX MMFAR=0x%08lX\r\n",
    (unsigned long)SCB->HFSR, (unsigned long)SCB->CFSR,
    (unsigned long)SCB->BFAR, (unsigned long)SCB->MMFAR);

  /* Visual panic indicator: blink PF6 using register-level access */
  __disable_irq();
  /* Enable GPIOF clock */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
  (void)RCC->AHB1ENR;
  /* PF6 as output */
  GPIOF->MODER &= ~(3UL << (6U * 2U));
  GPIOF->MODER |=  (1UL << (6U * 2U));
  GPIOF->OTYPER &= ~(1UL << 6);
  GPIOF->PUPDR &= ~(3UL << (6U * 2U));
  for(;;) {
    GPIOF->ODR ^= (1UL << 6);
    for (volatile uint32_t d = 0; d < 200000U; ++d) { __NOP(); }
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */
  printf("[FAULT] MemManage fault!\r\n");

  /* USER CODE END MemoryManagement_IRQn 0 */
  /* Blink PF6 to indicate fault */
  __disable_irq();
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
  (void)RCC->AHB1ENR;
  GPIOF->MODER &= ~(3UL << (6U * 2U));
  GPIOF->MODER |=  (1UL << (6U * 2U));
  GPIOF->OTYPER &= ~(1UL << 6);
  GPIOF->PUPDR &= ~(3UL << (6U * 2U));
  for(;;) {
    GPIOF->ODR ^= (1UL << 6);
    for (volatile uint32_t d = 0; d < 200000U; ++d) { __NOP(); }
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */
  printf("[FAULT] BusFault!\r\n");

  /* USER CODE END BusFault_IRQn 0 */
  __disable_irq();
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
  (void)RCC->AHB1ENR;
  GPIOF->MODER &= ~(3UL << (6U * 2U));
  GPIOF->MODER |=  (1UL << (6U * 2U));
  GPIOF->OTYPER &= ~(1UL << 6);
  GPIOF->PUPDR &= ~(3UL << (6U * 2U));
  for(;;) {
    GPIOF->ODR ^= (1UL << 6);
    for (volatile uint32_t d = 0; d < 200000U; ++d) { __NOP(); }
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles TIM4 global interrupt.
  */
void TIM4_IRQHandler(void)
{
  /* USER CODE BEGIN TIM4_IRQn 0 */

  /* USER CODE END TIM4_IRQn 0 */
  HAL_TIM_IRQHandler(&htim4);
  /* USER CODE BEGIN TIM4_IRQn 1 */

  /* USER CODE END TIM4_IRQn 1 */
}

/**
  * @brief This function handles SDIO global interrupt.
  */
void SDIO_IRQHandler(void)
{
  /* USER CODE BEGIN SDIO_IRQn 0 */

  /* USER CODE END SDIO_IRQn 0 */
  HAL_SD_IRQHandler(&hsd);
  /* USER CODE BEGIN SDIO_IRQn 1 */

  /* USER CODE END SDIO_IRQn 1 */
}

/**
  * @brief This function handles DMA2 Stream3 global interrupt (SDIO RX).
  */
void DMA2_Stream3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream3_IRQn 0 */

  /* USER CODE END DMA2_Stream3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_sdio_rx);
  /* USER CODE BEGIN DMA2_Stream3_IRQn 1 */

  /* USER CODE END DMA2_Stream3_IRQn 1 */
}

/**
  * @brief This function handles DMA2 Stream6 global interrupt (SDIO TX).
  */
void DMA2_Stream6_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream6_IRQn 0 */

  /* USER CODE END DMA2_Stream6_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_sdio_tx);
  /* USER CODE BEGIN DMA2_Stream6_IRQn 1 */

  /* USER CODE END DMA2_Stream6_IRQn 1 */
}

/* USER CODE BEGIN 1 */
void GTP_IRQHandler(void)
{
	
	
	if(__HAL_GPIO_EXTI_GET_IT(GTP_INT_GPIO_PIN) != RESET) //确保是否产生了EXTI Line中断
	{
		GTP_TouchProcess();    
		__HAL_GPIO_EXTI_CLEAR_IT(GTP_INT_GPIO_PIN);       //清除中断标志位
	}  
	
}


void DMA1_Stream4_IRQHandler(void)
   {
       I2Sx_TX_DMA_STREAM_IRQFUN();
   }



/* USER CODE END 1 */
