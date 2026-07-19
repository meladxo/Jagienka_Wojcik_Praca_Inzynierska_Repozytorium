/*
    Plik wykorzystuje bibliotekę Preferences (NVS, wear leveling)
    do trwałego backupu danych w przypadkach nietrwałości pamięci RTC.

    Funkcja FLASH_init - obsługuje inicjalizację przestrzeni, wyzerowanie danych i ustawienie wartości startowych.
    Zbiór funkcji FLASH_set_ - odpowiadają za ustawianie flag oraz przygotowanie stanu backupu
    Zbiór Funkcji FLASH_save_ - odpowiadają za trwały zapis liczników, kodów wybudzeń i struktur pomiarowych
    Zbiór funkcji FLASH_read_ - odpowiadają za odczyt flag, liczników i struktur pomiarowych z pamięci backupowej
*/
#include <Preferences.h> // Biblioteka z zaspisami NVS built-in wear leveling
#include "memmory/rtcStruct.h"
#include "memmory/Memmory.h"
#include "debug.h"
 
Preferences prefs; 

//  czyszczenie i inicjalizacja namespace 
void FLASH_init(){
    prefs.begin("backup_space", false);
    prefs.clear();
    prefs.putBool("backup_flg", false);
    prefs.putUShort("wakeupCounter", 0);
    prefs.putUShort("daysCounter", 1);
    prefs.putULong("dailyFailCnt", 0); // dla dailySendFailsCount nazwa zbyt długa 
    RTC_problemWakeUpCodes init = {0,0,0,0,0,0}; 
    prefs.putBytes("wakeUpCodes_v2", &init, sizeof(RTC_problemWakeUpCodes));
    prefs.end();
    if(isDebug){ Serial.println("FLASH: INIT backup_space - czyszczenie i inicjalizacja"); }
}
 

void FLASH_setFlgBckp(){
    prefs.begin("backup_space", false);
    prefs.putBool("backup_flg", true); 
    prefs.end();  
    if(isDebug){ Serial.println("FLASH: Wykonano backup BackupFlg = true");}
}

bool FLASH_readFlgBckp(){
    bool ret;
    prefs.begin("backup_space", true); // otwórz przestrzeń w trybie tylko do odczytu
    ret = prefs.getBool("backup_flg");
    prefs.end();  
    if(isDebug){ 
        Serial.print("FLASH: Staus flagi BackupFlg = ");
        Serial.println(ret ? "true" : "false ");
    }
    return ret;
}

void FLASH_clearFlgBckp(){
    prefs.begin("backup_space", false);
    prefs.putBool("backup_flg", false);
    prefs.end();  
    if(isDebug){ Serial.println("FLASH: CLEAR (BackupFlg = false) i przygotowanie dla nastepnego dnia"); }
}

 
void FLASH_saveBackupAll(const RTC_CyclicalMeasuremens* cyclicalBufor, uint16_t cyclicalCount, uint16_t wakeupCounter, uint16_t daysCounter) {
    prefs.begin("backup_space", false); // otwórz przestrzeń "backup_space" w trybie zapisu/odczytu
    size_t cyclicalBytes = (size_t)cyclicalCount * sizeof(RTC_CyclicalMeasuremens);

    // zapis liczników i struktury/tablice bezpośrednio jako bloki bajtów BLOB
    prefs.putUShort("wakeupCounter", wakeupCounter);
    prefs.putUShort("daysCounter", daysCounter);
    prefs.putBytes("cyclical", cyclicalBufor, cyclicalBytes);

    prefs.end(); // Zamknij i bezpiecznie zapisz na Flash
    if(isDebug){ Serial.println("FLASH : SAVE Backup z RTC danych pomiarowych i licznikow");}
}
 
void FLASH_readBackupAll(RTC_CyclicalMeasuremens* cyclicalBufor, uint16_t cyclicalCount, uint16_t* wakeupCounter,
    uint16_t* daysCounter, RTC_problemWakeUpCodes* RTC_wakeUpCodes, uint32_t* dailySendFailsCount ){

    prefs.begin("backup_space", true);
    size_t cyclicalBytes = (size_t)cyclicalCount * sizeof(RTC_CyclicalMeasuremens);
    size_t wakeUpCodesBytes = sizeof(RTC_problemWakeUpCodes);

    *wakeupCounter = prefs.getUShort("wakeupCounter", 0);
    *daysCounter = prefs.getUShort("daysCounter", 0);
    
    prefs.getBytes("cyclical", cyclicalBufor, cyclicalBytes);
    
    memset(RTC_wakeUpCodes, 0, wakeUpCodesBytes);
    prefs.getBytes("wakeUpCodes_v2", RTC_wakeUpCodes, wakeUpCodesBytes);
    *dailySendFailsCount = prefs.getULong("dailySendFailsCount", 0);
    
    prefs.end();
    if(isDebug){ Serial.println("FLASH : READ Backup dane pomiarowe i liczniki");}
}

void FLASH_readBackupDayCount(uint16_t* daysCounter) {
    prefs.begin("backup_space", true); // otwórz przestrzeń w trybie tylko do odczytu
    *daysCounter = prefs.getUShort("daysCounter", 0);
    prefs.end();
    if(isDebug){ 
        Serial.print("FLASH : READ Backup licznik daysCounter: ");
        Serial.println(*daysCounter);
    }
}

void FLASH_readBackupWakeUpCount(uint16_t* wakeupCounter) {
    prefs.begin("backup_space", true); // otwórz przestrzeń w trybie tylko do odczytu
    *wakeupCounter = prefs.getUShort("wakeupCounter", 0);
    prefs.end();
    if(isDebug){ 
        Serial.print("FLASH : READ Backup licznik wakeupCounter: ");
        Serial.println(*wakeupCounter);
    }
}


void FLASH_saveBackUpWakeUpCodes(const RTC_problemWakeUpCodes* RTC_wakeUpCodes) {
    prefs.begin("backup_space", false);  
    size_t wakeUpBytes = sizeof(RTC_problemWakeUpCodes);
    prefs.putBytes("wakeUpCodes_v2", RTC_wakeUpCodes, wakeUpBytes);
    prefs.end();
}


void FLASH_readBackupWakeUpCodes(RTC_problemWakeUpCodes* RTC_wakeUpCodes) {
    prefs.begin("backup_space", true);  
    size_t wakeUpBytes = sizeof(RTC_problemWakeUpCodes);
    memset(RTC_wakeUpCodes, 0, wakeUpBytes);
    prefs.getBytes("wakeUpCodes_v2", RTC_wakeUpCodes, wakeUpBytes);
    prefs.end();
}

void FLASH_saveDailySendFailCount(uint32_t dailySendFailsCount) {
    prefs.begin("backup_space", false);
    prefs.putULong("dailyFailCnt", dailySendFailsCount);     
    prefs.end();
}

void FLASH_readDailySendFailCount(uint32_t* dailySendFailsCount) {
    prefs.begin("backup_space", true);
    *dailySendFailsCount = prefs.getULong("dailyFailCnt", 0);
    prefs.end();
}