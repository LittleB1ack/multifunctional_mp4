/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "./led/led.h"
#include "lvgl.h"
#include "stdio.h"
#include "string.h"
#include "semphr.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "queue.h"
#include "fatfs.h"
#include "ff.h"

/*audio driver and mp3 decoder*/
#include "./wm8978/bsp_wm8978.h"
#include "mp3Player.h"
#include "math.h"


#define scr_act_width() lv_obj_get_width(lv_scr_act())
#define scr_act_height() lv_obj_get_height(lv_scr_act())

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
/* USER CODE BEGIN Variables */
static QueueHandle_t ledQueue;  /* 全局队列，用于 LVGL -> LED 任务传输状态 */
static SemaphoreHandle_t fsReadySemaphore; /* 文件系统就绪信号量 */
/* USER CODE END Variables */



/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void LVGL_Process(void* param);
void LED_Process(void *params);
void FS_Process(void *params);
void Audio_Test_Task(void *params);
/* USER CODE END FunctionPrototypes */
 


void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */

  /* 创建 LED 状态队列（在创建任务前） */
  ledQueue = xQueueCreate(4, sizeof(uint8_t));
  fsReadySemaphore = xSemaphoreCreateBinary(); /* 创建文件系统就绪信号量 */

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */

  /* USER CODE BEGIN RTOS_THREADS */


  /* 创建 LED 任务 */
  xTaskCreate(LED_Process,"LED_Task",128,NULL,osPriorityNormal,NULL);
  /* 创建 LVGL 任务 */
  xTaskCreate(LVGL_Process,"LVGL_Task",512,NULL,osPriorityNormal,NULL);

  /* FS 任务优先级设为 osPriorityAboveNormal，确保优先完成文件系统初始化 */
  xTaskCreate(FS_Process, "FS_Task", 512, NULL, osPriorityAboveNormal, NULL);
  /* 创建音频任务，增加栈大小到2048，优先级设为 osPriorityNormal，等待文件系统就绪后再运行 */
  xTaskCreate(Audio_Test_Task, "AudioTest", 2048, NULL, osPriorityBelowNormal, NULL); 


  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* LVGL 开关事件回调（LVGL v8）*/
static void switch_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *sw = lv_event_get_target(e);
    uint8_t state = lv_obj_has_state(sw, LV_STATE_CHECKED) ? On : Off; /* 使用 led.h 中的 On/Off */
    if(ledQueue) {
        xQueueSend(ledQueue, &state, 0); /* 非阻塞发送 */
    }
}





/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* RTOS diagnostics: malloc failed and stack overflow hooks */
void vApplicationMallocFailedHook(void)
{
  /* Called if a call to pvPortMalloc() fails because there is insufficient free memory available. */
  printf("[RTOS][HOOK] Malloc failed! Free heap too low.\r\n");
  taskDISABLE_INTERRUPTS();
  for(;;) { /* Trap here for debugging */ }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  /* Called if a stack overflow is detected during context switch. */
  (void)xTask; /* Unused */
  printf("[RTOS][HOOK] Stack overflow in task: %s\r\n", pcTaskName ? pcTaskName : "<null>");
  taskDISABLE_INTERRUPTS();
  for(;;) { /* Trap here for debugging */ }
}
void LED_Process(void *params)
{
  printf("[LED] LED_Process started\r\n");
    uint8_t state = 0;
    uint32_t loop_count = 0;
    for(;;)
    {		
			loop_count++;
			printf("[LED] Loop #%lu: Turning ON\r\n", loop_count);
			LED_Ctrl(LED_G,On);
			osDelay(1000);
			printf("[LED] Loop #%lu: Turning OFF\r\n", loop_count);
			LED_Ctrl(LED_G,Off);
			osDelay(1000);

        if(ledQueue && xQueueReceive(ledQueue, &state, portMAX_DELAY) == pdTRUE) {
            LED_Ctrl(LED_R,!state);
        }
    }
}


void LVGL_Process(void* param)
{
    lv_init();                    /* lvgl系统初始化 */ 

    lv_port_disp_init();          /* lvgl显示接口初始化,放在lv_init()的后面 */ 
    lv_port_indev_init();         /* lvgl输入接口初始化,放在lv_init()的后面 */ 
	


		static lv_obj_t *switch_cool;            /* 制冷模式开关 */
		static const lv_font_t *font;            /* 定义字体 */

		
 /* 制冷模式基础对象（矩形背景） */
    lv_obj_t *obj_cool = lv_obj_create(lv_scr_act());                               /* 创建基础对象 */
    lv_obj_set_size(obj_cool,scr_act_height() / 3, scr_act_height() / 3 );          /* 设置大小 */
    lv_obj_align(obj_cool, LV_ALIGN_CENTER, -scr_act_width() / 4, 0 );              /* 设置位置 */

    /* 制冷模式开关标签 */
    lv_obj_t *label_cool = lv_label_create(obj_cool);                               /* 创建标签 */
    lv_label_set_text(label_cool, "Cool");                                          /* 设置文本内容 */
    lv_obj_set_style_text_font(label_cool, font, LV_STATE_DEFAULT);                 /* 设置字体 */
    lv_obj_align(label_cool, LV_ALIGN_CENTER, 0, -scr_act_height() / 16 );          /* 设置位置 */

    /* 制冷模式开关 */
    switch_cool = lv_switch_create(obj_cool);                                       /* 创建开关 */
    lv_obj_set_size(switch_cool,scr_act_height() / 6, scr_act_height() / 12 );      /* 设置大小 */
    lv_obj_align(switch_cool, LV_ALIGN_CENTER, 0, scr_act_height() / 16 );          /* 设置位置 */

	  lv_obj_add_event_cb(switch_cool, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

	while(1)
	{
		vTaskDelay(5);
		lv_timer_handler();
	}
}

/* 文件系统任务：负责驱动链接、挂载与一次性读写验证 */
void FS_Process(void *params)
{
  /* 若未在 main 调用过，可在任务中调用一次驱动链接（幂等） */
  /* 稍等其他任务（如 LVGL）先完成初始化，减少竞争 */
  vTaskDelay(pdMS_TO_TICKS(50));
  MX_FATFS_Init();

  FRESULT fr;
  /* 立即挂载卷 */
  fr = f_mount(&SDFatFS, SDPath, 1);
  if (fr == FR_NO_FILESYSTEM) {
    printf("[FS] No FS, mkfs...\r\n");
    static BYTE workbuf[4096];
    /* R0.12c 签名：f_mkfs(path, opt, au, work, len) */
    fr = f_mkfs(SDPath, FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));
    if (fr == FR_OK) {
      fr = f_mount(&SDFatFS, SDPath, 1);
    }
  }
  if (fr == FR_OK) {
    printf("[FS] mount OK on %s\r\n", SDPath);
    
    /* 文件系统已就绪，释放信号量通知其他任务 */
    printf("[FS] Giving semaphore (handle=0x%08lX)...\r\n", (uint32_t)fsReadySemaphore);
    if (fsReadySemaphore != NULL) {
        BaseType_t result = xSemaphoreGive(fsReadySemaphore);
        printf("[FS] Filesystem ready semaphore given (result=%d).\r\n", result);
    } else {
        printf("[FS] ERROR: Semaphore is NULL!\r\n");
    }
  } else {
    printf("[FS] mount failed: %d\r\n", fr);
  }

  /* 任务完成其初始化使命，但保持运行以备将来使用，或挂起 */
  vTaskSuspend(NULL);
}

void Audio_Test_Task(void *params)
{
 /* 等待文件系统就绪 */
    xSemaphoreTake(fsReadySemaphore, portMAX_DELAY);
      
  /* 初始化 WM8978 音频编解码芯片 */
  printf("\r\n------------------------------------------------------------\r\n");
  printf("WM8978 Audio Codec Initialization\r\n");
  printf("------------------------------------------------------------\r\n");
  if (wm8978_Init() == 0)
  {
    printf("[WM8978] ERROR: Chip not detected!\r\n");
    printf("[WM8978] Please check:\r\n");
    printf("         - I2C connections (PB8/PB9)\r\n");
    printf("         - WM8978 power supply\r\n");
    printf("         - Pull-up resistors on I2C lines\r\n");
  }
  else
  {
    printf("[WM8978] Chip detected and initialized successfully\r\n");
  }
    vTaskDelay(pdMS_TO_TICKS(1000));
    /* 播放 MP3 文件 */
    mp3PlayerDemo("0:/mp3/张国荣-玻璃之情.mp3");
    
    /* 播放结束后的清理 */
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    /* 如需循环播放 */
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


/* USER CODE END Application */

