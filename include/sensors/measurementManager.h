#ifndef MEASUREMENT_MANAGER_H
#define MEASUREMENT_MANAGER_H

    #include <Arduino.h>

    void sensorMeasurements(float *temperature, float *distanceRaw, float *distanceCompensated, float *distance_samples, uint8_t sampleCount,uint8_t meanLimit);

#endif
