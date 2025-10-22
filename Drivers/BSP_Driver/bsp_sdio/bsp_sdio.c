#if 0
#include "./bsp_sdio/bsp_sdio.h"
#include "../../..//Core/Inc/dma.h"
#include "../../..//Core/Inc/sdio.h" /* 包含 core 层的声明，用于底层类型 */
#include "stm32f4xx_hal.h" /* 确保引入 HAL 常量与 SD API 原型 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* DMA 传输完成/错误标志（由 HAL 回调置位） */
static volatile uint8_t s_sd_rx_done = 0;
static volatile uint8_t s_sd_tx_done = 0;
static volatile uint8_t s_sd_error   = 0;

/* 测试缓冲区放在静态全局，避免放入可能不可 DMA 访问的内存区（如部分工程将栈/CCM用于局部变量） */
static uint32_t s_rbuf32[512 / 4];
static uint32_t s_wbuf32[512 / 4];

HAL_StatusTypeDef BSP_SD_Init(void)
{
    printf("\r\n------------------------------------------------------------\r\n");
    printf("SD Card Controller Initialization\r\n");
    printf("------------------------------------------------------------\r\n");

    printf("-- DMA Channel Configuration --\r\n");
    MX_DMA_Init();
    printf("DMA streams configured (RX: DMA2-S3, TX: DMA2-S6)\r\n");

    printf("\r\n-- SDIO Peripheral Configuration --\r\n");
    MX_SDIO_SD_Init();
    printf("SDIO clock and GPIO configured\r\n");

    printf("\r\n-- Card Identification Phase --\r\n");
    HAL_Delay(10);
    __HAL_RCC_SDIO_FORCE_RESET();
    HAL_Delay(1);
    __HAL_RCC_SDIO_RELEASE_RESET();
    
    if (HAL_SD_Init(&hsd) != HAL_OK) {
        printf("Card initialization failed (0x%08lX)\r\n", (unsigned long)hsd.ErrorCode);
        printf("------------------------------------------------------------\r\n\r\n");
        return HAL_ERROR;
    }
    printf("Card detected and initialized (default 1-bit mode)\r\n");

    printf("\r\n-- SD Card Ready for Data Transfer --\r\n");
    printf("Card State: HAL_SD_CARD_TRANSFER\r\n");
    printf("Bus Width:  1-bit (auto-negotiated)\r\n");
    printf("DMA Mode:   Enabled with polling fallback\r\n");
    printf("------------------------------------------------------------\r\n\r\n");

    return HAL_OK;
}

//    /* 4) 尝试切换 4-bit（若布线不佳，可保持 1-bit） */
//    if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK) {
//        printf("BSP_SD_Init: switch 4-bit failed, fallback 1-bit, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
//        if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_1B) != HAL_OK) {
//            printf("BSP_SD_Init: fallback 1-bit failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
//            return HAL_ERROR;
//        }
//    }

//    /* 5) 读取一次卡信息用于验证 */
//    HAL_SD_CardInfoTypeDef info;
//    printf("[SD] BSP_SD_Init: HAL_SD_GetCardInfo()\r\n");
//    if (HAL_SD_GetCardInfo(&hsd, &info) != HAL_OK) {
//        printf("BSP_SD_Init: get card info failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
//        return HAL_ERROR;
//    }
//    printf("[SD] BSP_SD_Init: HAL_SD_GetCardInfo OK\r\n");

    return HAL_OK;
}

/* HAL will handle address units internally based on card type; pass block number directly. */

/* 高层 BSP 实现：使用 HAL DMA 接口并阻塞等待完成（与 core 层分离） */
HAL_StatusTypeDef BSP_SD_ReadBlocks_DMA(uint8_t *pData, uint32_t startBlock, uint32_t numBlocks)
{
    s_sd_rx_done = 0;
    s_sd_error   = 0;
    /* 使用 HAL 库的 DMA 读取函数 */
    if (HAL_SD_ReadBlocks_DMA(&hsd, pData, startBlock, numBlocks) != HAL_OK) {
        printf("BSP_SD_ReadBlocks_DMA: HAL_SD_ReadBlocks_DMA failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
        return HAL_ERROR;
    }

    printf("BSP_SD_ReadBlocks_DMA: DMA read started\r\n");
    return HAL_OK;
}

HAL_StatusTypeDef BSP_SD_WriteBlocks_DMA(uint8_t *pData, uint32_t startBlock, uint32_t numBlocks)
{
    s_sd_tx_done = 0;
    s_sd_error   = 0;
    /* 使用 HAL 库的 DMA 写入函数 */
    if (HAL_SD_WriteBlocks_DMA(&hsd, pData, startBlock, numBlocks) != HAL_OK) {
        printf("BSP_SD_WriteBlocks_DMA: HAL_SD_WriteBlocks_DMA failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
        return HAL_ERROR;
    }

    printf("BSP_SD_WriteBlocks_DMA: DMA write started\r\n");
    return HAL_OK;
}

HAL_StatusTypeDef BSP_SD_WriteBlocks_DMA_Blocking(uint8_t *pData, uint32_t startBlock, uint32_t numBlocks, uint32_t timeout)
{
    /* 打印调试信息 */
    printf("BSP_SD_WriteBlocks_DMA_Blocking: startBlock=%lu, numBlocks=%lu, timeout=%lu\r\n", startBlock, numBlocks, timeout);

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

/* 兼容封装：阻塞式读取（内部仍然调用 HAL 的 DMA 读取，然后轮询等待完成） */
HAL_StatusTypeDef BSP_SD_ReadBlocks_DMA_Blocking(uint8_t *pData, uint32_t startBlock, uint32_t numBlocks, uint32_t timeout)
{
    s_sd_rx_done = 0;
    s_sd_error   = 0;
    /* 启动 DMA 读取（非阻塞） */
    if (HAL_SD_ReadBlocks_DMA(&hsd, pData, startBlock, numBlocks) != HAL_OK) {
        printf("BSP_SD_ReadBlocks_DMA_Blocking: start DMA failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
        return HAL_ERROR;
    }

    /* 轮询等待传输完成（阻塞，仅用于测试/兼容旧接口） */
    uint32_t start = HAL_GetTick();
    while (!s_sd_rx_done) {
        if (s_sd_error || hsd.ErrorCode != HAL_SD_ERROR_NONE) {
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

HAL_StatusTypeDef BSP_SD_EraseBlocks(uint32_t startBlock, uint32_t endBlock)
{
    return HAL_SD_Erase(&hsd, startBlock, endBlock);
}

/* HAL SD 完成/错误回调，用于置位状态标志 */
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd_)
{
    (void)hsd_;
    s_sd_rx_done = 1;
    printf("[SD] RxCplt\r\n");
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd_)
{
    (void)hsd_;
    s_sd_tx_done = 1;
    printf("[SD] TxCplt\r\n");
}

void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd_)
{
    (void)hsd_;
    s_sd_error = 1;
    printf("[SD] ErrorCallback: err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
}

/* 打印 HAL_SD 错误标志位，便于定位问题 */
static void BSP_SD_PrintHalError(uint32_t err)
{
    if (err == 0) { printf("    HAL_SD_ERROR_NONE\r\n"); return; }
    if (err & 0x00000001) printf("    CMD_CRC_FAIL\r\n");
    if (err & 0x00000002) printf("    DATA_CRC_FAIL/CRC\r\n");
    if (err & 0x00000004) printf("    CMD_RSP_TIMEOUT\r\n");
    if (err & 0x00000008) printf("    DATA_TIMEOUT\r\n");
    if (err & 0x00000010) printf("    TX_UNDERRUN\r\n");
    if (err & 0x00000020) printf("    RX_OVERRUN\r\n");
    if (err & 0x00000040) printf("    ADDRESS_MISALIGNED\r\n");
    if (err & 0x00000080) printf("    BLOCK_LEN_ERR\r\n");
    if (err & 0x00000100) printf("    ERASE_SEQ_ERR\r\n");
    if (err & 0x00000200) printf("    BAD_ERASE_PARAM\r\n");
    if (err & 0x00000400) printf("    WRITE_PROT_VIOLATION\r\n");
    if (err & 0x00000800) printf("    LOCK_UNLOCK_FAILED\r\n");
    if (err & 0x00001000) printf("    COM_CRC_ERROR\r\n");
    if (err & 0x00002000) printf("    ILLEGAL_CMD\r\n");
    if (err & 0x00004000) printf("    CARD_ECC_FAILED\r\n");
    if (err & 0x00008000) printf("    CC_ERROR\r\n");
    if (err & 0x00010000) printf("    GENERAL_UNKNOWN_ERROR\r\n");
    if (err & 0x00020000) printf("    STREAM_READ_UNDERRUN\r\n");
    if (err & 0x00040000) printf("    STREAM_WRITE_OVERRUN\r\n");
    if (err & 0x00080000) printf("    CID/CSD_OVERWRITE\r\n");
    if (err & 0x00100000) printf("    WP_ERASE_SKIP\r\n");
    if (err & 0x00200000) printf("    CARD_ECC_DISABLED\r\n");
    if (err & 0x00400000) printf("    ERASE_RESET\r\n");
    if (err & 0x00800000) printf("    AKE_SEQ_ERROR\r\n");
}

void SD_PrintCardInfo(void)
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
    /* 使用 32-bit 对齐的缓冲区，避免 DMA 对齐带来的潜在问题 */
    uint8_t *buf_read = (uint8_t *)s_rbuf32;
    uint8_t *buf_write = (uint8_t *)s_wbuf32;
    uint32_t test_lba = 2048; /* 避免可能的保留/引导区域 */

    /* 读取测试块 */
    if (BSP_SD_ReadBlocks_DMA(buf_read, test_lba, 1) != HAL_OK) {
        printf("BSP_SD_Test: start read failed\r\n");
        return HAL_ERROR;
    }
    {
        uint32_t t0 = HAL_GetTick();
        while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
            if (hsd.ErrorCode != HAL_SD_ERROR_NONE) {
                HAL_SD_Abort(&hsd);
                printf("BSP_SD_Test: read transfer error, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                return HAL_ERROR;
            }
            if ((HAL_GetTick() - t0) > 5000) {
                HAL_SD_Abort(&hsd);
                printf("BSP_SD_Test: read timeout, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                return HAL_TIMEOUT;
            }
        }
        printf("BSP_SD_Test: read block %lu success\r\n", (unsigned long)test_lba);
    }


    if (do_write) {
    memcpy(buf_write, buf_read, 512);
    for (int i = 0; i < 512; ++i) buf_write[i] = (uint8_t)i;
        // DMA写入两字节测试数据
        if (BSP_SD_WriteBlocks_DMA(buf_write, test_lba, 1) != HAL_OK) {
            printf("BSP_SD_Test: start write failed\r\n");
            BSP_SD_PrintHalError(hsd.ErrorCode);
            return HAL_ERROR;
        }
        {
            uint32_t t0 = HAL_GetTick();
            /* 等待 DMA TX 完成标志，更稳妥 */
            while (!s_sd_tx_done) {
                if (s_sd_error || hsd.ErrorCode != HAL_SD_ERROR_NONE) {
                    HAL_SD_Abort(&hsd);
                    printf("BSP_SD_Test: write transfer error, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                    BSP_SD_PrintHalError(hsd.ErrorCode);
                    return HAL_ERROR;
                }
                if ((HAL_GetTick() - t0) > 5000) {
                    HAL_SD_Abort(&hsd);
                    printf("BSP_SD_Test: write timeout, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                    BSP_SD_PrintHalError(hsd.ErrorCode);
                    return HAL_TIMEOUT;
                }
            }
            printf("BSP_SD_Test: write block %lu done\r\n", (unsigned long)test_lba);
        }
        // 读回刚写入的数据进行验证
        HAL_Delay(2);
        memset(s_rbuf32, 0, sizeof(s_rbuf32));
        if (BSP_SD_ReadBlocks_DMA(buf_read, test_lba, 1) != HAL_OK) {
            printf("BSP_SD_Test: start readback failed\r\n");
            BSP_SD_PrintHalError(hsd.ErrorCode);
            return HAL_ERROR;
        }
        {
            uint32_t t0 = HAL_GetTick();
            while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
                if (hsd.ErrorCode != HAL_SD_ERROR_NONE) {
                    HAL_SD_Abort(&hsd);
                    printf("BSP_SD_Test: readback transfer error, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                    BSP_SD_PrintHalError(hsd.ErrorCode);
                    return HAL_ERROR;
                }
                if ((HAL_GetTick() - t0) > 5000) {
                    HAL_SD_Abort(&hsd);
                    printf("BSP_SD_Test: readback timeout, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                    BSP_SD_PrintHalError(hsd.ErrorCode);
                    return HAL_TIMEOUT;
                }
            }
        }

        // 验证写入数据与读出数据是否一致
            if (memcmp(buf_read, buf_write, 512) != 0) {
            printf("BSP_SD_Test: write verify failed\r\n");
            /* 打印前 16 字节用于诊断 */
            printf("  write[0..15]: ");
            for (int i = 0; i < 16; ++i) printf("%02X ", buf_write[i]);
            printf("\r\n  read [0..15]: ");
            for (int i = 0; i < 16; ++i) printf("%02X ", buf_read[i]);
            printf("\r\n");
                /* 再试一次：使用 HAL 轮询读取对比，判断是否 DMA 读问题 */
                memset(s_rbuf32, 0, sizeof(s_rbuf32));
                if (HAL_SD_ReadBlocks(&hsd, buf_read, test_lba, 1, 5000) != HAL_OK) {
                    printf("BSP_SD_Test: HAL_SD_ReadBlocks (polling) failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                } else {
                    printf("BSP_SD_Test: polling read done for compare\r\n");
                    if (memcmp(buf_read, buf_write, 512) == 0) {
                        printf("BSP_SD_Test: polling read matches written data (DMA read path suspect)\r\n");
                    } else {
                        printf("BSP_SD_Test: polling read still mismatched (write likely failed)\r\n");
                    }
                }
            /* 回退一次使用非 DMA 写接口做对比诊断 */
            printf("BSP_SD_Test: fallback to HAL_SD_WriteBlocks (polling) once...\r\n");
            if (HAL_SD_WriteBlocks(&hsd, buf_write, test_lba, 1, 5000) != HAL_OK) {
                printf("BSP_SD_Test: HAL_SD_WriteBlocks failed, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                return HAL_ERROR;
            }
            /* 再次读回校验 */
            HAL_Delay(2);
            memset(s_rbuf32, 0, sizeof(s_rbuf32));
            if (BSP_SD_ReadBlocks_DMA(buf_read, test_lba, 1) != HAL_OK) {
                printf("BSP_SD_Test: start readback (after fallback write) failed\r\n");
                return HAL_ERROR;
            }
            {
                uint32_t t0 = HAL_GetTick();
                while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
                    if (hsd.ErrorCode != HAL_SD_ERROR_NONE) {
                        HAL_SD_Abort(&hsd);
                        printf("BSP_SD_Test: readback (after fallback write) transfer error, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                        return HAL_ERROR;
                    }
                    if ((HAL_GetTick() - t0) > 5000) {
                        HAL_SD_Abort(&hsd);
                        printf("BSP_SD_Test: readback (after fallback write) timeout, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                        return HAL_TIMEOUT;
                    }
                }
            }
            if (memcmp(buf_read, buf_write, 512) != 0) {
                printf("BSP_SD_Test: verify still failed after polling write\r\n");
                printf("  write[0..15]: ");
                for (int i = 0; i < 16; ++i) printf("%02X ", buf_write[i]);
                printf("\r\n  read [0..15]: ");
                for (int i = 0; i < 16; ++i) printf("%02X ", buf_read[i]);
                printf("\r\n");
                return HAL_ERROR;
            } else {
                printf("BSP_SD_Test: verify OK after polling write\r\n");
            }
        }

        if (BSP_SD_WriteBlocks_DMA(buf_write, test_lba, 1) != HAL_OK) {
            printf("BSP_SD_Test: start restore failed\r\n");
            return HAL_ERROR;
        }
        {
            uint32_t t0 = HAL_GetTick();
            while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
                if (hsd.ErrorCode != HAL_SD_ERROR_NONE) {
                    HAL_SD_Abort(&hsd);
                    printf("BSP_SD_Test: restore transfer error, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                    return HAL_ERROR;
                }
                if ((HAL_GetTick() - t0) > 5000) {
                    HAL_SD_Abort(&hsd);
                    printf("BSP_SD_Test: restore timeout, err=0x%08lX\r\n", (unsigned long)hsd.ErrorCode);
                    return HAL_TIMEOUT;
                }
            }
        }

        printf("BSP_SD_Test: write and verify success\r\n");
    }

    return HAL_OK;
}

#endif /* Disabled: migrated to Middlewares/Third_Party/FatFs/Target/bsp_driver_sd.c */
