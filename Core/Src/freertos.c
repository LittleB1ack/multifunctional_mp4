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
 // xTaskCreate(LED_Process,"LED_Task",128,NULL,osPriorityBelowNormal,NULL);
  /* 创建 LVGL 任务 - 降低优先级,避免阻塞音频 */
  xTaskCreate(LVGL_Process,"LVGL_Task",2048,NULL,osPriorityNormal,NULL);

  /* FS 任务优先级设为 osPriorityAboveNormal，确保优先完成文件系统初始化 */
//  xTaskCreate(FS_Process, "FS_Task", 512, NULL, osPriorityAboveNormal, NULL);
  /* 创建音频任务 - 提高优先级到Normal,确保实时性 */
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
    
    /* 在 lv_init 之前测试外部 SDRAM 0x6C030000 区域 */
    printf("[LVGL] Testing SDRAM at 0x6C030000...\r\n");
    volatile uint32_t *sdram_test = (volatile uint32_t *)0x6C030000;
    *sdram_test = 0xDEADBEEF;
    uint32_t readback = *sdram_test;
    if (readback == 0xDEADBEEF) {
        printf("[LVGL] SDRAM test OK (wrote 0xDEADBEEF, read 0x%08lX)\r\n", (unsigned long)readback);
    } else {
        printf("[LVGL] SDRAM test FAILED! (wrote 0xDEADBEEF, read 0x%08lX)\r\n", (unsigned long)readback);
        printf("[LVGL] ERROR: Cannot initialize LVGL - memory pool unavailable\r\n");
        vTaskSuspend(NULL);  /* 挂起任务 */
    }
    
    lv_init();                    /* lvgl系统初始化 */ 
    printf("[LVGL] lv_init() done\r\n");
    
    /* 打印配置的 LVGL 内存地址 */
    #if LV_MEM_ADR != 0
    printf("[LVGL] Memory pool configured at: 0x%08X (size: %lu KB)\r\n", 
           (unsigned int)LV_MEM_ADR, (unsigned long)(LV_MEM_SIZE / 1024));
    #else
    printf("[LVGL] Memory pool using internal allocation (size: %lu KB)\r\n", 
           (unsigned long)(LV_MEM_SIZE / 1024));
    #endif

    lv_port_disp_init();          /* lvgl显示接口初始化,放在lv_init()的后面 */ 
    printf("[LVGL] Display initialized\r\n");
    
    lv_port_indev_init();         /* lvgl输入接口初始化,放在lv_init()的后面 */ 
    printf("[LVGL] Input device initialized\r\n");
	
    /* 启用内存监控 - 现在使用内部RAM应该可以工作 */
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    printf("[LVGL] Memory: total=%lu, free=%lu, max_used=%lu, frag=%d%%\r\n", 
           (unsigned long)mon.total_size, (unsigned long)mon.free_size, 
           (unsigned long)mon.max_used, mon.frag_pct);
	
	printf("[LVGL] Calling setup_ui()...\r\n");
	
    /* ?? 原GUI设计优化说明:
     * 问题诊断:
     *  1. 屏幕尺寸配置错误(480x800应该是800x480) ? 已修复
     *  2. 图片使用Alpha通道占用过多内存(30KB) ? 已优化为RGB565
     *  3. 32KB内存池对于完整UI略显不足 ? 已简化设计
     * 
     * 当前优化版UI特性:
     *  - 横屏布局 800x480 适配NT35510
     *  - 渐变背景增强视觉效果
     *  - 卡片式信息展示清晰易读
     *  - 精简控件数量节省内存(<20KB)
     *  - 保留核心信息和交互功能
     */
    
    printf("[LVGL] Creating optimized UI (32KB memory, 800x480 landscape)...\r\n");
    
    lv_obj_t * scr = lv_scr_act();
    
    /* === 背景渐变设计 === */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1E3A5F), 0);           // 深蓝色
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x2E5090), 0);     // 浅蓝色
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    
    /* === 顶部标题栏 === */
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
    
    /* === 左侧信息卡片 === */
    lv_obj_t * card_left = lv_obj_create(scr);
    lv_obj_set_size(card_left, 360, 360);
    lv_obj_set_pos(card_left, 30, 80);
    lv_obj_set_style_bg_color(card_left, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(card_left, 230, 0);  // 半透明
    lv_obj_set_style_border_width(card_left, 2, 0);
    lv_obj_set_style_border_color(card_left, lv_color_hex(0x3498db), 0);
    lv_obj_set_style_radius(card_left, 15, 0);
    lv_obj_set_style_shadow_width(card_left, 20, 0);
    lv_obj_set_style_shadow_color(card_left, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card_left, 100, 0);
    lv_obj_clear_flag(card_left, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 基本信息标题 */
    lv_obj_t * label_basic = lv_label_create(card_left);
    lv_label_set_text(label_basic, "Basic Info");
    lv_obj_set_style_text_color(label_basic, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_text_font(label_basic, &lv_font_montserratMedium_20, 0);
    lv_obj_set_pos(label_basic, 20, 15);
    
    /* 基本信息内容 */
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
    
    /* 分割线 */
    lv_obj_t * line_sep = lv_obj_create(card_left);
    lv_obj_set_size(line_sep, 320, 2);
    lv_obj_set_pos(line_sep, 20, 200);
    lv_obj_set_style_bg_color(line_sep, lv_color_hex(0xBDC3C7), 0);
    lv_obj_set_style_border_width(line_sep, 0, 0);
    
    /* 技能标签 */
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
    
    /* === 右侧兴趣卡片 === */
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
    
    /* 爱好标题 */
    lv_obj_t * label_hobby_title = lv_label_create(card_right);
    lv_label_set_text(label_hobby_title, "Hobbies");
    lv_obj_set_style_text_color(label_hobby_title, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_text_font(label_hobby_title, &lv_font_montserratMedium_20, 0);
    lv_obj_set_pos(label_hobby_title, 20, 15);
    
    /* 爱好列表 */
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
    
    /* 底部状态栏 */
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
    }    /* UI创建完成,现在可以启用显示刷新 */
    printf("[LVGL] Enabling display flush...\r\n");
    disp_enable_update();  /* 启用刷新 */
    printf("[LVGL] Display flush enabled\r\n");
    
    printf("[LVGL] UI created, entering main loop...\r\n");

    uint32_t loop_count = 0;
    while(1)
    {
        lv_timer_handler();  /* 处理LVGL定时器 */
        
        loop_count++;
        /* 每100次循环检查一次内存 */
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
        
        vTaskDelay(pdMS_TO_TICKS(10));  /* 10ms延迟,避免占用过多CPU */
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

