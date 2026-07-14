#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H     

    #define TEMP_SENSOR_PIN (19)   //DS18B20 podłączony pin data line z rezystorem 4.7kOhm
    extern const float ERROR_TEMP_C;

    void tempSensorSetup();
    void requestTemp();
    float readTemp();
    bool isTempReady();
    unsigned long debug_getConversionTime();
    void tempSensorSleep();

#endif



 