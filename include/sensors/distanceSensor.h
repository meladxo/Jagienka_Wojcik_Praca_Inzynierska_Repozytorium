#ifndef DISTANCE_SENSOR_H
#define DISTANCE_SENSOR_H

    #include <Arduino.h>

    #define DISTANCE_SENSOR_TX_PIN (34)  //RX Lilygo - TX czujnika 
    #define DISTANCE_SENSOR_RX_PIN (32)  //TX Lilygo - RX czujnika (LOW=realtime 100ms, HIGH/float=stable 300ms)
    #define SerialDistanceSensor Serial2  

    extern const uint16_t DISTNACE_MIN_RANGE_CM; // zakres czujnika
    extern const uint16_t DISTNACE_MAX_RANGE_CM; 
    extern const float DISTANCE_ERROR;       
    extern const float DISTANCE_RISK_TEMP_C;    // limit bezpiecznej pracy czujnika > -15C


    void distanceSensor_SetUp();
    bool distanceSensor_isReady(float* distance);
    float distanceSensor_tempCompensation(float temp, float distance);
    void distanceSensorSleep();
    
#endif



 