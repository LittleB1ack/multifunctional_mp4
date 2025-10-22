/**
 ******************************************************************************
 * @file    extsram_migration.c
 * @brief   外部 SRAM 迁移示例
 *          展示如何给关键缓冲添加 EXTSRAM 标记并验证
 ******************************************************************************
 * 
 * 使用步骤：
 * 1. 在工程中包含本文件
 * 2. 在 main() 中调用示例函数进行验证
 * 3. 查看 map 文件，确认地址落在 0x6Cxxxxxx
 * 4. 观察性能与稳定性
 * 
 ******************************************************************************
 */

#include "./sdram/bsp_sdram.h"
#include <stdio.h>
#include <stdint.h>

/* ============================================================================
   EXTSRAM 内存布局宏定义（来自 mem_placement.h）
   ============================================================================ */

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
  /* ARM Compiler 5/6 (Keil MDK) */
  #define EXTSRAM           __attribute__((section("extsram"), zero_init))
  #define CCMRAM            __attribute__((section("ccmram"), zero_init))
  #define INTRAM            __attribute__((section(".bss")))
  #define NOINIT_EXTSRAM    __attribute__((section("extsram_noinit"), zero_init))
  
#elif defined(__GNUC__)
  /* GCC / ARM GCC */
  #define EXTSRAM           __attribute__((section(".extsram")))
  #define CCMRAM            __attribute__((section(".ccmram")))
  #define INTRAM            __attribute__((section(".bss")))
  #define NOINIT_EXTSRAM    __attribute__((section(".extsram_noinit")))
  
#elif defined(__ICCARM__)
  /* IAR Compiler */
  #define EXTSRAM           _Pragma("location=\"extsram\"")
  #define CCMRAM            _Pragma("location=\"ccmram\"")
  #define INTRAM            /* 默认 */
  #define NOINIT_EXTSRAM    _Pragma("location=\"extsram_noinit\"")
  
#else
  /* 未知编译器 */
  #define EXTSRAM
  #define CCMRAM
  #define INTRAM
  #define NOINIT_EXTSRAM
#endif


/* ============================================================================
   示例 1：LVGL 绘制缓冲
   ============================================================================
   
   原本在 Middlewares/LVGL/GUI/lvgl/examples/porting/lv_port_disp_template.c
   中的 buf_1 会占用内部 SRAM（典型 800*480*2 = 768KB）。
   
   迁移后，释放约 750KB 内部 SRAM。
*/

#define MY_DISP_HOR_RES  800
#define MY_DISP_VER_RES  480

/* 原：
   static lv_color_t buf_1[MY_DISP_HOR_RES * 20];
   
   改为：
*/
static EXTSRAM uint16_t lvgl_draw_buf_demo[MY_DISP_HOR_RES * 20];


/* ============================================================================
   示例 2：MP3/Helix 解码缓冲
   ============================================================================
   
   Helix MP3 解码器使用大量工作缓冲，通常占用 40~100KB。
   这些缓冲可安全迁移到外部 SRAM（不涉及 DMA）。
*/

/* 模拟 Helix 的主数据缓冲（实际应参考 Helix 源码） */
static EXTSRAM int32_t helix_main_data_buf[4096];
static EXTSRAM int16_t helix_output_buf[1152];    /* MP3 帧大小 */


/* ============================================================================
   示例 3：文件 I/O 缓冲（可选）
   ============================================================================
   
   用于 FatFs 读写的临时缓冲。若频繁进行文件操作，可放入外部 SRAM 以释放
   内部 SRAM 给其他关键功能（如 FreeRTOS 任务栈、音频 DMA 缓冲等）。
*/

static EXTSRAM uint8_t file_io_buf[64 * 1024];    /* 64KB 文件读写缓冲 */


/* ============================================================================
   示例 4：持久化工作区（上电不清零）
   ============================================================================
   
   若需要在掉电前保存状态并在上电恢复，可使用 NOINIT 区。
   本示例仅展示声明方式，实际需在 .sct 中定义 extsram_noinit 区段。
*/

/* 保留该注释，实际使用需 .sct 中配置支持
   static NOINIT_EXTSRAM uint32_t persistent_flags[64];
*/


/* ============================================================================
   示例 5：DMA 缓冲（反面教材 - 不应迁移）
   ============================================================================
   
   I2S、SDIO、SPI DMA 等使用的缓冲必须保留在内部 SRAM，
   因为外设 DMA 不支持通过 FSMC 访问外部存储器（或受限）。
*/

/* 错误示例（注释）：
   static EXTSRAM int16_t i2s_dma_buf[2][1152];  // ❌ 错误！
   
   正确做法：
*/
static uint16_t i2s_dma_buf[2][1152];  /* 保留在内部 SRAM */


/* ============================================================================
   验证与诊断函数
   ============================================================================ */

/**
 * @brief  打印所有示例缓冲的内存地址与所在区域
 */
void extsram_migration_print_layout(void)
{
  printf("\r\n");
  printf("========== EXTSRAM Migration - Memory Layout ==========\r\n");
  printf("\r\n");

  printf("LVGL 绘制缓冲：\r\n");
  SRAM_PRINT_ADDR("  lvgl_draw_buf_demo", lvgl_draw_buf_demo);
  printf("  大小: %u bytes, 期望区域: EXTSRAM\r\n", (unsigned int)sizeof(lvgl_draw_buf_demo));

  printf("\r\nHelix MP3 解码缓冲：\r\n");
  SRAM_PRINT_ADDR("  helix_main_data_buf", helix_main_data_buf);
  printf("  大小: %u bytes\r\n", (unsigned int)sizeof(helix_main_data_buf));
  SRAM_PRINT_ADDR("  helix_output_buf", helix_output_buf);
  printf("  大小: %u bytes\r\n", (unsigned int)sizeof(helix_output_buf));

  printf("\r\n文件 I/O 缓冲：\r\n");
  SRAM_PRINT_ADDR("  file_io_buf", file_io_buf);
  printf("  大小: %u bytes, 期望区域: EXTSRAM\r\n", (unsigned int)sizeof(file_io_buf));

  printf("\r\n音频 DMA 缓冲（保留在内部 SRAM）：\r\n");
  SRAM_PRINT_ADDR("  i2s_dma_buf", i2s_dma_buf);
  printf("  大小: %u bytes, 期望区域: INTRAM\r\n", (unsigned int)sizeof(i2s_dma_buf));

  printf("\r\n=========================================================\r\n");
  printf("\r\n内存占用统计：\r\n");
  printf("  LVGL buffer:         ~%6u KB (EXTSRAM)\r\n", (unsigned int)(sizeof(lvgl_draw_buf_demo) / 1024));
  printf("  Helix buffers:       ~%6u KB (EXTSRAM)\r\n",
         (unsigned int)((sizeof(helix_main_data_buf) + sizeof(helix_output_buf)) / 1024));
  printf("  File I/O buffer:     ~%6u KB (EXTSRAM)\r\n", (unsigned int)(sizeof(file_io_buf) / 1024));
  printf("  I2S DMA buffer:      ~%6u KB (INTRAM, 必须)\r\n", (unsigned int)(sizeof(i2s_dma_buf) / 1024));
  printf("\r\n总外部 SRAM 使用:     ~%6u KB / 1024 KB\r\n",
         (unsigned int)((sizeof(lvgl_draw_buf_demo) + sizeof(helix_main_data_buf) +
          sizeof(helix_output_buf) + sizeof(file_io_buf)) / 1024));
  printf("总内部 SRAM 节省:     ~%6u KB\r\n",
         (unsigned int)((sizeof(lvgl_draw_buf_demo) + sizeof(helix_main_data_buf) +
          sizeof(helix_output_buf) + sizeof(file_io_buf)) / 1024));
  printf("\r\n=========================================================\r\n\r\n");
}

/**
 * @brief  快速功能测试：读写示例缓冲，验证外部 SRAM 可用
 */
void extsram_migration_functional_test(void)
{
  printf("\r\n");
  printf("========== EXTSRAM Migration - Functional Test ==========\r\n");

  /* 测试 LVGL 缓冲 */
  printf("\r\n[TEST] LVGL draw buffer write/read test...\r\n");
  lvgl_draw_buf_demo[0] = 0x1234;
  lvgl_draw_buf_demo[100] = 0x5678;
  lvgl_draw_buf_demo[1000] = 0xABCD;

  if (lvgl_draw_buf_demo[0] == 0x1234 &&
      lvgl_draw_buf_demo[100] == 0x5678 &&
      lvgl_draw_buf_demo[1000] == 0xABCD) {
    printf("[PASS] LVGL buffer test OK\r\n");
  } else {
    printf("[FAIL] LVGL buffer test FAILED\r\n");
  }

  /* 测试 Helix 缓冲 */
  printf("\r\n[TEST] Helix buffers write/read test...\r\n");
  helix_main_data_buf[0] = 0x12345678;
  helix_main_data_buf[1000] = 0xDEADBEEF;
  helix_output_buf[0] = 0x7FFF;
  helix_output_buf[500] = 0x8000;

  if (helix_main_data_buf[0] == 0x12345678 &&
      helix_main_data_buf[1000] == 0xDEADBEEF &&
      helix_output_buf[0] == 0x7FFF &&
      helix_output_buf[500] == 0x8000) {
    printf("[PASS] Helix buffers test OK\r\n");
  } else {
    printf("[FAIL] Helix buffers test FAILED\r\n");
  }

  /* 测试文件缓冲 */
  printf("\r\n[TEST] File I/O buffer write/read test...\r\n");
  file_io_buf[0] = 0xAA;
  file_io_buf[32 * 1024] = 0xBB;
  file_io_buf[64 * 1024 - 1] = 0xCC;

  if (file_io_buf[0] == 0xAA &&
      file_io_buf[32 * 1024] == 0xBB &&
      file_io_buf[64 * 1024 - 1] == 0xCC) {
    printf("[PASS] File I/O buffer test OK\r\n");
  } else {
    printf("[FAIL] File I/O buffer test FAILED\r\n");
  }

  printf("\r\n");
  printf("==========================================================\r\n\r\n");
}

/**
 * @brief  集成测试：从 main() 调用
 *         
 *  使用示例：
 *  
 *  在 Core/Src/main.c 中：
 *  
 *    #include "./sdram/bsp_sdram.h"
 *    
 *    void main(void) {
 *      SystemInit();
 *      SystemClock_Config();
 *      DEBUG_USART_Config();
 *      
 *      // 打印内存布局
 *      sram_print_layout();
 *      extsram_migration_print_layout();
 *      
 *      // 运行示例测试
 *      extsram_migration_functional_test();
 *      
 *      // 详细硬件测试（可选）
 *      uint32_t result = sram_test_quick(0x6C000000, 4096);
 *      if (result != 0) {
 *        printf("[MAIN] EXTSRAM hardware test FAILED: %lu\r\n", result);
 *      }
 *      
 *      // 继续正常初始化...
 *      MX_GPIO_Init();
 *      // ...
 *    }
 */
void extsram_migration_all(void)
{
  sram_print_layout();
  extsram_migration_print_layout();
  extsram_migration_functional_test();
}
