#ifndef MEMMORY_H
#define MEMMORY_H
#include <memmory/rtcStruct.h>

void FLASH_init();

void FLASH_setFlgBckp();
void FLASH_clearFlgBckp();
bool FLASH_readFlgBckp();

void FLASH_saveBackupAll(const RTC_CyclicalMeasuremens* cyclicalBufor, uint16_t cyclicalCount, uint16_t wakeupCounter, uint16_t daysCounter);
void FLASH_readBackupAll(RTC_CyclicalMeasuremens* cyclicalBufor, uint16_t cyclicalCount, uint16_t* wakeupCounter, uint16_t* daysCounter, RTC_problemWakeUpCodes* RTC_wakeUpCodes, uint32_t* dailySendFailsCount);

void FLASH_readBackupWakeUpCount(uint16_t* wakeupCounter);
void FLASH_readBackupDayCount(uint16_t* daysCounter);

void FLASH_saveBackUpWakeUpCodes(const RTC_problemWakeUpCodes* RTC_wakeUpCodes);
void FLASH_readBackupWakeUpCodes(RTC_problemWakeUpCodes* RTC_wakeUpCodes);

void FLASH_saveDailySendFailCount(uint32_t dailySendFailsCount);
void FLASH_readDailySendFailCount(uint32_t* dailySendFailsCount);

#endif