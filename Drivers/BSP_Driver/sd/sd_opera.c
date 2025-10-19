/**
  ******************************************************************************
  * @file           : sd_opera.c
  * @author         : Kevin_WWW
  * @brief          : None
  * @attention      : None
  * @date           : 2024/5/5
  ******************************************************************************
  */
#include "./sd/sd_opera.h"
#include "usart.h"

void SDCard_ShowInfo(void) {
    HAL_SD_CardInfoTypeDef cardInfo;
    HAL_StatusTypeDef res = HAL_SD_GetCardInfo(&hsd, &cardInfo);
    if (res != HAL_OK) {
        printf("HAL_SD_GetCardInfo() error\r\n");
        return;
    }

    printf("*** HAL_SD_GetCardInfo() info ***\r\n");

    char temp_str[40];
    sprintf(temp_str, "Card Type=%d", cardInfo.CardType);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Card Version=%d", cardInfo.CardVersion);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Card Class=%d", cardInfo.Class);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Relative Card Address=%d", cardInfo.RelCardAdd);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Block Count=%d", cardInfo.BlockNbr);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Block Size(Bytes)=%d", cardInfo.BlockSize);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Logic Block Count=%d", cardInfo.LogBlockNbr);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Logic Block Size(Bytes)=%d", cardInfo.LogBlockSize);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "SD Card Capacity(MB)=%d", cardInfo.BlockNbr / 1024 * cardInfo.BlockSize / 1024);
    printf("%s\r\n", temp_str);
		

}

void SDCard_EraseBlocks(void) {
    printf("*** Erasing blocks ***\r\n");
    uint32_t BlockStartAdd = 0;
    uint32_t BlockEndAdd = 10;
    HAL_StatusTypeDef res = HAL_SD_Erase(&hsd, BlockStartAdd, BlockEndAdd);
    if (res == HAL_OK) {
        printf("Erase blocks success\r\n");
    } else {
        printf("Erase blocks error\r\n");
    }
    HAL_SD_CardStateTypeDef cardState = HAL_SD_GetCardState(&hsd);
    char temp_str[40];
    sprintf(temp_str, "GetCardState() = %d", cardState);
    printf("%s\r\n", temp_str);
    while (cardState != HAL_SD_CARD_TRANSFER) {
        HAL_Delay(1);
        cardState = HAL_SD_GetCardState(&hsd);
    }
    sprintf(temp_str, "GetCardState() = %d", cardState);
    printf("%s\r\n", temp_str);
    printf("Blocks 0-10 is erased.\r\n");
		

}

void SDCard_TestWrite(void) {
    printf("*** Writing blocks ***\r\n");
    uint8_t pData[BLOCKSIZE] = "Hello, welcome to UPC\0";
    uint32_t BlockAdd = 5;
    uint32_t NumberOfBlocks = 1;
    uint32_t Timeout = 1000;
    HAL_StatusTypeDef res = HAL_SD_WriteBlocks(&hsd, pData, BlockAdd, NumberOfBlocks, Timeout);
    if (res == HAL_OK) {
        printf("Write to Block 5 is OK\r\n");
        printf("The string is :\r\n");
    printf("%s\r\n", (char *)pData);
    } else {
        printf("Write to Block 5 is error\r\n");
    }
		

}

void SDCard_TestRead(void) {
    printf("*** Reading blocks ***\r\n");
    uint8_t pData[BLOCKSIZE];
    uint32_t BlockAdd = 5;
    uint32_t NumberOfBlocks = 1;
    uint32_t Timeout = 1000;
    HAL_StatusTypeDef res = HAL_SD_ReadBlocks(&hsd, pData, BlockAdd, NumberOfBlocks, Timeout);
    if (res == HAL_OK) {
        printf("Read Block 5 is OK\r\n");
    printf("The string is :\r\n");
    printf("%s\r\n", (char *)pData);
    } else {
        printf("Read Block 5 is error\r\n");
			
    }
}

