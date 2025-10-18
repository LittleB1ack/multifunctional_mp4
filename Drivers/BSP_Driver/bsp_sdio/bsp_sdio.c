#include "bsp_sdio.h"
#include "../../..//Core/Inc/dma.h"
#include "../../..//Core/Inc/sdio.h" /* 包含 core 层的声明，用于底层类型 */
#include <string.h>
#include <stdio.h>
HAL_StatusTypeDef BSP_SD_Init(void)
{
    printf("[SD] BSP_SD_Init: MX_DMA_Init()\r\n");
    /* 1) DMA 时钟与 NVIC */
    MX_DMA_Init();
    printf("[SD] BSP_SD_Init: MX_DMA_Init done\r\n");

    /* 2) SDIO 外设句柄配置（时钟、总线宽度参数） */
    printf("[SD] BSP_SD_Init: MX_SDIO_SD_Init()\r\n");
    MX_SDIO_SD_Init(); /* 或 MX_SDIO_SD_Init_Fix(); 二者参数一致 */

    /* 3) HAL 初始化（上电识别、进入传输态，默认 1-bit） */
    printf("[SD] BSP_SD_Init: verify tick before HAL_SD_Init: %lu\r\n", (unsigned long)HAL_GetTick());
    HAL_Delay(10);
    printf("[SD] BSP_SD_Init: verify tick after  HAL_Delay:  %lu\r\n", (unsigned long)HAL_GetTick());
    printf("[SD] BSP_SD_Init: SDIO force reset\r\n");
    __HAL_RCC_SDIO_FORCE_RESET();
    HAL_Delay(1);
    __HAL_RCC_SDIO_RELEASE_RESET();
    printf("[SD] BSP_SD_Init: HAL_SD_Init() start\r\n");
    if (HAL_SD_Init(&hsd) != HAL_OK) {
        printf("BSP_SD_Init: HAL_SD_Init failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
        return HAL_ERROR;
    }
    printf("[SD] BSP_SD_Init: HAL_SD_Init OK\r\n");

    /* 4) 尝试切换 4-bit（若布线不佳，可保持 1-bit） */
//    if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK) {
//        printf("BSP_SD_Init: switch 4-bit failed, fallback 1-bit, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
//        if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_1B) != HAL_OK) {
//            printf("BSP_SD_Init: fallback 1-bit failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
//            return HAL_ERROR;
//        }
//    }

    /* 5) 读取一次卡信息用于验证 */
    HAL_SD_CardInfoTypeDef info;
    printf("[SD] BSP_SD_Init: HAL_SD_GetCardInfo()\r\n");
    if (HAL_SD_GetCardInfo(&hsd, &info) != HAL_OK) {
        printf("BSP_SD_Init: get card info failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
        return HAL_ERROR;
    }
    printf("[SD] BSP_SD_Init: HAL_SD_GetCardInfo OK\r\n");

    return HAL_OK;
}

/* HAL will handle address units internally based on card type; pass block number directly. */

/* 高层 BSP 实现：使用 HAL DMA 接口并阻塞等待完成（与 core 层分离） */
HAL_StatusTypeDef BSP_SD_ReadBlocks_DMA_Blocking(uint8_t *pData, uint32_t startBlock, uint32_t numBlocks, uint32_t timeout)
{
    /* Start DMA read. Address parameter is logical block address (LBA). */
    if (HAL_SD_ReadBlocks_DMA(&hsd, pData, startBlock, numBlocks) != HAL_OK) {
        printf("BSP_SD_ReadBlocks_DMA_Blocking: start DMA failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
        return HAL_ERROR;
    }

    uint32_t start = HAL_GetTick();
    while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
        if (hsd.ErrorCode != HAL_SD_ERROR_NONE) {
            HAL_SD_Abort(&hsd);
            printf("BSP_SD_ReadBlocks_DMA_Blocking: transfer error, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
            return HAL_ERROR;
        }
        if ((HAL_GetTick() - start) > timeout) {
            HAL_SD_Abort(&hsd);
            printf("BSP_SD_ReadBlocks_DMA_Blocking: timeout, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef BSP_SD_WriteBlocks_DMA_Blocking(uint8_t *pData, uint32_t startBlock, uint32_t numBlocks, uint32_t timeout)
{
    /* Start DMA write. Address parameter is logical block address (LBA). */
    if (HAL_SD_WriteBlocks_DMA(&hsd, pData, startBlock, numBlocks) != HAL_OK) {
        printf("BSP_SD_WriteBlocks_DMA_Blocking: start DMA failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
        return HAL_ERROR;
    }

    uint32_t start = HAL_GetTick();
    while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
        if (hsd.ErrorCode != HAL_SD_ERROR_NONE) {
            HAL_SD_Abort(&hsd);
            printf("BSP_SD_WriteBlocks_DMA_Blocking: transfer error, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
            return HAL_ERROR;
        }
        if ((HAL_GetTick() - start) > timeout) {
            HAL_SD_Abort(&hsd);
            printf("BSP_SD_WriteBlocks_DMA_Blocking: timeout, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef BSP_SD_EraseBlocks(uint32_t startBlock, uint32_t endBlock)
{
    return HAL_SD_Erase(&hsd, startBlock, endBlock);
}

void BSP_SD_PrintCardInfo(void)
{
    HAL_SD_CardInfoTypeDef info;
    if (HAL_SD_GetCardInfo(&hsd, &info) != HAL_OK) {
        printf("BSP_SD: get card info failed\r\n");
        return;
    }

    printf("SD Card Info (BSP):\r\n");
    printf("  CardType: %lu\r\n", (unsigned long)info.CardType);
    printf("  CardVersion: %lu\r\n", (unsigned long)info.CardVersion);
    printf("  Class: %lu\r\n", (unsigned long)info.Class);
    printf("  RelCardAdd: %lu\r\n", (unsigned long)info.RelCardAdd);
    printf("  BlockNbr: %lu\r\n", (unsigned long)info.BlockNbr);
    printf("  BlockSize: %lu\r\n", (unsigned long)info.BlockSize);
    printf("  LogBlockNbr: %lu\r\n", (unsigned long)info.LogBlockNbr);
    printf("  LogBlockSize: %lu\r\n", (unsigned long)info.LogBlockSize);
}

HAL_StatusTypeDef BSP_SD_Test(uint8_t do_write)
{
    uint8_t buf_read[512];
    uint8_t buf_write[512];
    HAL_SD_CardInfoTypeDef info;

    /* 确保已初始化 */
    if (BSP_SD_Init() != HAL_OK) {
        printf("BSP_SD_Test: init failed\r\n");
        return HAL_ERROR;
    }

    if (HAL_SD_GetCardInfo(&hsd, &info) != HAL_OK) {
        printf("BSP_SD_Test: cannot get card info\r\n");
        return HAL_ERROR;
    }

    BSP_SD_PrintCardInfo();

    /* 读取第 0 块作为测试 */
    if (BSP_SD_ReadBlocks_DMA_Blocking(buf_read, 0, 1, 5000) != HAL_OK) {
        printf("BSP_SD_Test: read failed\r\n");
        return HAL_ERROR;
    }

    printf("BSP_SD_Test: read block 0 success\r\n");

    if (do_write) {
        memcpy(buf_write, buf_read, sizeof(buf_write));
        for (int i = 0; i < (int)sizeof(buf_write); ++i) buf_write[i] = (uint8_t)i;

        if (BSP_SD_WriteBlocks_DMA_Blocking(buf_write, 1, 1, 5000) != HAL_OK) {
            printf("BSP_SD_Test: write failed\r\n");
            return HAL_ERROR;
        }

        memset(buf_read, 0, sizeof(buf_read));
        if (BSP_SD_ReadBlocks_DMA_Blocking(buf_read, 1, 1, 5000) != HAL_OK) {
            printf("BSP_SD_Test: readback failed\r\n");
            return HAL_ERROR;
        }

        if (memcmp(buf_read, buf_write, sizeof(buf_read)) != 0) {
            printf("BSP_SD_Test: write verify failed\r\n");
            return HAL_ERROR;
        }

        if (BSP_SD_WriteBlocks_DMA_Blocking(buf_write, 1, 1, 5000) != HAL_OK) {
            printf("BSP_SD_Test: restore failed\r\n");
            return HAL_ERROR;
        }

        printf("BSP_SD_Test: write and verify success\r\n");
    }

    return HAL_OK;
}
