/*
    Obłsuga wysyłki danych do serveru ThingSpeak metodą bulk-write JSON:
    - odczyty danych zapisanych w rejestrze pamięcie RTC 
    - budowy ciała wiadomości JSON z wykozystaniem biblioteki ArduinoJson
    - wysyłki danych do kanału thingSpeak

    Wymagania ThingSpeak :
    https://www.mathworks.com/help/thingspeak/bulkwritejsondata.html
*/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <math.h>
#include "lilygoDefines.h"
#include <TinyGsmClient.h>
#include "debug.h"

#include "memmory/rtcStruct.h"
#include "communication/modemManager.h"
#include "communication/secrets.h"
#include "communication/timeManager.h"

/*
    ThingSpeak Channel :
    Field 1 - distance_compensated 
    Field 2 - distance_raw
    Field 3 - temperature
    Field 4 - battery_voltage
    Field 5 - solar_voltage
    Field 6 - debug
    Field 7 - longitude
    Field 8 - latitude
    status  - debug
*/

/* 
    Funkcja budująca Body JSON dla wysyłanych dziennych danych.
    Surowe dane znacznika czasu zostają transformowane :
        - z bajtów danych w formacie UTC+0
        - do czasu poprawnego w formacie zimowym/letnim w lokalnej strefie czasowej i formacie ISO
    Raz w tygodniu zostają dodatkowo wysyłane dane GPS.
*/
String buildJson(const RTC_CyclicalMeasuremens* cyclicalBufor, uint16_t buforSize, const RTC_SingleMeasurements* singleBufor, 
    const RTC_TestStatus* statusBufor, uint16_t dayCounter , const RTC_problemWakeUpCodes* RTC_wakeUpCodes, 
    uint32_t dailySendFailsCount){

    JsonDocument doc;
    doc["write_api_key"] = writeAPIKey;
    JsonArray updates = doc["updates"].to<JsonArray>();

    for(int i = 0; i < buforSize; i++){    
        JsonObject updates_add = updates.add<JsonObject>();
        
        //odczyt czasu UTC+0 bytes i konwersja na czas lokalny ISO (osbłsuga czasu zimowego/letniego)  
        String created_at =  timeManager_bytesToISO(cyclicalBufor[i].created_at_bytes);

        updates_add["created_at"] = created_at;
        updates_add["field1"] = cyclicalBufor[i].compensatedDistance_rtc;
        updates_add["field2"] = cyclicalBufor[i].rawdDistance_rtc;
        updates_add["field3"] = cyclicalBufor[i].temperature_rtc;
        updates_add["field4"] = cyclicalBufor[i].batteryVoltage_rtc;
        updates_add["field5"] = cyclicalBufor[i].solarVoltage_rtc;           
        updates_add["field6"] = String(cyclicalBufor[i].debugWakeupReason);  //debug

        // zapis statusu bledow i GPS 
        if(i == (buforSize - 1)){ // ostatni zapis w buforze == najnowszy zapis w cyklu 'wysylki danych' gdzie wykonywany jest pomiar GPS oraz rejestr bledow modemu 
          
            // wysylka danych GPS tylko kiedy wykonano pomiar 
            if(!isnan(singleBufor->longitude_rtc) &&  !isnan(singleBufor->latitude_rtc)){
                updates_add["field7"] = singleBufor->longitude_rtc;
                updates_add["field8"] = singleBufor->latitude_rtc;
            }
            // debug status
            updates_add["status"] = 
                     "DayLoss:" + String(dailySendFailsCount)
                    + "|ModemFail:" + String(statusBufor->modemFail_count) 
                    + "|Connect:" + String(statusBufor->connectionFail_count)
                    + "|Send:" + String(statusBufor->sendFail_count)
                    + "|Retr:" + String(statusBufor->scndRetry_count)
                    + "|Day:" + String(dayCounter)
                    + "]WKP5:" + String(RTC_wakeUpCodes->code_5_count)
                    + "|WKP7:" + String(RTC_wakeUpCodes->code_7_count)
                    + "|WKP9:" + String(RTC_wakeUpCodes->code_9_count)
                    + "|WKP1:" + String(RTC_wakeUpCodes->code_1_count)
                    + "|WKPX:"+ String(RTC_wakeUpCodes->code_OTHER_count) 
                    + "|X:"+ String(RTC_wakeUpCodes->code_OTHER);
        }
    }

    String jsonBody;
    serializeJson(doc, jsonBody);
    if(isDebug){ 
     Serial.println(" ------------------- jsonBody ----------------------------------");
     Serial.println(jsonBody); 
     Serial.println("----------------------------------------------------------------");}
    return jsonBody;
}

/* Funkcja wysylki Body JSON z wykorzystaniem funkcji modemManager:modem_PostHttpsThingSpeak */
bool sendToThingSpeak(String JSON){
    return modem_PostHttpsThingSpeak(JSON, THINGSPEAK_CHANNEL_ID);
 }



















 