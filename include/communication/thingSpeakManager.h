#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H     

    #include "memmory/rtcStruct.h"
    #include <Arduino.h>

    String buildJson(const RTC_CyclicalMeasuremens* cyclicalBufor, uint16_t buforSize, 
        const RTC_SingleMeasurements* singleBufor, const RTC_TestStatus* statusBufor, 
        uint16_t dayCounter, const RTC_problemWakeUpCodes* RTC_wakeUpCodes, 
        uint32_t dailySendFailsCount);
 
    bool sendToThingSpeak(String JSON);
    
#endif
