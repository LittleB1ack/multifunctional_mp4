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

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */

  /* USER CODE BEGIN RTOS_THREADS */


  /* ���� LED ���� */
  xTaskCreate(LED_Process,"LED_Task",128,NULL,osPriorityNormal,NULL);
  /* ���� LVGL ���� */
  xTaskCreate(LVGL_Process,"LVGL_Task",512,NULL,osPriorityNormal,NULL);

  /* FS �������ȼ����� LVGL��������ռ UI ��Ⱦ */
  xTaskCreate(FS_Process, "FS_Task", 512, NULL, osPriorityLow, NULL);
  /* ������Ƶ��������Ϊ������ϣ���ʱ���ã� */
  /* xTaskCreate(Audio_Test_Task, "AudioTest", 1024, NULL, 2, NULL); */


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
    /* �򵥶�д��֤ */
    FIL fp;
    UINT bw = 0;
    if (f_open(&fp, "0:/hello.txt", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
      const char *msg = "Hello, FatFs + FreeRTOS!\r\n";
      f_write(&fp, msg, (UINT)strlen(msg), &bw);
      f_close(&fp);
      printf("[FS] write %u bytes\r\n", bw);
    }
    if (f_open(&fp, "0:/hello.txt", FA_READ) == FR_OK) {
      char buf[64] = {0};
      UINT br = 0;
      f_read(&fp, buf, sizeof(buf)-1, &br);
      f_close(&fp);
      printf("[FS] read %u bytes: %s\r\n", br, buf);
    }
  } else {
    printf("[FS] mount failed: %d\r\n", fr);
  }

  /* �ļ�ϵͳ��ʼ��������ɺ������ɾ�� */
  vTaskDelete(NULL);
}

void Audio_Test_Task(void *params)
{
		printf("[AUDIO] ========================================\r\n");
		printf("[AUDIO] I2S-DMA-WM8978 Test Started\r\n");
		printf("[AUDIO] ========================================\r\n");
		
		// ============= Step 1: Reset WM8978 =============
		printf("[AUDIO] Step 1: Resetting WM8978...\r\n");
		wm8978_Reset();
		vTaskDelay(pdMS_TO_TICKS(10));
		printf("[AUDIO] ? WM8978 reset done\r\n");
		
		// ============= Step 2: Configure Audio Path =============
		printf("[AUDIO] Step 2: Configuring audio path...\r\n");
		wm8978_CfgAudioPath(DAC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);
		printf("[AUDIO] ? Audio path configured (DAC -> Headphones)\r\n");
		
		// ============= Step 3: Set Volume =============
		printf("[AUDIO] Step 3: Setting volume...\r\n");
		wm8978_SetOUT1Volume(45);  // ��������Ϊ 45 (0-63)
		printf("[AUDIO] ? Volume set to 45\r\n");
		
		// ============= Step 4: Configure I2S Interface =============
		printf("[AUDIO] Step 4: Configuring I2S interface...\r\n");
		wm8978_CfgAudioIF(I2S_STANDARD_PHILIPS, 16);  // Philips ��׼��16-bit
		printf("[AUDIO] ? I2S interface configured\r\n");
		
		// ============= Step 5: Initialize I2S =============
		printf("[AUDIO] Step 5: Initializing I2S and GPIO...\r\n");
		I2S_Stop();
		I2S_GPIO_Config();
		printf("[AUDIO] ? I2S GPIO initialized\r\n");
		
		printf("[AUDIO] Step 6: Configuring I2S mode...\r\n");
		I2Sx_Mode_Config(I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, I2S_AUDIOFREQ_44K);
		printf("[AUDIO] ? I2S mode configured (44.1kHz)\r\n");
		
		// ============= Step 6: Generate Audio Buffers =============
		printf("[AUDIO] Step 7: Generating audio buffers...\r\n");
		
		// ˫���壬ÿ�� 1152 ������ (44.1kHz �� ~26ms)
		static int16_t sine_buffer[2][1152];
		
		// ���� 1kHz ���Ҳ�
		float phase = 0.0f;
		float phase_step = (2.0f * 3.14159265f * 1000.0f) / 44100.0f;  // 1000Hz
		
		for (int i = 0; i < 1152; i++) {
				int16_t sample = (int16_t)(32000.0f * sinf(phase));
				sine_buffer[0][i] = sample;
				sine_buffer[1][i] = sample;
				phase += phase_step;
				if (phase > 2.0f * 3.14159265f) {
						phase -= 2.0f * 3.14159265f;
				}
		}
		printf("[AUDIO] ? Audio buffers generated (1kHz sine wave)\r\n");
		
		// ============= Step 7: Initialize DMA =============
		printf("[AUDIO] Step 8: Initializing DMA transfer...\r\n");
		I2Sx_TX_DMA_Init((uint32_t)&sine_buffer[0][0], 
										 (uint32_t)&sine_buffer[1][0], 
										 1152);
		printf("[AUDIO] ? DMA initialized\r\n");
		
		// ============= Step 8: Start Playback =============
		printf("[AUDIO] Step 9: Starting playback...\r\n");
		I2S_Play_Start();
		printf("[AUDIO] ? Playback started\r\n");
		printf("[AUDIO] ========================================\r\n");
		printf("[AUDIO] Playing 1kHz tone for 10 seconds...\r\n");
		printf("[AUDIO] Please listen carefully!\r\n");
		printf("[AUDIO] ========================================\r\n");
		
		// ���� 10 ��
		vTaskDelay(pdMS_TO_TICKS(10000));
		
		// ============= Step 9: Stop Playback =============
		printf("[AUDIO] ========================================\r\n");
		printf("[AUDIO] Stopping playback...\r\n");
		I2S_Play_Stop();
		wm8978_OutMute(1);  // �ر����
		printf("[AUDIO] ? Playback stopped\r\n");
		printf("[AUDIO] ========================================\r\n");
		printf("[AUDIO] Test completed!\r\n");
		printf("[AUDIO] ========================================\r\n");
		
		// ɾ������
		vTaskDelete(NULL);
}


/* USER CODE END Application */

