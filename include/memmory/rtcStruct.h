#ifndef RTC_STRUCT_H
#define RTC_STRUCT_H
#include <Arduino.h>


// stan aktualny z pamieci rtc
// stan backup z pamieci flash

    // struktura pomocnicza dla przechowywania czasu bajtów rzeczywistego modułem Real Time Clock zamiast przechowywania w pamięci RTC char
    struct dateTime_bytes{
            byte year;  // last two digits
            byte month;
            byte day;
            byte hour;
            byte minute;
            byte second;
            byte dOW;
    };

    // Dzienny zapis pomiarów do rejestrów pamięcie RTC 
    // struktura dla pomiarów wykonywanych cyklicznie przy kazdym wybudzeniu 
    // backup Flash
    struct RTC_CyclicalMeasuremens {
        float compensatedDistance_rtc;
        float rawdDistance_rtc;
        float temperature_rtc;
        float batteryVoltage_rtc;
        float solarVoltage_rtc;
        dateTime_bytes created_at_bytes;
        uint8_t debugWakeupReason; // debug
    };

    // struktua dla dodatkowych pomiarów wykonywanych raz dziennie   
    // tylko rtc - nie flash
    struct RTC_SingleMeasurements {
        float latitude_rtc;
        float longitude_rtc;
    };

    // DEBUG struktura dla rejestracji niepowodzen w trakcie dzialania modemu 
    // liczniki stanu aktualnego, niekumulatywne
    // tylko rtc - nie flash
    struct RTC_TestStatus {
        uint16_t modemFail_count;
        uint16_t powerOnFail_count;
        uint16_t connectionFail_count;
        uint16_t sendFail_count;
        uint16_t powerOffFail_count;
        uint16_t GpsFail_count;
        uint16_t scndRetry_count;
    };

    // struktura dla rejestracji problemow z resetami software (innych niz = 8 deep Sleep waketup)
    // liczniki kumulatywne i aktualny stan code_OTHER
    // Backup Flash
    struct RTC_problemWakeUpCodes {
        // korupcja RTC
        uint16_t code_1_count; // ESP_RST_POWERON
        uint16_t code_9_count; // ESP_RST_BROWNOUT

        // RTC zachowana, lecz jest to nadmiarowy reset
        uint16_t code_5_count; // ESP_RST_INT_WDT
        uint16_t code_7_count; // ESP_RST_WDT

        // inne 
        uint16_t code_OTHER_count;
        uint8_t  code_OTHER;
        
        // licznik następujących niepowodzen 
        uint8_t failLoop_count; 
    };

   
#endif

 