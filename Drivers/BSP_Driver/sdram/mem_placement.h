/**
 ******************************************************************************
 * @file    mem_placement.h
 * @brief   统一的内存分区放置宏，便于将变量/缓冲精准放入 CCM/外部SRAM/内部SRAM
 * @note    配合工程的 scatter 文件（.sct）使用
 ******************************************************************************
 * 使用方法：
 *   1. 在需要定向放置的变量声明前加上对应的宏：
 *      - EXTSRAM：放入外部 SRAM（0x6C000000，FSMC Bank4/NE4）
 *      - CCMRAM：放入 CCM（0x10000000，64KB，仅 CPU 访问，DMA 不可用）
 *      - INTRAM：显式放入内部 SRAM（通常不需要，兜底即可）
 *      - NOINIT_EXTSRAM：外部 SRAM 中的 NOINIT 区（上电不清零）
 *   
 *   2. 全局开关 - ENABLE_EXTSRAM_MIGRATION：
 *      设置为 1：启用外部 SRAM 迁移（所有 EXTSRAM 标记的对象放入外部 SRAM）
 *      设置为 0：禁用迁移（所有对象回退到内部 SRAM，用于问题诊断）
 *   
 *   3. 示例：
 *      static EXTSRAM uint8_t lvgl_draw_buf[800*480*2];  // LVGL 绘制缓冲
 *      static CCMRAM uint32_t heap_array[16384];         // FreeRTOS heap
 *      static NOINIT_EXTSRAM uint8_t big_work_buf[1024*1024]; // 1MB 工作区
 *
 *   4. 限制：
 *      - CCM 仅供 CPU 访问，DMA/SDIO/I2S/FSMC 等外设无法访问，不要放 DMA 缓冲
 *      - 外部 SRAM 带宽有限，高速 DMA 访问（如 I2S/Camera）优先放内部 SRAM
 *      - 构建后务必查看 map 文件，确认符号地址在预期范围：
 *          EXTSRAM: 0x6Cxxxxxx
 *          CCMRAM:  0x1000xxxx
 *          INTRAM:  0x2000xxxx
 ******************************************************************************
 */

#ifndef MEM_PLACEMENT_H
#define MEM_PLACEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
   外部 SRAM 迁移全局开关（核心配置）
   ============================================================================
   使用方法：
   1. 编译时启用：在编译器预定义中添加 ENABLE_EXTSRAM_MIGRATION=1
   2. 或在此文件中直接修改默认值（下面一行）
   
   设置为 1：启用外部 SRAM 迁移（默认，性能模式）
   设置为 0：禁用迁移，所有缓冲回退到内部 SRAM（调试模式）
*/
#ifndef ENABLE_EXTSRAM_MIGRATION
  #define ENABLE_EXTSRAM_MIGRATION  1
#endif

/* ============================================================================
   编译器兼容性适配
   ============================================================================ */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
  /* ARM Compiler 5/6 (Keil MDK) */
  #if ENABLE_EXTSRAM_MIGRATION
    #define EXTSRAM           __attribute__((section("extsram"), zero_init))
    #define NOINIT_EXTSRAM    __attribute__((section("extsram_noinit"), zero_init))
  #else
    /* 禁用迁移：空宏，对象回退到内部 SRAM */
    #define EXTSRAM
    #define NOINIT_EXTSRAM
  #endif
  
  #define CCMRAM            __attribute__((section("ccmram"), zero_init))
  #define INTRAM            __attribute__((section(".bss")))
  
#elif defined(__GNUC__)
  /* GCC / ARM GCC */
  #if ENABLE_EXTSRAM_MIGRATION
    #define EXTSRAM           __attribute__((section(".extsram")))
    #define NOINIT_EXTSRAM    __attribute__((section(".extsram_noinit")))
  #else
    /* 禁用迁移：空宏，对象回退到内部 SRAM */
    #define EXTSRAM
    #define NOINIT_EXTSRAM
  #endif
  
  #define CCMRAM            __attribute__((section(".ccmram")))
  #define INTRAM            __attribute__((section(".bss")))
  
#elif defined(__ICCARM__)
  /* IAR Compiler */
  #if ENABLE_EXTSRAM_MIGRATION
    #define EXTSRAM           _Pragma("location=\"extsram\"")
    #define NOINIT_EXTSRAM    _Pragma("location=\"extsram_noinit\"")
  #else
    /* 禁用迁移：空宏，对象回退到内部 SRAM */
    #define EXTSRAM
    #define NOINIT_EXTSRAM
  #endif
  
  #define CCMRAM            _Pragma("location=\"ccmram\"")
  #define INTRAM            /* 默认 */
  
#else
  /* 未知编译器，定义为空宏（降级模式） */
  #define EXTSRAM
  #define CCMRAM
  #define INTRAM
  #define NOINIT_EXTSRAM
#endif


/* ============================================================================
   便捷对齐宏（可选）
   ============================================================================ */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) || defined(__GNUC__)
  #define ALIGN_4     __attribute__((aligned(4)))
  #define ALIGN_32    __attribute__((aligned(32)))
  #define ALIGN_64    __attribute__((aligned(64)))
#elif defined(__ICCARM__)
  #define ALIGN_4     _Pragma("data_alignment=4")
  #define ALIGN_32    _Pragma("data_alignment=32")
  #define ALIGN_64    _Pragma("data_alignment=64")
#else
  #define ALIGN_4
  #define ALIGN_32
  #define ALIGN_64
#endif


/* ============================================================================
   典型应用场景说明
   ============================================================================ */
/*
  1. LVGL 绘制缓冲（大容量，非 DMA）
     static EXTSRAM ALIGN_32 lv_color_t draw_buf_1[800*480];
     
  2. MP3 解码工作区（Helix，非 DMA）
     static EXTSRAM uint8_t mp3_main_data_buf[...];
     
  3. FreeRTOS heap（已在 sct 中按 heap_4.o 自动放入 CCMRAM）
     若需显式声明其他独立 heap：
     static CCMRAM uint8_t my_heap[32768];
     
  4. I2S/音频 DMA 缓冲（必须内部 SRAM）
     static INTRAM ALIGN_32 int16_t audio_dma_buf[2][1152];
     
  5. 大型文件读写缓冲（可外部，减轻内部压力）
     static EXTSRAM uint8_t file_io_buf[64*1024];
     
  6. 持久化工作区（上电不清零，用于断电恢复）
     static NOINIT_EXTSRAM uint32_t persistent_state[256];
*/

#ifdef __cplusplus
}
#endif

#endif /* MEM_PLACEMENT_H */
