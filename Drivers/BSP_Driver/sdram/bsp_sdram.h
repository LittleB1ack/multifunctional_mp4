#ifndef __SDRAM_H
#define	__SDRAM_H

#include "stm32f4xx.h"
#include "./usart/bsp_debug_usart.h"

/* ============================================================================
   极简 FSMC 外部 SRAM 初始化（Bank4/NE4，16bit，异步）
   ============================================================================ */
void FSMC_SRAM_Init_Bank4_16bit_Minimal(void);

//使用NOR/SRAM的 Bank1.sector4,地址位HADDR[27,26]=10 
//对IS61LV25616/IS62WV25616,地址线范围为A0~A17 
//对IS61LV51216/IS62WV51216,地址线范围为A0~A18
#define Bank1_SRAM4_ADDR    ((uint32_t)(0x6C000000))		

#define IS62WV51216_SIZE 0x100000  //512*16/2bits = 0x100000  ，1M字节

/* ============================================================================
   内存区间定义（从 sct 映射，用于运行时检查）- 来自 sram_verify.h
   ============================================================================ */

#define EXTSRAM_BASE   0x6C000000UL
#define EXTSRAM_SIZE   0x00100000UL   /* 1MB */
#define EXTSRAM_END    (EXTSRAM_BASE + EXTSRAM_SIZE)

#define CCMRAM_BASE    0x10000000UL
#define CCMRAM_SIZE    0x00010000UL   /* 64KB */
#define CCMRAM_END     (CCMRAM_BASE + CCMRAM_SIZE)

#define INTRAM_BASE    0x20000000UL
#define INTRAM_SIZE    0x00020000UL   /* 128KB */
#define INTRAM_END     (INTRAM_BASE + INTRAM_SIZE)

#define IS_IN_EXTSRAM(addr)  ((addr) >= EXTSRAM_BASE && (addr) < EXTSRAM_END)
#define IS_IN_CCMRAM(addr)   ((addr) >= CCMRAM_BASE  && (addr) < CCMRAM_END)
#define IS_IN_INTRAM(addr)   ((addr) >= INTRAM_BASE  && (addr) < INTRAM_END)

/* ============================================================================
   调试打印宏
   ============================================================================ */

#define SRAM_PRINT_ADDR(var_name, var_addr) \
  do { \
    printf("[SRAM] %-20s @ 0x%08lX (%s)\r\n", \
           (var_name), (uint32_t)(var_addr), sram_get_region_name((uint32_t)(var_addr))); \
  } while(0)

#define SRAM_PRINT_REGION(ptr, size) \
  do { \
    printf("[SRAM] Region [0x%08lX .. 0x%08lX] size=%lu bytes (%s)\r\n", \
           (uint32_t)(ptr), (uint32_t)(ptr) + (size) - 1, (size), \
           sram_get_region_name((uint32_t)(ptr))); \
  } while(0)


//#define FSMC_GPIO_AF             GPIO_AF_FSMC

///*A地址信号线*/    
//#define FSMC_A0_GPIO_PORT        GPIOF
//#define FSMC_A0_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A0_GPIO_PIN         GPIO_Pin_0
//#define FSMC_A0_GPIO_PinSource   GPIO_PinSource0

//#define FSMC_A1_GPIO_PORT        GPIOF
//#define FSMC_A1_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A1_GPIO_PIN         GPIO_Pin_1
//#define FSMC_A1_GPIO_PinSource   GPIO_PinSource1

//#define FSMC_A2_GPIO_PORT        GPIOF
//#define FSMC_A2_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A2_GPIO_PIN         GPIO_Pin_2
//#define FSMC_A2_GPIO_PinSource   GPIO_PinSource2

//#define FSMC_A3_GPIO_PORT        GPIOF
//#define FSMC_A3_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A3_GPIO_PIN         GPIO_Pin_3
//#define FSMC_A3_GPIO_PinSource   GPIO_PinSource3

//#define FSMC_A4_GPIO_PORT        GPIOF
//#define FSMC_A4_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A4_GPIO_PIN         GPIO_Pin_4
//#define FSMC_A4_GPIO_PinSource   GPIO_PinSource4

//#define FSMC_A5_GPIO_PORT        GPIOF
//#define FSMC_A5_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A5_GPIO_PIN         GPIO_Pin_5
//#define FSMC_A5_GPIO_PinSource   GPIO_PinSource5

//#define FSMC_A6_GPIO_PORT        GPIOF
//#define FSMC_A6_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A6_GPIO_PIN         GPIO_Pin_12
//#define FSMC_A6_GPIO_PinSource   GPIO_PinSource12

//#define FSMC_A7_GPIO_PORT        GPIOF
//#define FSMC_A7_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A7_GPIO_PIN         GPIO_Pin_13
//#define FSMC_A7_GPIO_PinSource   GPIO_PinSource13

//#define FSMC_A8_GPIO_PORT        GPIOF
//#define FSMC_A8_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A8_GPIO_PIN         GPIO_Pin_14
//#define FSMC_A8_GPIO_PinSource   GPIO_PinSource14

//#define FSMC_A9_GPIO_PORT        GPIOF
//#define FSMC_A9_GPIO_CLK         RCC_AHB1Periph_GPIOF
//#define FSMC_A9_GPIO_PIN         GPIO_Pin_15
//#define FSMC_A9_GPIO_PinSource   GPIO_PinSource15

//#define FSMC_A10_GPIO_PORT        GPIOG
//#define FSMC_A10_GPIO_CLK         RCC_AHB1Periph_GPIOG
//#define FSMC_A10_GPIO_PIN         GPIO_Pin_0
//#define FSMC_A10_GPIO_PinSource   GPIO_PinSource0

//#define FSMC_A11_GPIO_PORT        GPIOG
//#define FSMC_A11_GPIO_CLK         RCC_AHB1Periph_GPIOG
//#define FSMC_A11_GPIO_PIN         GPIO_Pin_1
//#define FSMC_A11_GPIO_PinSource   GPIO_PinSource1

//#define FSMC_A12_GPIO_PORT        GPIOG
//#define FSMC_A12_GPIO_CLK         RCC_AHB1Periph_GPIOG
//#define FSMC_A12_GPIO_PIN         GPIO_Pin_2
//#define FSMC_A12_GPIO_PinSource   GPIO_PinSource2

//#define FSMC_A13_GPIO_PORT        GPIOG
//#define FSMC_A13_GPIO_CLK         RCC_AHB1Periph_GPIOG
//#define FSMC_A13_GPIO_PIN         GPIO_Pin_3
//#define FSMC_A13_GPIO_PinSource   GPIO_PinSource3

//#define FSMC_A14_GPIO_PORT        GPIOG
//#define FSMC_A14_GPIO_CLK         RCC_AHB1Periph_GPIOG
//#define FSMC_A14_GPIO_PIN         GPIO_Pin_4
//#define FSMC_A14_GPIO_PinSource   GPIO_PinSource4

//#define FSMC_A15_GPIO_PORT        GPIOG
//#define FSMC_A15_GPIO_CLK         RCC_AHB1Periph_GPIOG
//#define FSMC_A15_GPIO_PIN         GPIO_Pin_5
//#define FSMC_A15_GPIO_PinSource   GPIO_PinSource5

//#define FSMC_A16_GPIO_PORT        GPIOD
//#define FSMC_A16_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_A16_GPIO_PIN         GPIO_Pin_11
//#define FSMC_A16_GPIO_PinSource   GPIO_PinSource11

//#define FSMC_A17_GPIO_PORT        GPIOD
//#define FSMC_A17_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_A17_GPIO_PIN         GPIO_Pin_12
//#define FSMC_A17_GPIO_PinSource   GPIO_PinSource12

//#define FSMC_A18_GPIO_PORT        GPIOD
//#define FSMC_A18_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_A18_GPIO_PIN         GPIO_Pin_13
//#define FSMC_A18_GPIO_PinSource   GPIO_PinSource13

///*D 数据信号线*/
//#define FSMC_D0_GPIO_PORT        GPIOD
//#define FSMC_D0_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_D0_GPIO_PIN         GPIO_Pin_14
//#define FSMC_D0_GPIO_PinSource   GPIO_PinSource14

//#define FSMC_D1_GPIO_PORT        GPIOD
//#define FSMC_D1_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_D1_GPIO_PIN         GPIO_Pin_15
//#define FSMC_D1_GPIO_PinSource   GPIO_PinSource15

//#define FSMC_D2_GPIO_PORT        GPIOD
//#define FSMC_D2_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_D2_GPIO_PIN         GPIO_Pin_0
//#define FSMC_D2_GPIO_PinSource   GPIO_PinSource0

//#define FSMC_D3_GPIO_PORT        GPIOD
//#define FSMC_D3_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_D3_GPIO_PIN         GPIO_Pin_1
//#define FSMC_D3_GPIO_PinSource   GPIO_PinSource1

//#define FSMC_D4_GPIO_PORT        GPIOE
//#define FSMC_D4_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_D4_GPIO_PIN         GPIO_Pin_7
//#define FSMC_D4_GPIO_PinSource   GPIO_PinSource7

//#define FSMC_D5_GPIO_PORT        GPIOE
//#define FSMC_D5_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_D5_GPIO_PIN         GPIO_Pin_8
//#define FSMC_D5_GPIO_PinSource   GPIO_PinSource8

//#define FSMC_D6_GPIO_PORT        GPIOE
//#define FSMC_D6_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_D6_GPIO_PIN         GPIO_Pin_9
//#define FSMC_D6_GPIO_PinSource   GPIO_PinSource9

//#define FSMC_D7_GPIO_PORT        GPIOE
//#define FSMC_D7_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_D7_GPIO_PIN         GPIO_Pin_10
//#define FSMC_D7_GPIO_PinSource   GPIO_PinSource10

//#define FSMC_D8_GPIO_PORT        GPIOE
//#define FSMC_D8_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_D8_GPIO_PIN         GPIO_Pin_11
//#define FSMC_D8_GPIO_PinSource   GPIO_PinSource11

//#define FSMC_D9_GPIO_PORT        GPIOE
//#define FSMC_D9_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_D9_GPIO_PIN         GPIO_Pin_12
//#define FSMC_D9_GPIO_PinSource   GPIO_PinSource12

//#define FSMC_D10_GPIO_PORT        GPIOE
//#define FSMC_D10_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_D10_GPIO_PIN         GPIO_Pin_13
//#define FSMC_D10_GPIO_PinSource   GPIO_PinSource13

//#define FSMC_D11_GPIO_PORT        GPIOE
//#define FSMC_D11_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_D11_GPIO_PIN         GPIO_Pin_14
//#define FSMC_D11_GPIO_PinSource   GPIO_PinSource14

//#define FSMC_D12_GPIO_PORT        GPIOE
//#define FSMC_D12_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_D12_GPIO_PIN         GPIO_Pin_15
//#define FSMC_D12_GPIO_PinSource   GPIO_PinSource15

//#define FSMC_D13_GPIO_PORT        GPIOD
//#define FSMC_D13_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_D13_GPIO_PIN         GPIO_Pin_8
//#define FSMC_D13_GPIO_PinSource   GPIO_PinSource8

//#define FSMC_D14_GPIO_PORT        GPIOD
//#define FSMC_D14_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_D14_GPIO_PIN         GPIO_Pin_9
//#define FSMC_D14_GPIO_PinSource   GPIO_PinSource9

//#define FSMC_D15_GPIO_PORT        GPIOD
//#define FSMC_D15_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_D15_GPIO_PIN         GPIO_Pin_10
//#define FSMC_D15_GPIO_PinSource   GPIO_PinSource10

///*控制信号线*/  
///*CS片选*/
///*NE4 ,对应的基地址0x6C000000*/
//#define FSMC_CS_GPIO_PORT        GPIOG
//#define FSMC_CS_GPIO_CLK         RCC_AHB1Periph_GPIOG
//#define FSMC_CS_GPIO_PIN         GPIO_Pin_12
//#define FSMC_CS_GPIO_PinSource   GPIO_PinSource12

///*WE写使能*/
//#define FSMC_WE_GPIO_PORT        GPIOD
//#define FSMC_WE_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_WE_GPIO_PIN         GPIO_Pin_5
//#define FSMC_WE_GPIO_PinSource   GPIO_PinSource5

///*OE读使能*/
//#define FSMC_OE_GPIO_PORT        GPIOD
//#define FSMC_OE_GPIO_CLK         RCC_AHB1Periph_GPIOD
//#define FSMC_OE_GPIO_PIN         GPIO_Pin_4
//#define FSMC_OE_GPIO_PinSource   GPIO_PinSource4


///*UB数据掩码*/
//#define FSMC_UDQM_GPIO_PORT        GPIOE
//#define FSMC_UDQM_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_UDQM_GPIO_PIN         GPIO_Pin_1
//#define FSMC_UDQM_GPIO_PinSource   GPIO_PinSource1

///*LB数据掩码*/
//#define FSMC_LDQM_GPIO_PORT        GPIOE
//#define FSMC_LDQM_GPIO_CLK         RCC_AHB1Periph_GPIOE
//#define FSMC_LDQM_GPIO_PIN         GPIO_Pin_0
//#define FSMC_LDQM_GPIO_PinSource   GPIO_PinSource0


/*信息输出*/
#define SRAM_DEBUG_ON         1

#define SRAM_INFO(fmt,arg...)           printf("<<-SRAM-INFO->> "fmt"\n",##arg)
#define SRAM_ERROR(fmt,arg...)          printf("<<-SRAM-ERROR->> "fmt"\n",##arg)
#define SRAM_DEBUG(fmt,arg...)          do{\
                                          if(SRAM_DEBUG_ON)\
                                          printf("<<-SRAM-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                          }while(0)




											  
void FSMC_SRAM_Init(void);
void FSMC_SRAM_MspInit(void);                                          
void FSMC_SRAM_WriteBuffer(uint8_t* pBuffer,uint32_t WriteAddr,uint32_t NumHalfwordToWrite);
void FSMC_SRAM_ReadBuffer(uint8_t* pBuffer,uint32_t ReadAddr,uint32_t NumHalfwordToRead);

uint8_t SRAM_Test(void);

/* ============================================================================
   SRAM 验证函数声明 - 来自 sram_verify.h
   ============================================================================ */

/**
 * @brief  快速检查外部 SRAM 基本可读写
 * @param  start_addr  起始地址
 * @param  words_count 16-bit 字数
 * @return 0=通过，非零=失败
 */
uint32_t sram_test_quick(uint32_t start_addr, uint32_t words_count);

/**
 * @brief  外部 SRAM 功能测试（行列位反转、地址走查）
 * @param  start_addr  起始地址（通常 0x6C000000）
 * @param  size_bytes  测试大小
 * @return 0=通过，非零=失败
 */
uint32_t sram_test_comprehensive(uint32_t start_addr, uint32_t size_bytes);

/**
 * @brief  查询某个地址所属的内存区域
 * @param  addr  地址
 * @return 返回区域名字符串或 "UNKNOWN"
 */
const char* sram_get_region_name(uint32_t addr);

/**
 * @brief  打印内存布局表（标准格式）
 */
void sram_print_layout(void);

/**
 * @brief  打印 SRAM 快速检测和初始化摘要
 *         包括内存布局、SRAM快速测试
 *         推荐在 main() 早期调用
 */
void sram_print_init_summary(void);


#endif /* __SDRAM_H */
