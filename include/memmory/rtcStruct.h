#ifndef RTC_STRUCT_H
#define RTC_STRUCT_H
#include <Arduino.h>

    // struktura pomocnicza dla przechowywania czasu bajtów rzeczywistego modułem Real Time Clock  
    struct dateTime_bytes{
            byte year;                          // 2 ostatnie cyfry
            byte month;
            byte day;
            byte hour;
            byte minute;
            byte second;
            byte dOW;
    };

    // struktura dla pomiarów wykonywanych cyklicznie przy kazdym wybudzeniu 
    // cogodzinne zapi do rejestrów pamięci RTC i 1 na dzien backup danych FLASH
    struct RTC_CyclicalMeasuremens {
        float compensatedDistance_rtc;
        float rawdDistance_rtc;
        float temperature_rtc;
        float batteryVoltage_rtc;
        float solarVoltage_rtc;
        dateTime_bytes created_at_bytes;
        uint8_t debugWakeupReason;              // dane debug
    };

    // struktua dla dodatkowych pomiarów wykonywanych raz dziennie (raz w tygodniu)
    // zapis do RTC
    struct RTC_SingleMeasurements {
        float latitude_rtc;
        float longitude_rtc;
    };

    // DEBUG struktura pomocnicza dla rejestracji niepowodzen w trakcie dzialania modemu, liczniki kumulatywne
    // zapis do RTC
    struct RTC_TestStatus {
        uint16_t modemFail_count;          
        uint16_t powerOnFail_count;
        uint16_t connectionFail_count;
        uint16_t sendFail_count;
        uint16_t powerOffFail_count;
        uint16_t scndRetry_count;
    };

    // struktura dla rejestracji problemow z resetami software (innych niz = 8 deep Sleep waketup)
    // liczniki kumulatywne code_X_count
    // aktualna informacja o kodzie nieoczekiwanego resetu code_OTHER oraz licznik resetowany licznik następujących resetów failLoop_count
    // zapis do RTC i backup FLASH
    struct RTC_problemWakeUpCodes {
        // korupcja RTC
        uint16_t code_1_count;      // ESP_RST_POWERON
        uint16_t code_9_count;      // ESP_RST_BROWNOUT

        // RTC zachowana, lecz jest to nadmiarowy reset
        uint16_t code_5_count;      // ESP_RST_INT_WDT
        uint16_t code_7_count;      // ESP_RST_WDT

        // inne nieoczekiwane kody
        uint16_t code_OTHER_count;
        uint8_t  code_OTHER;
        
        // licznik następujących po sobie niepowodzen 
        uint8_t failLoop_count; 
    };
   
#endif

 