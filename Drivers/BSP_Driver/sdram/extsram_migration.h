/**
 ******************************************************************************
 * @file    extsram_migration.h
 * @brief   外部 SRAM 迁移示例头文件
 *          声明迁移示例相关函数
 ******************************************************************************
 */

#ifndef EXTSRAM_MIGRATION_H
#define EXTSRAM_MIGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  打印所有示例缓冲的内存地址与所在区域
 */
void extsram_migration_print_layout(void);

/**
 * @brief  快速功能测试：读写示例缓冲，验证外部 SRAM 可用
 */
void extsram_migration_functional_test(void);

/**
 * @brief  集成测试：从 main() 调用
 *         
 *  使用示例：
 *  
 *  在 Core/Src/main.c 中：
 *  
 *    #include "./sdram/extsram_migration.h"
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
void extsram_migration_all(void);

#ifdef __cplusplus
}
#endif

#endif /* EXTSRAM_MIGRATION_H */
