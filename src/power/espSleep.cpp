/*
    Obłsuga trybów oszczędzania energii esp32 
    Kod został zaadaptowany na bazie przykładów producenta płytki Lilygo DeepSlep, PowerMonitoring :
    https://github.com/Xinyuan-LilyGO/LilyGo-Modem-Series/tree/main/
        author    Lewis He (lewishe@outlook.com)
        license   MIT
        copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
*/
#include "power/espSleep.h"
#include "lilygoDefines.h"
#include <Arduino.h>
#include "debug.h"
#include <WiFi.h>

static const uint64_t sec_TO_uS_FACTOR = 1000000ULL;
static uint8_t lastResetReason = 0;

/*
  Funkcja wybudająca esp32 ze stanu deep sleep.
  Wyłączająca nie uzywane modułu radiowe ESP32
*/
void deepSleepWakeUp(){
  lastResetReason = (uint8_t)esp_reset_reason();
   
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
}

/*
  Funkcja zapisująca kod źródła wybudzenia ESP32
*/ 
uint8_t deepSleepGetResetReason(){
  return lastResetReason;
}


/*
  Funkcja wprowadzenia esp32  w stan deep sleep (raport producenta pomiaru dla T-A7670-ESP32 ~157 uA) 
  Parametrem jest czas sekund na który esp32 zostaną uśpione. 
*/
void goIntoDeepSleep(uint32_t timeToSleepSec){
    if(isDebug){ 
        Serial.println("Enter esp32 deepsleep!");
        Serial.println();
    }
    gpio_deep_sleep_hold_en();  // poszczególne piny usawiane gpio_hold_en()
    esp_sleep_enable_timer_wakeup(((uint64_t)timeToSleepSec) * sec_TO_uS_FACTOR);  
    esp_deep_sleep_start();
}
 
/*
  Funkcja wprowadzenia esp32  w stan light sleep
  Parametrem jest czas milisekuns na który esp32 zostaną uśpione. 
*/
void lightSleepDelay(uint32_t ms){
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}


