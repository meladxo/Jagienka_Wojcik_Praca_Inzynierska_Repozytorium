/*
    Obłusga sterowania zasilaniem dla czujnikow 
    z wykorzystaniem tranzystora P-kanałowy MOSFET.
*/
#include "debug.h"
#include "power/mosfet.h"
#include <Arduino.h>
#include <driver/gpio.h>

void mosfetON(){
    gpio_hold_dis((gpio_num_t)MOSFET_GATE_PIN);
    pinMode(MOSFET_GATE_PIN, OUTPUT);      
    digitalWrite(MOSFET_GATE_PIN, LOW);
    if(isDebug){ Serial.println("Tranzystor MOSFET przewodzi - czujniki zostają zasilane"); } 
}

void mosfetOFF(){
    pinMode(MOSFET_GATE_PIN, OUTPUT);      
    digitalWrite(MOSFET_GATE_PIN, HIGH);
   
    /*pin MOSFET_GATE_PIN nie nalezy do domeny RTC (zewnetrzny rezystor pull-up odpowiada za utrzymanie stanu)
        pin nie zostanie utrzymany w stanie głębokiego uśpienia,
        za utrzymanie odpowiada więc zewnętrzny rezystor pullup 10kOhm
    */ 
    gpio_hold_en((gpio_num_t)MOSFET_GATE_PIN);  // mozna zakomentować

    if(isDebug){ Serial.println("Tranzystor MOSFET zatkany - zasilanie zostało odcięte"); } 
}
       