#ifndef DEEP_SLEEP_H
#define DEEP_SLEEP_H
    
    #include <Arduino.h>
 
    void deepSleepWakeUp();
    uint8_t deepSleepGetResetReason();
    void goIntoDeepSleep(uint32_t timeToSleepSec);
    void lightSleepDelay(uint32_t ms);

#endif