/**
  ******************************************************************************
  * @file           : file_opera.h
  * @author         : Kevin_WWW
  * @brief          : None
  * @attention      : None
  * @date           : 2024/4/25
  ******************************************************************************
  */
#ifndef DEMO12_1FLASHFAT_FILE_OPERA_H
#define DEMO12_1FLASHFAT_FILE_OPERA_H

#include "ff.h"
#include <stdio.h>
#include "rtc.h"

void fatTest_GetDiskInfo(void);
void fatTest_ScanDir(const TCHAR* PathName);
void fatTest_WriteTXTFile(TCHAR* filename, uint16_t year, uint8_t month, uint8_t day);
void fatTest_WriteBinFile(TCHAR* filename, uint32_t pointCount, uint32_t sampFreq);
void fatTest_ReadTXTFile(TCHAR* filename);
void fatTest_ReadBinFile(TCHAR* filename);
void fatTest_GetFileInfo(TCHAR* filename);

DWORD fat_GetFatTimeFromRTC(void);

#endif //DEMO12_1FLASHFAT_FILE_OPERA_H
