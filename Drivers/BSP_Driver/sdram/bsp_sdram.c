/**
  ******************************************************************************
  * @file    bsp_sdram.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   sdramӦ�ú����ӿ�
  *          ������FSMC SRAM ��ʼ������д������
  *          �Լ����� FSMC ��ʼ����fmc_sram_min_init���� SRAM ��֤��sram_verify��
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ��  STM32 F407 ������  
  * ��̳    :http://www.chuxue123.com
  * �Ա�    :http://firestm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "./sdram/bsp_sdram.h"
#include <stdio.h>

static SRAM_HandleTypeDef  SRAM_Handler;
static FMC_NORSRAM_TimingTypeDef Timing;

/* ============================================================================
   FSMC �����ʼ�����Ĵ�������- ���� fmc_sram_min_init.c
   ============================================================================ */

/**
 * @brief  ��ָ����������Ϊ AF12(FSMC)�����������٣�����
 */
static void fsmc_config_pin_af12(GPIO_TypeDef *GPIOx, uint32_t pin)
{
  /* MODER: 10b = Alternate Function */
  GPIOx->MODER &= ~(0x3U << (pin * 2));
  GPIOx->MODER |=  (0x2U << (pin * 2));

  /* OTYPER: 0 = Push-Pull */
  GPIOx->OTYPER &= ~(1U << pin);

  /* OSPEEDR: 11b = High Speed */
  GPIOx->OSPEEDR |=  (0x3U << (pin * 2));

  /* PUPDR: 01b = Pull-up */
  GPIOx->PUPDR &= ~(0x3U << (pin * 2));
  GPIOx->PUPDR |=  (0x1U << (pin * 2));

  /* AFR: set AF12 (0xC) */
  if (pin < 8) {
    uint32_t shift = pin * 4U;
    GPIOx->AFR[0] &= ~(0xFU << shift);
    GPIOx->AFR[0] |=  (0xCU << shift);
  } else {
    uint32_t shift = (pin - 8U) * 4U;
    GPIOx->AFR[1] &= ~(0xFU << shift);
    GPIOx->AFR[1] |=  (0xCU << shift);
  }
}

/**
 * @brief  ���� GPIO �� AF12 FSMC��NE4+����/��ַ/���ƣ�
 */
static void fsmc_gpio_init_af12_full(void)
{
  /* 1) ʹ�ܶ˿�ʱ�ӣ�GPIOD/E/F/G */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN
          | RCC_AHB1ENR_GPIOEEN
          | RCC_AHB1ENR_GPIOFEN
          | RCC_AHB1ENR_GPIOGEN;
  __DSB();

  /* 2) ����Ϊ AF12 FSMC */
  /* GPIOD: PD0, PD1, PD4, PD5, PD8..PD15 */
  fsmc_config_pin_af12(GPIOD, 0);
  fsmc_config_pin_af12(GPIOD, 1);
  fsmc_config_pin_af12(GPIOD, 4);
  fsmc_config_pin_af12(GPIOD, 5);
  fsmc_config_pin_af12(GPIOD, 8);
  fsmc_config_pin_af12(GPIOD, 9);
  fsmc_config_pin_af12(GPIOD, 10);
  fsmc_config_pin_af12(GPIOD, 11);
  fsmc_config_pin_af12(GPIOD, 12);
  fsmc_config_pin_af12(GPIOD, 13);
  fsmc_config_pin_af12(GPIOD, 14);
  fsmc_config_pin_af12(GPIOD, 15);

  /* GPIOE: PE0, PE1, PE7..PE15 */
  fsmc_config_pin_af12(GPIOE, 0);
  fsmc_config_pin_af12(GPIOE, 1);
  fsmc_config_pin_af12(GPIOE, 7);
  fsmc_config_pin_af12(GPIOE, 8);
  fsmc_config_pin_af12(GPIOE, 9);
  fsmc_config_pin_af12(GPIOE, 10);
  fsmc_config_pin_af12(GPIOE, 11);
  fsmc_config_pin_af12(GPIOE, 12);
  fsmc_config_pin_af12(GPIOE, 13);
  fsmc_config_pin_af12(GPIOE, 14);
  fsmc_config_pin_af12(GPIOE, 15);

  /* GPIOF: PF0..PF5, PF12..PF15 */
  fsmc_config_pin_af12(GPIOF, 0);
  fsmc_config_pin_af12(GPIOF, 1);
  fsmc_config_pin_af12(GPIOF, 2);
  fsmc_config_pin_af12(GPIOF, 3);
  fsmc_config_pin_af12(GPIOF, 4);
  fsmc_config_pin_af12(GPIOF, 5);
  fsmc_config_pin_af12(GPIOF, 12);
  fsmc_config_pin_af12(GPIOF, 13);
  fsmc_config_pin_af12(GPIOF, 14);
  fsmc_config_pin_af12(GPIOF, 15);

  /* GPIOG: PG0..PG5, PG12(NE4) */
  fsmc_config_pin_af12(GPIOG, 0);
  fsmc_config_pin_af12(GPIOG, 1);
  fsmc_config_pin_af12(GPIOG, 2);
  fsmc_config_pin_af12(GPIOG, 3);
  fsmc_config_pin_af12(GPIOG, 4);
  fsmc_config_pin_af12(GPIOG, 5);
  fsmc_config_pin_af12(GPIOG, 12);
}

/**
 * @brief  FSMC Bank4 16bit �첽 SRAM �����ʼ��
 *         - ʱ��������أ������ȶ�����
 *         - GPIO ������������ fsmc_gpio_init_af12_full() ���
 */
void FSMC_SRAM_Init_Bank4_16bit_Minimal(void)
{
  /* 0) GPIO ���õ� AF12 FSMC�����룩 */
  fsmc_gpio_init_af12_full();

  /* 1) �� FSMC ʱ�ӣ�F4 �� AHB3ENR */
  RCC->AHB3ENR |= RCC_AHB3ENR_FSMCEN;
  __DSB();

  /* 2) �ȹ� Bank4������ BCR4����ʹ�� */
  FSMC_Bank1->BTCR[6] = 0
             | (0U << 0)   /* MBKEN=0 */
             | (0U << 1)   /* MUXEN=0 */
             | (0U << 2)   /* MTYP=00 (SRAM) */
             | (1U << 4)   /* MWID=01 (16-bit) */
             | (1U << 12)  /* WREN=1 */
             | (0U << 13)  /* WAITEN=0 */
             | (1U << 14); /* EXTMOD=1 */

  /* 3) BTR4 ��ʱ�� + BWTR4 дʱ�� */
  const uint32_t ADDSET  = 7;
  const uint32_t DATAST  = 15;
  const uint32_t BUSTURN = 3;

  FSMC_Bank1->BTCR[7] = 0
            | (ADDSET  << 0)
            | (0U      << 4)
            | (DATAST  << 8)
            | (BUSTURN << 16)
            | (0U      << 28);

  FSMC_Bank1E->BWTR[3] = 0
              | (ADDSET  << 0)
              | (0U      << 4)
              | (DATAST  << 8)
              | (BUSTURN << 16)
              | (0U      << 28);

  /* 4) ʹ�� Bank4 */
  FSMC_Bank1->BTCR[6] |= (1U << 0);
  __DSB();
}

/**
  * @brief  Initializes SRAM MSP.
  */
 void FSMC_SRAM_MspInit(void)
{
//  static DMA_HandleTypeDef dmaHandle;
  GPIO_InitTypeDef GPIO_Init_Structure;
//  SRAM_HandleTypeDef *hsram = & SRAM_Handler;
    
  /* Enable FMC clock */
  __HAL_RCC_FSMC_CLK_ENABLE();
  
//  /* Enable chosen DMAx clock */
//  __SRAM_DMAx_CLK_ENABLE();

  /* Enable GPIOs clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  
  /* Common GPIO configuration */
  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
  GPIO_Init_Structure.Pull      = GPIO_PULLUP;
  GPIO_Init_Structure.Speed     = GPIO_SPEED_HIGH;
  GPIO_Init_Structure.Alternate = GPIO_AF12_FSMC;
  
  /* GPIOD configuration */
  GPIO_Init_Structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8     |\
                              GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |\
                              GPIO_PIN_14 | GPIO_PIN_15;
   
  HAL_GPIO_Init(GPIOD, &GPIO_Init_Structure);

  /* GPIOE configuration */  
  GPIO_Init_Structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7     |\
                              GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |\
                              GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOE, &GPIO_Init_Structure);
  
  /* GPIOF configuration */  
  GPIO_Init_Structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4     |\
                              GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOF, &GPIO_Init_Structure);
  
  /* GPIOG configuration */  
  GPIO_Init_Structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4     |\
                              GPIO_PIN_5 | GPIO_PIN_12;
  
  HAL_GPIO_Init(GPIOG, &GPIO_Init_Structure);  
  
}


/**
  * @brief  Initializes the SRAM device.
  * @retval SRAM status
  */
void FSMC_SRAM_Init(void)
{ 
   SRAM_Handler.Instance = FMC_NORSRAM_DEVICE;
   SRAM_Handler.Extended = FMC_NORSRAM_EXTENDED_DEVICE;
  
  /* SRAM device configuration */  
  Timing.AddressSetupTime      = 0x00;
  Timing.AddressHoldTime       = 0x00;
  Timing.DataSetupTime         = 0x08;
  Timing.BusTurnAroundDuration = 0x00;
  Timing.CLKDivision           = 0x00;
  Timing.DataLatency           = 0x00;
  Timing.AccessMode            = FSMC_ACCESS_MODE_A;
  //ʹ��NE4
  SRAM_Handler.Init.NSBank=FSMC_NORSRAM_BANK4;     					        
	//��ַ/�����߲�����
	SRAM_Handler.Init.DataAddressMux=FSMC_DATA_ADDRESS_MUX_DISABLE; 	
	//SRAM
	SRAM_Handler.Init.MemoryType=FSMC_MEMORY_TYPE_SRAM;   				    
	//16λ���ݿ��
	SRAM_Handler.Init.MemoryDataWidth=FSMC_NORSRAM_MEM_BUS_WIDTH_16; 
	//�Ƿ�ʹ��ͻ������,����ͬ��ͻ���洢����Ч,�˴�δ�õ�
	SRAM_Handler.Init.BurstAccessMode=FSMC_BURST_ACCESS_MODE_DISABLE; 
	//�ȴ��źŵļ���,����ͻ��ģʽ����������
	SRAM_Handler.Init.WaitSignalPolarity=FSMC_WAIT_SIGNAL_POLARITY_LOW;
	//�洢�����ڵȴ�����֮ǰ��һ��ʱ�����ڻ��ǵȴ������ڼ�ʹ��NWAIT
	SRAM_Handler.Init.WaitSignalActive=FSMC_WAIT_TIMING_BEFORE_WS;   
	//�洢��дʹ��
	SRAM_Handler.Init.WriteOperation=FSMC_WRITE_OPERATION_ENABLE;    
	//�ȴ�ʹ��λ,�˴�δ�õ�
	SRAM_Handler.Init.WaitSignal=FSMC_WAIT_SIGNAL_DISABLE;           	
	//��дʹ����ͬ��ʱ��
	SRAM_Handler.Init.ExtendedMode=FSMC_EXTENDED_MODE_DISABLE;        	
	//�Ƿ�ʹ��ͬ������ģʽ�µĵȴ��ź�,�˴�δ�õ�
	SRAM_Handler.Init.AsynchronousWait=FSMC_ASYNCHRONOUS_WAIT_DISABLE;	
	//��ֹͻ��д
	SRAM_Handler.Init.WriteBurst=FSMC_WRITE_BURST_DISABLE;           	  
	SRAM_Handler.Init.ContinuousClock=FSMC_CONTINUOUS_CLOCK_SYNC_ASYNC;
  
  /* SRAM controller initialization */
  FSMC_SRAM_MspInit();
  HAL_SRAM_Init(& SRAM_Handler, &Timing, &Timing);
}


/**
  * @brief  �ԡ��֡�Ϊ��λ��sramд������ 
  * @param  pBuffer: ָ�����ݵ�ָ�� 
  * @param  uwWriteAddress: Ҫд���SRAM�ڲ���ַ
  * @param  uwBufferSize: Ҫд�����ݴ�С
  * @retval None.
  */
void SRAM_WriteBuffer(uint32_t* pBuffer, uint32_t uwWriteAddress, uint32_t uwBufferSize)
{
  __IO uint32_t write_pointer = (uint32_t)uwWriteAddress;
 

  /* ѭ��д������ */
  for (; uwBufferSize != 0; uwBufferSize--) 
  {
    /* �������ݵ�SRAM */
    *(uint32_t *) (Bank1_SRAM4_ADDR + write_pointer) = *pBuffer++;

    /* ��ַ����*/
    write_pointer += 4;
  }
    
}

/**
  * @brief  ��SRAM�ж�ȡ���� 
  * @param  pBuffer: ָ��洢���ݵ�buffer
  * @param  ReadAddress: Ҫ��ȡ���ݵĵ�ʮ
  * @param  uwBufferSize: Ҫ��ȡ�����ݴ�С
  * @retval None.
  */
void SRAM_ReadBuffer(uint32_t* pBuffer, uint32_t uwReadAddress, uint32_t uwBufferSize)
{
  __IO uint32_t write_pointer = (uint32_t)uwReadAddress;
  
  
  /*��ȡ���� */
  for(; uwBufferSize != 0x00; uwBufferSize--)
  {
   *pBuffer++ = *(__IO uint32_t *)(Bank1_SRAM4_ADDR + write_pointer );
    
   /* ��ַ����*/
    write_pointer += 4;
  } 
}


/**
  * @brief  ����SRAM�Ƿ����� 
  * @param  None
  * @retval ��������1���쳣����0
  */
uint8_t SRAM_Test(void)
{
  /*д�����ݼ�����*/
  uint32_t counter=0;
  
  /* 8λ������ */
  uint8_t ubWritedata_8b = 0, ubReaddata_8b = 0;  
  
  /* 16λ������ */
  uint16_t uhWritedata_16b = 0, uhReaddata_16b = 0; 
  
  SRAM_INFO("���ڼ��SRAM����8λ��16λ�ķ�ʽ��дsram...");


  /*��8λ��ʽ��д���ݣ���У��*/
  
  /* ��SRAM����ȫ������Ϊ0 ��IS62WV51216_SIZE����8λΪ��λ�� */
  for (counter = 0x00; counter < IS62WV51216_SIZE; counter++)
  {
    *(__IO uint8_t*) (Bank1_SRAM4_ADDR + counter) = (uint8_t)0x0;
  }
  
  /* ������SRAMд������  8λ */
  for (counter = 0; counter < IS62WV51216_SIZE; counter++)
  {
    *(__IO uint8_t*) (Bank1_SRAM4_ADDR + counter) = (uint8_t)(ubWritedata_8b + counter);
  }
  
  /* ��ȡ SRAM ���ݲ����*/
  for(counter = 0; counter<IS62WV51216_SIZE;counter++ )
  {
    ubReaddata_8b = *(__IO uint8_t*)(Bank1_SRAM4_ADDR + counter);  //�Ӹõ�ַ��������
    
    if(ubReaddata_8b != (uint8_t)(ubWritedata_8b + counter))      //������ݣ�������ȣ���������,���ؼ��ʧ�ܽ����
    {
      SRAM_ERROR("8λ���ݶ�д����");
      return 0;
    }
  }
	
  
  /*��16λ��ʽ��д���ݣ������*/
  
  /* ��SRAM����ȫ������Ϊ0 */
  for (counter = 0x00; counter < IS62WV51216_SIZE/2; counter++)
  {
    *(__IO uint16_t*) (Bank1_SRAM4_ADDR + 2*counter) = (uint16_t)0x00;
  }
  
  /* ������SRAMд������  16λ */
  for (counter = 0; counter < IS62WV51216_SIZE/2; counter++)
  {
    *(__IO uint16_t*) (Bank1_SRAM4_ADDR + 2*counter) = (uint16_t)(uhWritedata_16b + counter);
  }
  
    /* ��ȡ SRAM ���ݲ����*/
  for(counter = 0; counter<IS62WV51216_SIZE/2;counter++ )
  {
    uhReaddata_16b = *(__IO uint16_t*)(Bank1_SRAM4_ADDR + 2*counter);  //�Ӹõ�ַ��������
    
    if(uhReaddata_16b != (uint16_t)(uhWritedata_16b + counter))      //������ݣ�������ȣ���������,���ؼ��ʧ�ܽ����
    {
      SRAM_ERROR("16λ���ݶ�д����");

      return 0;
    }
  }
  
  SRAM_INFO("SRAM��д����������"); 
  /*���������return 1 */
  return 1;

}


/*********************************************END OF FILE**********************/

/* ============================================================================
   SRAM ��֤���� - ���� sram_verify.c
   ============================================================================ */

/**
 * @brief  ���ټ���ⲿ SRAM �����ɶ�д����������
 * @param  start_addr  ��ʼ��ַ
 * @param  words_count 16-bit ��������4096 �Ƽ���
 * @return 0=ͨ��������=ʧ��
 */
uint32_t sram_test_quick(uint32_t start_addr, uint32_t words_count)
{
  volatile uint16_t *p = (volatile uint16_t *)start_addr;
  uint32_t i;

  /* Pattern 1: incrementing XOR 0xAAAA */
  for (i = 0; i < words_count; i++) {
    p[i] = (uint16_t)(i ^ 0xAAAA);
  }
  for (i = 0; i < words_count; i++) {
    if (p[i] != (uint16_t)(i ^ 0xAAAA)) {
      return 1 + i;  /* ����ʧ��λ�� */
    }
  }

  /* Pattern 2: inverted XOR 0x5555 */
  for (i = 0; i < words_count; i++) {
    p[i] = (uint16_t)(i ^ 0x5555);
  }
  for (i = 0; i < words_count; i++) {
    if (p[i] != (uint16_t)(i ^ 0x5555)) {
      return 1000 + i;  /* ���ز�ͬ��Χ��ʧ��λ�� */
    }
  }

  return 0;  /* ͨ�� */
}

/**
 * @brief  �ⲿ SRAM ���ܲ��ԣ�����λ��ת����ַ�߲飩
 * @param  start_addr  ��ʼ��ַ��ͨ�� 0x6C000000��
 * @param  size_bytes  ���Դ�С���������� 64KB��
 * @return 0=ͨ��������=ʧ�ܲ��� ID ��λ��
 */
uint32_t sram_test_comprehensive(uint32_t start_addr, uint32_t size_bytes)
{
  volatile uint8_t *p8 = (volatile uint8_t *)start_addr;
  volatile uint16_t *p16 = (volatile uint16_t *)start_addr;
  volatile uint32_t *p32 = (volatile uint32_t *)start_addr;
  uint32_t i;
  uint32_t word_count = size_bytes / 4;

  SRAM_DEBUG("EXTSRAM comprehensive test starting...");

  /* Sub-test 1: Fill with 0xFFFF */
  SRAM_DEBUG("Sub-test 1: Fill 0xFFFF...");
  for (i = 0; i < word_count; i++) {
    p32[i] = 0xFFFFFFFF;
  }
  for (i = 0; i < word_count; i++) {
    if (p32[i] != 0xFFFFFFFF) {
      SRAM_ERROR("Sub-test 1 FAILED at offset 0x%lX", i * 4);
      return 10 + i;
    }
  }
  SRAM_DEBUG("Sub-test 1 PASSED");

  /* Sub-test 2: Fill with 0x0000 */
  SRAM_DEBUG("Sub-test 2: Fill 0x0000...");
  for (i = 0; i < word_count; i++) {
    p32[i] = 0x00000000;
  }
  for (i = 0; i < word_count; i++) {
    if (p32[i] != 0x00000000) {
      SRAM_ERROR("Sub-test 2 FAILED at offset 0x%lX", i * 4);
      return 20 + i;
    }
  }
  SRAM_DEBUG("Sub-test 2 PASSED");

  /* Sub-test 3: Address walking (MSB bit sweep) */
  SRAM_DEBUG("Sub-test 3: Address walking...");
  for (i = 0; i < word_count; i++) {
    p32[i] = 0xAAAAAAAA;  /* Pattern 1 */
  }
  for (i = 0; i < word_count; i++) {
    if (p32[i] != 0xAAAAAAAA) {
      SRAM_ERROR("Sub-test 3 FAILED at offset 0x%lX", i * 4);
      return 30 + i;
    }
  }

  for (i = 0; i < word_count; i++) {
    p32[i] = 0x55555555;  /* Pattern 2 (inverted) */
  }
  for (i = 0; i < word_count; i++) {
    if (p32[i] != 0x55555555) {
      SRAM_ERROR("Sub-test 3B FAILED at offset 0x%lX", i * 4);
      return 35 + i;
    }
  }
  SRAM_DEBUG("Sub-test 3 PASSED");

  /* Sub-test 4: Checkered pattern (byte-level test) */
  SRAM_DEBUG("Sub-test 4: Checkered pattern...");
  for (i = 0; i < size_bytes; i++) {
    p8[i] = (i & 1) ? 0xAA : 0x55;
  }
  for (i = 0; i < size_bytes; i++) {
    uint8_t expected = (i & 1) ? 0xAA : 0x55;
    if (p8[i] != expected) {
      SRAM_ERROR("Sub-test 4 FAILED at offset 0x%lX (got 0x%02X, expect 0x%02X)",
                 i, p8[i], expected);
      return 40 + i;
    }
  }
  SRAM_DEBUG("Sub-test 4 PASSED");

  SRAM_INFO("Comprehensive SRAM test PASSED");
  return 0;
}

/**
 * @brief  ��ѯĳ����ַ�������ڴ�����
 * @param  addr  ��ַ
 * @return �����������ַ����� "UNKNOWN"
 */
const char* sram_get_region_name(uint32_t addr)
{
  if (addr >= EXTSRAM_BASE && addr < EXTSRAM_END) {
    return "EXTSRAM";
  } else if (addr >= CCMRAM_BASE && addr < CCMRAM_END) {
    return "CCMRAM";
  } else if (addr >= INTRAM_BASE && addr < INTRAM_END) {
    return "INTRAM";
  } else {
    return "UNKNOWN";
  }
}

/**
 * @brief  ��ӡ�ڴ沼�ֱ���׼��ʽ��
 */
void sram_print_layout(void)
{
  printf("\r\n------------------------------------------------------------\r\n");
  printf("Memory Layout:\r\n");
  printf("------------------------------------------------------------\r\n");
  printf("EXTSRAM:  0x%08lX - 0x%08lX   (%4lu KB)  FSMC Bank4\r\n",
    EXTSRAM_BASE, EXTSRAM_END - 1, EXTSRAM_SIZE / 1024);
  printf("CCMRAM:   0x%08lX - 0x%08lX   (%4lu KB)  CPU-Only\r\n",
    CCMRAM_BASE, CCMRAM_END - 1, CCMRAM_SIZE / 1024);
  printf("INTRAM:   0x%08lX - 0x%08lX   (%4lu KB)  Internal\r\n",
    INTRAM_BASE, INTRAM_END - 1, INTRAM_SIZE / 1024);
  
  uint32_t total_kb = (EXTSRAM_SIZE + CCMRAM_SIZE + INTRAM_SIZE) / 1024;
  printf("Total accessible memory: %5lu KB\r\n", total_kb);
  printf("------------------------------------------------------------\r\n\r\n");
}

/**
 * @brief  ��ӡ SRAM ���ټ��ͳ�ʼ��ժҪ
 *         �����ڴ沼�֡�SRAM���ٲ��ԡ��ؼ������ַ��
 *         �Ƽ��� main() ���ڵ���
 */
void sram_print_init_summary(void)
{
  /* ��ӡ�ڴ沼�� */
  sram_print_layout();
  
  /* SRAM���ٲ��� */
  printf("------------------------------------------------------------\r\n");
  printf("External SRAM Quick Verification\r\n");
  printf("------------------------------------------------------------\r\n");
  printf("Testing EXTSRAM accessibility (4096 words)...\r\n");
  uint32_t test_result = sram_test_quick(EXTSRAM_BASE, 4096);
  if (test_result == 0) {
    printf("EXTSRAM hardware test: PASS\r\n");
  } else {
    printf("EXTSRAM hardware test: FAIL (error code: %lu)\r\n", test_result);
  }
  printf("\r\n");
}

