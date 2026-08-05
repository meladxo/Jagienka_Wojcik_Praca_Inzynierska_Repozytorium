/*
    Obsługa odczytu napięcia baterii oraz napięcia wyjściowego panelu słonecznego
    
    Kod został zaadaptowany na podstawie przykładu producenta płytek LilyGo 
    - ReadBattery :  https://github.com/Xinyuan-LilyGO/LilyGo-Modem-Series/tree/main/examples
        author    Lewis He (lewishe@outlook.com)
        license   MIT
        Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
*/
#include "lilygoDefines.h"
#include "debug.h"
#include "power/batteryMonitor.h"
#include <Arduino.h>

/*
  Funkcja odczytu napięcia baterii oraz napięcia wyjściowego panelu słonecznego
  Parametrem są wstaźniki na zmienne do których zostaną zapisane wyniki
*/
void batteryMonitor_readVoltage(float *batteryVoltage, float *solarVoltage){

    // ustawienie tłumienia 11dB dla maksymalnej skali pomiaru i maksymalnej rozdzielczości
    // sugerowana skala maksymalne liniowe odczyty do ~2,45V (zamiast matematycznej 3,9V)
    analogSetAttenuation(ADC_11db);
    analogReadResolution(12);
    #if CONFIG_IDF_TARGET_ESP32
        analogSetWidth(12);
    #endif

    // płytka lilygo wykorzystuje dzielnik mierzący połowę rzeczywistego napięcia z uwagi na skalę ADC11db ~2,45V
    uint32_t battery_voltage_mV = analogReadMilliVolts(BOARD_BAT_ADC_PIN); // analogReadMilliVolts z kalibracja vref (które dla esp32 jest róźne 1000mV - 1200mV)
    battery_voltage_mV *= 2;   //dzielnik napięcia *2
    *batteryVoltage = battery_voltage_mV / 1000.0f;

    uint32_t solar_voltage_mV = analogReadMilliVolts(BOARD_SOLAR_ADC_PIN);
    solar_voltage_mV *= 2;     //dzielnik napięcia *2
    *solarVoltage = solar_voltage_mV / 1000.0f;
    
    if(isDebug){  
        Serial.println("------------------- Pomiar Napięcia ---------------------"); 
        Serial.print(*batteryVoltage,3);
        Serial.println("V (battery); ");
        Serial.print(*solarVoltage,3);
        Serial.println("V (solar); ");
        Serial.println("---------------------------------------------------------");
    }
}