/**
  ******************************************************************************
  * @file           : file_opera.c
  * @author         : Kevin_WWW
  * @brief          : None
  * @attention      : None
  * @date           : 2024/4/25
  ******************************************************************************
  */
#include "./sd/file_opera.h"
#include "usart.h"
#include "./sdram/mem_placement.h"        /* 外部 SRAM 内存放置宏 */

/* ============================================================================
   文件 I/O 缓冲（迁移至外部 SRAM）
   ============================================================================ */
/**
 * @brief  FatFs 文件读写缓冲
 * @note   放置在外部 SRAM（0x6C000000）以释放内部 SRAM
 *         用于大文件读写、流式处理等场景
 *         容量：64KB
 */
static EXTSRAM uint8_t file_io_buf[64 * 1024];

/**
 * @brief  获取文件 I/O 缓冲指针
 * @return 返回文件缓冲地址
 */
uint8_t* get_file_io_buffer(void)
{
  return file_io_buf;
}

/**
 * @brief  获取文件 I/O 缓冲大小
 * @return 返回文件缓冲大小（64KB）
 */
uint32_t get_file_io_buffer_size(void)
{
  return sizeof(file_io_buf);
}

void fatTest_GetDiskInfo(void) {
    FATFS *fs;
    DWORD free_clust;
    FRESULT res = f_getfree("0:", &free_clust, &fs);
    if (res != FR_OK) {
				printf("f_getfree() error\r\n");
        return;
    }
    printf("*** FAT disk info ***\r\n");
    DWORD total_sector = (fs->n_fatent - 2) * fs->csize;
    DWORD free_sector = free_clust * fs->csize;

#if _MAX_SS == _MIN_SS
    DWORD free_space = free_sector >> 11;
    DWORD total_space = total_sector >> 11;
#else
    DWORD free_space = (free_sector * fs->ssize) >> 10;
    DWORD total_space = (total_sector * fs->ssize) >> 10;
#endif

    char temp_str[40];
    sprintf(temp_str, "FAT type = %d", (int)fs->fs_type);
    printf("%s\r\n", temp_str);
    printf("[1=FAT12,2=FAT16,3=FAT32,4=EXFAT]\r\n");

#if _MAX_SS == _MIN_SS
    sprintf(temp_str, "Sector size(bytes) = %d", _MIN_SS);
#else
    sprintf(temp_str, "Sector size(bytes) = %d", fs->ssize);
#endif
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Cluster size(sectors) = %d", fs->csize);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Total cluster count = %d", fs->n_fatent - 2);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Total sector count = %d", total_sector);
    printf("%s\r\n", temp_str);

#if _MAX_SS == _MIN_SS
    sprintf(temp_str, "Total space(MB) = %d", total_space);
#else
    sprintf(temp_str, "Total space(KB) = %d", total_space);
#endif
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Free cluster count = %d", free_clust);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Free sector count = %d", free_sector);
    printf("%s\r\n", temp_str);

#if _MAX_SS == _MIN_SS
    sprintf(temp_str, "Free space(MB) = %d", free_space);
#else
    sprintf(temp_str, "Free space(KB) = %d", free_space);
#endif
    printf("%s\r\n", temp_str);

    printf("Get FAT disk info OK\r\n");
}

void fatTest_ScanDir(const TCHAR* PathName) {
    DIR dir;
    FILINFO dir_info;
    uint8_t show_line = 9;
    FRESULT res = f_opendir(&dir, PathName);
    if (res != FR_OK) {
        f_closedir(&dir);
        return;
    }

    char temp_str[40];
    sprintf(temp_str, "All entries in dir %s", PathName);
    printf("%s\r\n", temp_str);
    while (1) {
        res = f_readdir(&dir, &dir_info);
        if (res != FR_OK || dir_info.fname[0] == 0) {
            break;
        }
        if (dir_info.fattrib & AM_DIR) {
            sprintf(temp_str, "DIR   %s", dir_info.fname);
            printf("%s\r\n", temp_str);
            show_line++;
        } else {
            sprintf(temp_str, "FILE  %s", dir_info.fname);
            printf("%s\r\n", temp_str);
            show_line++;
        }
    }
    printf("Scan dir OK\r\n");
    f_closedir(&dir);
}

void fatTest_WriteTXTFile(TCHAR* filename, uint16_t year, uint8_t month, uint8_t day) {
    FIL file;
    FRESULT res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK) {
        f_puts("Line1: Hello FatFS\n", &file);
        f_puts("Line2: UPC, Qingdao\n", &file);
        f_printf(&file, "Line3: Date=%04d-%02d-%02d\n", year, month, day);
    } else {
        printf("Write file error\r\n");
    }
    f_close(&file);
}

void fatTest_WriteBinFile(TCHAR* filename, uint32_t pointCount, uint32_t sampFreq) {
    FIL file;
    FRESULT res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK) {
        f_puts("ADC1-IN5\n", &file);

        UINT bw = 0;
        f_write(&file, &pointCount, sizeof (uint32_t), &bw);
        f_write(&file, &sampFreq, sizeof (uint32_t), &bw);

        uint32_t value = 1000;
        for (uint16_t i = 0; i < pointCount; i++, value++) {
            f_write(&file, &value, sizeof (uint32_t), &bw);
        }
    }
    f_close(&file);
}

void fatTest_ReadTXTFile(TCHAR* filename) {
    char temp_str[40];
    sprintf(temp_str, "Reading TXT file: %s", filename);
    printf("%s\r\n", temp_str);

    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    uint8_t show_line = 9;
    if (res == FR_OK) {
        char show_str[40];
        while (!f_eof(&file)) {
            f_gets(show_str, 40, &file);
            printf("%s\r\n", show_str);
            show_line++;
        }
    } else if (res == FR_NO_FILE) {
        sprintf(temp_str, "%s does not exist", filename);
        printf("%s\r\n", temp_str);
    } else {
        printf("f_open() error\r\n");
    }
    f_close(&file);
}

void fatTest_ReadBinFile(TCHAR* filename) {
    char temp_str[40];
    sprintf(temp_str, "Reading BIN file: %s", filename);
    printf("%s\r\n", temp_str);
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res == FR_OK) {
        char show_str[40];
        f_gets(show_str, 40, &file);

        UINT bw = 0;
        uint32_t pointCount;
        uint32_t sampFreq;
        f_read(&file, &pointCount, sizeof (uint32_t), &bw);
        f_read(&file, &sampFreq, sizeof (uint32_t), &bw);
        uint32_t value[pointCount];
        for (uint16_t i = 0; i < pointCount; ++i) {
            f_read(&file, &value[i], sizeof (uint32_t), &bw);
        }

    printf("%s\r\n", show_str);

    sprintf(temp_str, "Point count: %d", pointCount);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "Sampling frequency: %d", sampFreq);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "value[5] = %d", value[5]);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "value[16] = %d", value[16]);
    printf("%s\r\n", temp_str);
    } else if (res == FR_NO_FILE) {
        sprintf(temp_str, "%s does not exist", filename);
        printf("%s\r\n", temp_str);
    } else {
        printf("f_open() error\r\n");
    }
    f_close(&file);
}

void fatTest_GetFileInfo(TCHAR* filename) {
    char temp_str[40];
    sprintf(temp_str, "File info of: %s", filename);
    printf("%s\r\n", temp_str);

    FILINFO file_info;
    FRESULT res = f_stat(filename, &file_info);
    if (res == FR_OK) {
    sprintf(temp_str, "File size(bytes)= %d", file_info.fsize);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "File attribute = 0x%X", file_info.fattrib);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "File name = %s", file_info.fname);
    printf("%s\r\n", temp_str);

        RTC_DateTypeDef sDate;
        RTC_TimeTypeDef sTime;

        sDate.Date = file_info.fdate & 0x001F;
        sDate.Month = (file_info.fdate & 0x01E0) >> 5;
        sDate.Year = 1980 + ((file_info.fdate & 0xFE00) >> 9) - 2000;

        sTime.Hours = (file_info.ftime & 0xF800) >> 11;
        sTime.Minutes = (file_info.ftime & 0x07E0) >> 5;
        sTime.Seconds = (file_info.ftime & 0x001F) << 1;


    sprintf(temp_str, "File Date = 20%02d-%02d-%02d", sDate.Year, sDate.Month, sDate.Date);
    printf("%s\r\n", temp_str);

    sprintf(temp_str, "File Time = %02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);
    printf("%s\r\n", temp_str);
    } else if (res == FR_NO_FILE) {
    sprintf(temp_str, "%s does not exist", filename);
    printf("%s\r\n", temp_str);
    } else {
    printf("f_stat() error\r\n");
    }
}

DWORD fat_GetFatTimeFromRTC(void) {
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK) {
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        WORD date = ((2000 + sDate.Year - 1980) << 9) | (sDate.Month << 5) | sDate.Date;
        WORD time = (sTime.Hours << 11) | (sTime.Minutes << 5) | (sTime.Seconds >> 1);

        DWORD fatTime = ((DWORD)date << 16) | time;
        return fatTime;
    } else {
        return 0;
    }
}
