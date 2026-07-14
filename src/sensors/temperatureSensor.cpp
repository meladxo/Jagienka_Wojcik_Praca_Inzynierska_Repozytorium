/* 
    Osbługa pomiaru temperatury czujnikiem DS18B20
    
    Kod został zaadaptowany na przykładzie dla biblioteki DallasTemperature
    https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/3f570bbe2d0ad3cae55663cfaec1fedbc5b4fa50/examples/Simple/Simple.ino#L40
    MIT License | Copyright (c) 2025 Miles Burton
*/ 
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include "sensors/temperatureSensor.h"
#include "driver/gpio.h"

//utworzenie instancji oneWire DS18B20 
OneWire oneWire(TEMP_SENSOR_PIN);      
DallasTemperature sensors(&oneWire);    

// Wybór rozdzielczosci czujnika i czasu konwersji temperatury: 9bit-94ms; 10bit-188ms; 11bit-375ms; 12bit-750ms 
const uint8_t TEMP_SENSOR_RESOLUTION = 12;   
const int CONVERSION_TIME_MS = 750;      
const float ERROR_TEMP_C = -127.0f;


//weryfikacja czasu konwersji
static unsigned long lastTempRequest = 0;
static unsigned long debug_conversionStart = 0;

/* Funkcja inicjalizacji czujnika w nieblokującym. */
void tempSensorSetup(){   
    sensors.begin();                   
    sensors.setResolution(TEMP_SENSOR_RESOLUTION);   
    
    //wyłączenie trybu blokującego z opóźnieniami Delay
    sensors.setWaitForConversion(false);                                
}


/* Funkcja wyołująca rozpoczęcie konwersji temperatury. */
void requestTemp(){
    sensors.requestTemperatures();
    lastTempRequest = millis();
}


/*
  Funkcja dla odczytu temperatury nieblokującego procesora przy oczekiwaniu na konwersje temperatury przez czujnik.
  Zwraca true dla zakończonego pomiaru.
*/
bool isTempReady(){ 
    if(millis() - lastTempRequest >= CONVERSION_TIME_MS){
        return true;    
    }else{
        return false;
    }
}


/* Funkcja odczytu temperatury, zwraca temperature po konwersji przez czujnik. */
float readTemp(){
    debug_conversionStart = millis() - lastTempRequest; 
    return sensors.getTempCByIndex(0);
}


/* Fukcja mierząca czas konwersji temperatury */
unsigned long debug_getConversionTime(){ 
    return debug_conversionStart;
}   




/* 
  Funkcja przygotowania czujnika do odcięcia zasilania. Zabezpieczenie przed poborem pradu przez podłaczone piny.
*/ 
void tempSensorSleep(){
     //linia danych DS18B20 ustawiona w stan bezpieczny (wysokiej impedancji/float)
    pinMode(TEMP_SENSOR_PIN, INPUT);
    gpio_set_pull_mode((gpio_num_t)TEMP_SENSOR_PIN, GPIO_FLOATING);
    
}