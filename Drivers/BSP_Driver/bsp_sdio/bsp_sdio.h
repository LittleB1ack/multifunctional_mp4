#ifndef BSP_SDIO_H
#define BSP_SDIO_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "../../../Core/Inc/sdio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BSP 层对外暴露的阻塞 DMA 接口（包装 core 层实现） */
HAL_StatusTypeDef BSP_SD_ReadBlocks_DMA_Blocking(uint8_t *pData, uint32_t startBlock, uint32_t numBlocks, uint32_t timeout);
HAL_StatusTypeDef BSP_SD_WriteBlocks_DMA_Blocking(uint8_t *pData, uint32_t startBlock, uint32_t numBlocks, uint32_t timeout);
HAL_StatusTypeDef BSP_SD_EraseBlocks(uint32_t startBlock, uint32_t endBlock);
void BSP_SD_PrintCardInfo(void);
HAL_StatusTypeDef BSP_SD_Test(uint8_t do_write);
HAL_StatusTypeDef BSP_SD_Init(void);

#ifdef __cplusplus
}
#endif

#endif
