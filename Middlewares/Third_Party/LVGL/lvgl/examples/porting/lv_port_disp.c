/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "../../lvgl.h"
#include <stdbool.h>
#include <stdio.h>
#include "bsp_nt35510_lcd.h"
#include "FreeRTOS.h"                     /* FreeRTOS 核心 */
#include "task.h"                         /* FreeRTOS 任务 */
#include "./sdram/mem_placement.h"        /* 外部 SRAM 内存放置宏 - 必须最后 */

/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
    #define MY_DISP_HOR_RES    800
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen height, default value 240 is used for now.
    #define MY_DISP_VER_RES    480
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

//    /* Example for 1) */
//    static lv_disp_draw_buf_t draw_buf_dsc_1;
//    static lv_color_t buf_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
//    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    /* Example for 2) */
    static lv_disp_draw_buf_t draw_buf_dsc_2;
    static EXTSRAM lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];  /* 改为10行,加快刷新速度 */
    static EXTSRAM lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];  /* 改为10行,加快刷新速度 */
    
    /* 打印缓冲区地址，验证是否在外部SDRAM */
    printf("[DISP] buf_2_1 at: 0x%08X (size: %lu bytes)\r\n", 
           (unsigned int)buf_2_1, (unsigned long)sizeof(buf_2_1));
    printf("[DISP] buf_2_2 at: 0x%08X (size: %lu bytes)\r\n", 
           (unsigned int)buf_2_2, (unsigned long)sizeof(buf_2_2));
    
    /* 测试外部SDRAM访问 - 直接写入字节 */
    printf("[DISP] Testing EXTSRAM access...\r\n");
    volatile uint16_t *test_ptr1 = (volatile uint16_t *)buf_2_1;
    volatile uint16_t *test_ptr2 = (volatile uint16_t *)buf_2_2;
    test_ptr1[0] = 0x1234;
    test_ptr2[0] = 0x5678;
    if (test_ptr1[0] == 0x1234 && test_ptr2[0] == 0x5678) {
        printf("[DISP] EXTSRAM access OK\r\n");
    } else {
        printf("[DISP] EXTSRAM access FAILED! (read: 0x%04X, 0x%04X)\r\n", 
               test_ptr1[0], test_ptr2[0]);
    }
    
    printf("[DISP] Initializing draw buffer...\r\n");
    lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);   /* 改为10行 */
    printf("[DISP] Draw buffer initialized\r\n");

//    /* Example for 3) also set disp_drv.full_refresh = 1 below*/
//    static lv_disp_draw_buf_t draw_buf_dsc_3;
//    static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
//    static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
//    lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
//                          MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_2;

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    printf("[DISP] Registering display driver...\r\n");
    printf("[DISP] Driver config: %dx%d, flush_cb=0x%08X, draw_buf=0x%08X\r\n",
           disp_drv.hor_res, disp_drv.ver_res, 
           (unsigned int)disp_drv.flush_cb, (unsigned int)disp_drv.draw_buf);
    printf("[DISP] Note: flush is DISABLED to avoid blocking\r\n");
    printf("[DISP] Calling lv_disp_drv_register...\r\n");
    
    /* 注册显示驱动 - flush回调会立即返回(disp_flush_enabled=false) */
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    
    if (disp) {
        printf("[DISP] Display driver registered successfully (disp=0x%08X)\r\n", (unsigned int)disp);
    } else {
        printf("[DISP] ERROR: lv_disp_drv_register returned NULL!\r\n");
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    printf("[DISP] Initializing NT35510 LCD...\r\n");
    NT35510_Init();
    printf("[DISP] NT35510 initialized successfully\r\n");
    /*You code here*/
}

volatile bool disp_flush_enabled = false;  /* 初始禁用,避免耗时刷新 */

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    static uint32_t flush_count = 0;
    flush_count++;
    
    /* 只打印前30次和关键位置 */
    if(flush_count <= 30 || flush_count % 10 == 0) {
        /* 立即打印,确认函数被调用 */
        printf("[FLUSH_ENTER] #%u\r\n", (unsigned int)flush_count);
        
        /* 打印刷新信息 */
        uint32_t width = area->x2 - area->x1 + 1;
        uint32_t height = area->y2 - area->y1 + 1;
        uint32_t pixels = width * height;
        
        printf("[FLUSH] #%u: (%d,%d)-(%d,%d) %ux%u=%u px ", 
               (unsigned int)flush_count, 
               area->x1, area->y1, area->x2, area->y2,
               (unsigned int)width, (unsigned int)height, (unsigned int)pixels);
    }
    
    /* 检查刷新是否启用 */
    if(!disp_flush_enabled) {
        if(flush_count <= 30 || flush_count % 10 == 0) {
            printf("SKIP\r\n");
        }
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    if(flush_count <= 30 || flush_count % 10 == 0) {
        printf("START\r\n");
    }
    
    /* 逐像素刷新 */
    int32_t x, y;
    uint32_t count = 0;
    uint32_t start_tick = xTaskGetTickCount();  /* 使用 FreeRTOS 计时 */
    
    for(y = area->y1; y <= area->y2; y++)
    {
        for(x = area->x1; x <= area->x2; x++)
        {
            NT35510_SetColorPointPixel(x, y, color_p->full);
            color_p++;
            count++;
            
            /* 每1000个像素让出CPU,避免看门狗 */
            if(count % 1000 == 0) {
                if(flush_count <= 30 || flush_count % 10 == 0) {
                    printf(".");
                }
                vTaskDelay(1);  /* 1ms延迟 */
            }
        }
    }
    
    uint32_t elapsed = xTaskGetTickCount() - start_tick;
    if(flush_count <= 30 || flush_count % 10 == 0) {
        printf("\r\n[FLUSH] #%u: OK (wrote %u pixels in %u ms)\r\n", 
               (unsigned int)flush_count, (unsigned int)count, (unsigned int)elapsed);
    }
    
    /* 通知LVGL刷新完成 */
    lv_disp_flush_ready(disp_drv);
    
    if(flush_count <= 30 || flush_count % 10 == 0) {
        printf("[FLUSH_EXIT] #%u\r\n", (unsigned int)flush_count);
    }
}

/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}


#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
