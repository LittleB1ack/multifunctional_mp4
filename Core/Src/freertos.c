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
#include "stdio.h"
#include "string.h"

/*FreeRTOS driver */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "queue.h"

/*hardware driver */
#include "./led/led.h"


/*lvgl driver */
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "../generated/gui_guider.h"
#include "../generated/events_init.h"

/*fatfs driver */
#include "fatfs.h"
#include "ff.h"

/*audio driver and mp3 decoder*/
#include "./wm8978/bsp_wm8978.h"
#include "mp3Player.h"
#include "math.h"

#define scr_act_width() lv_obj_get_width(lv_scr_act())
#define scr_act_height() lv_obj_get_height(lv_scr_act())


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
 // xTaskCreate(LED_Process,"LED_Task",128,NULL,osPriorityBelowNormal,NULL);
  /* ���� LVGL ���� - �������ȼ�,����������Ƶ */
  xTaskCreate(LVGL_Process,"LVGL_Task",2048,NULL,osPriorityNormal,NULL);

  /* FS �������ȼ���Ϊ osPriorityAboveNormal��ȷ����������ļ�ϵͳ��ʼ�� */
//  xTaskCreate(FS_Process, "FS_Task", 512, NULL, osPriorityAboveNormal, NULL);
  /* ������Ƶ���� - ������ȼ���Normal,ȷ��ʵʱ�� */
//  xTaskCreate(Audio_Test_Task, "AudioTest", 2048, NULL, osPriorityNormal, NULL); 


  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

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
    printf("[LVGL] Initializing LVGL...\r\n");
    
    /* �� lv_init ֮ǰ�����ⲿ SDRAM 0x6C030000 ���� */
    printf("[LVGL] Testing SDRAM at 0x6C030000...\r\n");
    volatile uint32_t *sdram_test = (volatile uint32_t *)0x6C030000;
    *sdram_test = 0xDEADBEEF;
    uint32_t readback = *sdram_test;
    if (readback == 0xDEADBEEF) {
        printf("[LVGL] SDRAM test OK (wrote 0xDEADBEEF, read 0x%08lX)\r\n", (unsigned long)readback);
    } else {
        printf("[LVGL] SDRAM test FAILED! (wrote 0xDEADBEEF, read 0x%08lX)\r\n", (unsigned long)readback);
        printf("[LVGL] ERROR: Cannot initialize LVGL - memory pool unavailable\r\n");
        vTaskSuspend(NULL);  /* �������� */
    }
    
    lv_init();                    /* lvglϵͳ��ʼ�� */ 
    printf("[LVGL] lv_init() done\r\n");
    
    /* ��ӡ���õ� LVGL �ڴ��ַ */
    #if LV_MEM_ADR != 0
    printf("[LVGL] Memory pool configured at: 0x%08X (size: %lu KB)\r\n", 
           (unsigned int)LV_MEM_ADR, (unsigned long)(LV_MEM_SIZE / 1024));
    #else
    printf("[LVGL] Memory pool using internal allocation (size: %lu KB)\r\n", 
           (unsigned long)(LV_MEM_SIZE / 1024));
    #endif

    lv_port_disp_init();          /* lvgl��ʾ�ӿڳ�ʼ��,����lv_init()�ĺ��� */ 
    printf("[LVGL] Display initialized\r\n");
    
    lv_port_indev_init();         /* lvgl����ӿڳ�ʼ��,����lv_init()�ĺ��� */ 
    printf("[LVGL] Input device initialized\r\n");
	
    /* �����ڴ��� - ����ʹ���ڲ�RAMӦ�ÿ��Թ��� */
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    printf("[LVGL] Memory: total=%lu, free=%lu, max_used=%lu, frag=%d%%\r\n", 
           (unsigned long)mon.total_size, (unsigned long)mon.free_size, 
           (unsigned long)mon.max_used, mon.frag_pct);
	
	printf("[LVGL] Calling setup_ui()...\r\n");
	
    /* ?? ԭGUI����Ż�˵��:
     * �������:
     *  1. ��Ļ�ߴ����ô���(480x800Ӧ����800x480) ? ���޸�
     *  2. ͼƬʹ��Alphaͨ��ռ�ù����ڴ�(30KB) ? ���Ż�ΪRGB565
     *  3. 32KB�ڴ�ض�������UI���Բ��� ? �Ѽ����
     * 
     * ��ǰ�Ż���UI����:
     *  - �������� 800x480 ����NT35510
     *  - ���䱳����ǿ�Ӿ�Ч��
     *  - ��Ƭʽ��Ϣչʾ�����׶�
     *  - ����ؼ�������ʡ�ڴ�(<20KB)
     *  - ����������Ϣ�ͽ�������
     */
    
    printf("[LVGL] Creating optimized UI (32KB memory, 800x480 landscape)...\r\n");
    
    lv_obj_t * scr = lv_scr_act();
    
    /* === ����������� === */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1E3A5F), 0);           // ����ɫ
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x2E5090), 0);     // ǳ��ɫ
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    
    /* === ���������� === */
    lv_obj_t * title_bar = lv_obj_create(scr);
    lv_obj_set_size(title_bar, 800, 60);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x2195f6), 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t * title = lv_label_create(title_bar);
    lv_label_set_text(title, "Personal Information");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserratMedium_24, 0);
    lv_obj_center(title);
    
    /* === �����Ϣ��Ƭ === */
    lv_obj_t * card_left = lv_obj_create(scr);
    lv_obj_set_size(card_left, 360, 360);
    lv_obj_set_pos(card_left, 30, 80);
    lv_obj_set_style_bg_color(card_left, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(card_left, 230, 0);  // ��͸��
    lv_obj_set_style_border_width(card_left, 2, 0);
    lv_obj_set_style_border_color(card_left, lv_color_hex(0x3498db), 0);
    lv_obj_set_style_radius(card_left, 15, 0);
    lv_obj_set_style_shadow_width(card_left, 20, 0);
    lv_obj_set_style_shadow_color(card_left, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card_left, 100, 0);
    lv_obj_clear_flag(card_left, LV_OBJ_FLAG_SCROLLABLE);
    
    /* ������Ϣ���� */
    lv_obj_t * label_basic = lv_label_create(card_left);
    lv_label_set_text(label_basic, "Basic Info");
    lv_obj_set_style_text_color(label_basic, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_text_font(label_basic, &lv_font_montserratMedium_20, 0);
    lv_obj_set_pos(label_basic, 20, 15);
    
    /* ������Ϣ���� */
    lv_obj_t * label_info = lv_label_create(card_left);
    lv_label_set_text(label_info, 
        "Name:      Black\n"
        "Gender:    Male\n"
        "Age:       21\n"
        "Location:  GuangDong");
    lv_obj_set_style_text_color(label_info, lv_color_hex(0x34495E), 0);
    lv_obj_set_style_text_font(label_info, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_text_line_space(label_info, 8, 0);
    lv_obj_set_pos(label_info, 20, 60);
    
    /* �ָ��� */
    lv_obj_t * line_sep = lv_obj_create(card_left);
    lv_obj_set_size(line_sep, 320, 2);
    lv_obj_set_pos(line_sep, 20, 200);
    lv_obj_set_style_bg_color(line_sep, lv_color_hex(0xBDC3C7), 0);
    lv_obj_set_style_border_width(line_sep, 0, 0);
    
    /* ���ܱ�ǩ */
    lv_obj_t * label_tech = lv_label_create(card_left);
    lv_label_set_text(label_tech, "Skills");
    lv_obj_set_style_text_color(label_tech, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_text_font(label_tech, &lv_font_montserratMedium_20, 0);
    lv_obj_set_pos(label_tech, 20, 220);
    
    lv_obj_t * label_skills = lv_label_create(card_left);
    lv_label_set_text(label_skills, "EDA Design\nFirmware Dev");
    lv_obj_set_style_text_color(label_skills, lv_color_hex(0x34495E), 0);
    lv_obj_set_style_text_font(label_skills, &lv_font_montserratMedium_16, 0);
    lv_obj_set_pos(label_skills, 20, 260);
    
    /* === �Ҳ���Ȥ��Ƭ === */
    lv_obj_t * card_right = lv_obj_create(scr);
    lv_obj_set_size(card_right, 360, 360);
    lv_obj_set_pos(card_right, 410, 80);
    lv_obj_set_style_bg_color(card_right, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(card_right, 230, 0);
    lv_obj_set_style_border_width(card_right, 2, 0);
    lv_obj_set_style_border_color(card_right, lv_color_hex(0x3498db), 0);
    lv_obj_set_style_radius(card_right, 15, 0);
    lv_obj_set_style_shadow_width(card_right, 20, 0);
    lv_obj_set_style_shadow_color(card_right, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card_right, 100, 0);
    lv_obj_clear_flag(card_right, LV_OBJ_FLAG_SCROLLABLE);
    
    /* ���ñ��� */
    lv_obj_t * label_hobby_title = lv_label_create(card_right);
    lv_label_set_text(label_hobby_title, "Hobbies");
    lv_obj_set_style_text_color(label_hobby_title, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_text_font(label_hobby_title, &lv_font_montserratMedium_20, 0);
    lv_obj_set_pos(label_hobby_title, 20, 15);
    
    /* �����б� */
    lv_obj_t * label_hobbies = lv_label_create(card_right);
    lv_label_set_text(label_hobbies, 
        LV_SYMBOL_OK " Basketball\n\n"
        LV_SYMBOL_OK " Swimming\n\n"
        LV_SYMBOL_OK " Coding\n\n"
        LV_SYMBOL_OK " Reading");
    lv_obj_set_style_text_color(label_hobbies, lv_color_hex(0x27AE60), 0);
    lv_obj_set_style_text_font(label_hobbies, &lv_font_montserratMedium_16, 0);
    lv_obj_set_style_text_line_space(label_hobbies, 5, 0);
    lv_obj_set_pos(label_hobbies, 30, 60);
    
    /* �ײ�״̬�� */
    lv_obj_t * status_bar = lv_obj_create(scr);
    lv_obj_set_size(status_bar, 360, 40);
    lv_obj_set_pos(status_bar, 410, 450);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x27AE60), 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 8, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t * status_label = lv_label_create(status_bar);
    lv_label_set_text(status_label, LV_SYMBOL_OK " System Ready");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserratMedium_16, 0);
    lv_obj_center(status_label);
    
    printf("[LVGL] Optimized UI created successfully!\r\n");
    
    lv_mem_monitor(&mon);
    printf("[LVGL] Memory usage: total=%lu, free=%lu, used=%lu\r\n", 
           (unsigned long)mon.total_size, 
           (unsigned long)mon.free_size,
           (unsigned long)(mon.total_size - mon.free_size));
    
    if(mon.free_size < 2048) {
        printf("[LVGL] WARNING: Low memory! Only %lu bytes free\r\n", 
               (unsigned long)mon.free_size);
    }    /* UI�������,���ڿ���������ʾˢ�� */
    printf("[LVGL] Enabling display flush...\r\n");
    disp_enable_update();  /* ����ˢ�� */
    printf("[LVGL] Display flush enabled\r\n");
    
    printf("[LVGL] UI created, entering main loop...\r\n");

    uint32_t loop_count = 0;
    while(1)
    {
        lv_timer_handler();  /* ����LVGL��ʱ�� */
        
        loop_count++;
        /* ÿ100��ѭ�����һ���ڴ� */
        if(loop_count % 100 == 0) {
            lv_mem_monitor_t mon;
            lv_mem_monitor(&mon);
            printf("[LOOP] #%lu: free=%lu, max_used=%lu, frag=%d%%\r\n", 
                   (unsigned long)loop_count, (unsigned long)mon.free_size, 
                   (unsigned long)mon.max_used, mon.frag_pct);
            
            if(mon.free_size < 1024) {
                printf("[LOOP] WARNING: Low memory! free=%lu bytes\r\n", (unsigned long)mon.free_size);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));  /* 10ms�ӳ�,����ռ�ù���CPU */
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

