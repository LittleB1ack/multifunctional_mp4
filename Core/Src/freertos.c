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
static QueueHandle_t ledQueue;  /* ȫ�ֶ��У����� LVGL -> LED ������״̬ */
static SemaphoreHandle_t fsReadySemaphore; /* �ļ�ϵͳ�����ź��� */
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

  /* ���� LED ״̬���У��ڴ�������ǰ�� */
  ledQueue = xQueueCreate(4, sizeof(uint8_t));
  fsReadySemaphore = xSemaphoreCreateBinary(); /* �����ļ�ϵͳ�����ź��� */

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */

  /* USER CODE BEGIN RTOS_THREADS */


  /* ���� LED ���� */
  xTaskCreate(LED_Process,"LED_Task",128,NULL,osPriorityNormal,NULL);
  /* ���� LVGL ���� */
  xTaskCreate(LVGL_Process,"LVGL_Task",512,NULL,osPriorityNormal,NULL);

  /* FS �������ȼ���Ϊ osPriorityAboveNormal��ȷ����������ļ�ϵͳ��ʼ�� */
  xTaskCreate(FS_Process, "FS_Task", 512, NULL, osPriorityAboveNormal, NULL);
  /* ������Ƶ��������ջ��С��2048�����ȼ���Ϊ osPriorityNormal���ȴ��ļ�ϵͳ������������ */
  xTaskCreate(Audio_Test_Task, "AudioTest", 2048, NULL, osPriorityBelowNormal, NULL); 


  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* LVGL �����¼��ص���LVGL v8��*/
static void switch_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t *sw = lv_event_get_target(e);
    uint8_t state = lv_obj_has_state(sw, LV_STATE_CHECKED) ? On : Off; /* ʹ�� led.h �е� On/Off */
    if(ledQueue) {
        xQueueSend(ledQueue, &state, 0); /* ���������� */
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
    lv_init();                    /* lvglϵͳ��ʼ�� */ 

    lv_port_disp_init();          /* lvgl��ʾ�ӿڳ�ʼ��,����lv_init()�ĺ��� */ 
    lv_port_indev_init();         /* lvgl����ӿڳ�ʼ��,����lv_init()�ĺ��� */ 
	


		static lv_obj_t *switch_cool;            /* ����ģʽ���� */
		static const lv_font_t *font;            /* �������� */

		
 /* ����ģʽ�������󣨾��α����� */
    lv_obj_t *obj_cool = lv_obj_create(lv_scr_act());                               /* ������������ */
    lv_obj_set_size(obj_cool,scr_act_height() / 3, scr_act_height() / 3 );          /* ���ô�С */
    lv_obj_align(obj_cool, LV_ALIGN_CENTER, -scr_act_width() / 4, 0 );              /* ����λ�� */

    /* ����ģʽ���ر�ǩ */
    lv_obj_t *label_cool = lv_label_create(obj_cool);                               /* ������ǩ */
    lv_label_set_text(label_cool, "Cool");                                          /* �����ı����� */
    lv_obj_set_style_text_font(label_cool, font, LV_STATE_DEFAULT);                 /* �������� */
    lv_obj_align(label_cool, LV_ALIGN_CENTER, 0, -scr_act_height() / 16 );          /* ����λ�� */

    /* ����ģʽ���� */
    switch_cool = lv_switch_create(obj_cool);                                       /* �������� */
    lv_obj_set_size(switch_cool,scr_act_height() / 6, scr_act_height() / 12 );      /* ���ô�С */
    lv_obj_align(switch_cool, LV_ALIGN_CENTER, 0, scr_act_height() / 16 );          /* ����λ�� */

	  lv_obj_add_event_cb(switch_cool, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

	while(1)
	{
		vTaskDelay(5);
		lv_timer_handler();
	}
}

/* �ļ�ϵͳ���񣺸����������ӡ�������һ���Զ�д��֤ */
void FS_Process(void *params)
{
  /* ��δ�� main ���ù������������е���һ���������ӣ��ݵȣ� */
  /* �Ե����������� LVGL������ɳ�ʼ�������پ��� */
  vTaskDelay(pdMS_TO_TICKS(50));
  MX_FATFS_Init();

  FRESULT fr;
  /* �������ؾ� */
  fr = f_mount(&SDFatFS, SDPath, 1);
  if (fr == FR_NO_FILESYSTEM) {
    printf("[FS] No FS, mkfs...\r\n");
    static BYTE workbuf[4096];
    /* R0.12c ǩ����f_mkfs(path, opt, au, work, len) */
    fr = f_mkfs(SDPath, FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));
    if (fr == FR_OK) {
      fr = f_mount(&SDFatFS, SDPath, 1);
    }
  }
  if (fr == FR_OK) {
    printf("[FS] mount OK on %s\r\n", SDPath);
    
    /* �ļ�ϵͳ�Ѿ������ͷ��ź���֪ͨ�������� */
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

  /* ����������ʼ��ʹ���������������Ա�����ʹ�ã������ */
  vTaskSuspend(NULL);
}

void Audio_Test_Task(void *params)
{
 /* �ȴ��ļ�ϵͳ���� */
    xSemaphoreTake(fsReadySemaphore, portMAX_DELAY);
      
  /* ��ʼ�� WM8978 ��Ƶ�����оƬ */
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
    /* ���� MP3 �ļ� */
    mp3PlayerDemo("0:/mp3/�Ź���-����֮��.mp3");
    
    /* ���Ž���������� */
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    /* ����ѭ������ */
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


/* USER CODE END Application */

