#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H
    
    #include <Arduino.h>
    #include "memmory/rtcStruct.h"

    #define SDA_PIN 21
    #define SCL_PIN 22

    void timeManager_getNtpTimeUTC(dateTime_bytes* NtpBuff);
    void timeManager_setUp_DS3231();
    void timeManager_read_DS3231(dateTime_bytes *timeBuff);
    String timeManager_bytesToISO(const dateTime_bytes &buff);
    void rtcPinsSleep();


#endif