#include <Arduino.h>
#include "debug.h"
#include "lilygoDefines.h"
#include "sensors/temperatureSensor.h"
#include "sensors/distanceSensor.h"
#include "sensors/measurementManager.h"
#include <power/mosfet.h>
#include "power/espSleep.h"
#include "power/batteryMonitor.h"
#include "communication/modemManager.h"
#include "communication/thingSpeakManager.h"
#include "communication/timeManager.h"
#include "memmory/rtcStruct.h"
#include <TinyGsmClient.h>
#include <esp_system.h>
#include <memmory/Memmory.h> 

// Organizacja zadań inicjalizacyjnych
const bool init_flg = false;
const bool rtcSetUp_flg = false;     // flaga ustawiana ręcznie przy konfiguracji zegara
const bool flashSetUp_flg = false;   // flaga ustawiana ręcznie przy konfiguracji Flash
 
// Harmonogram cyklów wybudzania i wykonywania pomiarów
RTC_NOINIT_ATTR uint16_t wakeUpCounter = 0;   // Licznik dziennych wybudzen
RTC_NOINIT_ATTR uint16_t daysCounter = 1;     // Licznik dni tygodnia w pamieci 
const uint16_t FullCycleWakeups = 24;         //  Liczba cyklów pomiarowych i numer ostatniego cyklu wysyłki danych 
const uint32_t timeToSleepSec = 36000;         // 3600 sekundy czasu uśpienia płytki między cyklami co godzin 
const bool sendGps_flg = true;
const uint16_t sendGpsDay = 7;                // 7 Dzien tygodnia pomiaru gps
void hourlyTaskList(uint16_t index);          // Funkcja pomocnicza organizująca zadania wykonywane co-godzinę

// Organizacja struktur dla zapisu danych pomiarowych do rejestrów pamięcie RTC podczas uśpienia 
RTC_NOINIT_ATTR RTC_CyclicalMeasuremens RTC_buforCyclical[FullCycleWakeups];  // tablica struktur pomiarów wykonywanych cyklicznie: przy kazdym wybudzeniu 
RTC_NOINIT_ATTR RTC_SingleMeasurements RTC_buforSingle;                       // struktua dla pomiarów GPS wykonywanych raz dziennie   

// Organizacja pomiaru odległości i liczby zbieranych próbek 
const uint8_t DISTANCE_SAMPLE_COUNT = 16;               // liczba próbek z której wyznaczona zostanie wartość średniej obciętej
float distance_samples[DISTANCE_SAMPLE_COUNT] = {0};    // tablica próbek zebranych przez czujnik odległości
const uint8_t meanLimit = 8;                            // dla zebrania mniej ponizej 8 probek, wyznaczana jest alternatywnie mediana 


// Organizacja struktur i zmiennych dla rejestracji błędów i statusu pracy
RTC_NOINIT_ATTR uint32_t rtcNoInitMagic = 0;                              // Kontrolna zmienna magic value - dla wykrycia korupcji pamieci RTC
RTC_NOINIT_ATTR uint32_t dailySendFailsCount = 0;                         // Licznik totalnego błędu wysyłki danych  - stan kumulacyjny
RTC_NOINIT_ATTR RTC_TestStatus RTC_StatusBufor = {0,0,0,0,0,0,0};         // Licznik błędów pracy modemu - stan aktualny
RTC_NOINIT_ATTR RTC_problemWakeUpCodes RTC_wakeUpCodes = {0,0,0,0,0,0};   // Licznik błędów resetu - stan kumulacyjny


void setup(){
  if(isDebug){  
    Serial.begin(115200);  
    delay(5000);    
  }
  /********* ZADANIA INICJALIZACYJNE ******************************************************************************************************************************/ 
  if(init_flg){
    
    modemForceOffPins();  // zabezpieczające odciecie zasilania modemu 
    if(rtcSetUp_flg){
      timeManager_setUp_DS3231();    
    }
    if(flashSetUp_flg){
      FLASH_init();       
    }
  }else{

  /********* OBSLUGA WYBUDZEŃ I NIEPOZADANYCH RESETÓW ******************************************************************************************************************************/ 
  modemForceOffPins();  // zabezpieczające odciecie zasilania modemu 
  deepSleepWakeUp();
  uint8_t wakeUpCode = deepSleepGetResetReason();
  
  // Moment wykrycia problematycznego resetu innego niz DeepSleep
    if( wakeUpCode != 8 ){  
      RTC_wakeUpCodes.code_OTHER = 0;                 // czyszczenie dla aktualnego zapisu
      FLASH_readBackupWakeUpCodes(&RTC_wakeUpCodes);
      if(wakeUpCode == 1){
        RTC_wakeUpCodes.code_1_count++;
      }else if(wakeUpCode == 9){
        RTC_wakeUpCodes.code_9_count++;
      }else if(wakeUpCode == 5){
        RTC_wakeUpCodes.code_5_count++;
      }else if(wakeUpCode == 7){
        RTC_wakeUpCodes.code_7_count++;
      }else{
        RTC_wakeUpCodes.code_OTHER_count++;          // zapis innych nieprzewidzianych kodow 
        RTC_wakeUpCodes.code_OTHER = wakeUpCode;
      }

      // Kontrola awaryjnych resetow max 5 (stop dla nieskończonych resetów i degradacji pamięci flash)
      RTC_wakeUpCodes.failLoop_count++;
      if(RTC_wakeUpCodes.failLoop_count >= 6 ){
        RTC_wakeUpCodes.failLoop_count = 0;
        FLASH_saveBackUpWakeUpCodes(&RTC_wakeUpCodes);

          // Zapis błędu
          FLASH_readDailySendFailCount(&dailySendFailsCount);  
          dailySendFailsCount++;
          FLASH_saveDailySendFailCount(dailySendFailsCount); // TODO dodaj to kalkulacji flash - tylko to 

          // Wymuszone zakonczenie dnia
          wakeUpCounter = 0;
          FLASH_readBackupDayCount(&daysCounter);
          if(daysCounter >= sendGpsDay){
            daysCounter = 1;
          }else{
            daysCounter++;
          }
          memset(&RTC_buforSingle, 0, sizeof(RTC_buforSingle));
          memset(RTC_buforCyclical, 0, sizeof(RTC_buforCyclical));
          modemForceOffPins(); // awaryjne odciecie zasilania modemu
          FLASH_clearFlgBckp();
          goIntoDeepSleep(timeToSleepSec);
      }else{
        FLASH_saveBackUpWakeUpCodes(&RTC_wakeUpCodes);
      }
   }
  /********* OBSŁUGA LICZNIKÓW i PAMIECI  ***************************************************************************************************************************************/
  bool BackUpFlg = FLASH_readFlgBckp(); // Czy istnieje backup FLASH wskazujący na uruchomienie po resecie przy cyklu wysyłki danych

  // Weryfikacja pamięci RTC noinit  (baza dla korzystania z samego RTC)
  const uint32_t RTC_NOINIT_ATTR_MAGIC = 0x12345679;       // magic value w pamieci rtc noinit dla porównania i wykrycia resetu https://interrupt.memfault.com/blog/noinit-memory
  if(rtcNoInitMagic != RTC_NOINIT_ATTR_MAGIC){             // weryfikacja czy nie nastapil nieprzewidziany reset i korupcja danych w pamieci RTC noinit
    rtcNoInitMagic = RTC_NOINIT_ATTR_MAGIC;
    memset(&RTC_StatusBufor, 0, sizeof(RTC_StatusBufor));

    // nie wykonano backupu i nastąpił reset, degradacja RTC którego nie przewidziano i nie obsługuję
    if(BackUpFlg == false){
     wakeUpCounter = 0;
     daysCounter = 1;
     memset(&RTC_buforSingle, 0, sizeof(RTC_buforSingle));
     memset(RTC_buforCyclical, 0, sizeof(RTC_buforCyclical));
     memset(&RTC_wakeUpCodes, 0, sizeof(RTC_wakeUpCodes));
    }
  }
   
  // Odtworzenie liczników z FLASH jesli wykryto istniejący backup, czyli uruchomienie po resecie  
  if(BackUpFlg == true){ 
    FLASH_readBackupWakeUpCount(&wakeUpCounter);
    if(wakeUpCounter < FullCycleWakeups){
      wakeUpCounter = FullCycleWakeups; //zabezpieczenie licznika
    }
    FLASH_readBackupDayCount(&daysCounter);
    if(isDebug){ Serial.println("FLASH : Wakup z resetu i przywrocenie licznikow z backUp");}
  }

  // Licznik nie inkrementowany - gdy praca przy awaryjnym resecie
  if(BackUpFlg == false){
    wakeUpCounter++;
  }

  // zabezpieczenie bufora i danych RTC przez ograniczenie indeksu (0 do FullCycleWakeups - 1)
  wakeUpCounter = constrain(wakeUpCounter, 1, FullCycleWakeups);
  uint16_t index = wakeUpCounter - 1; 
  // zabezpieczenie licznika dni
  if(daysCounter > sendGpsDay || daysCounter == 0){
    daysCounter = 1;
  }
  /*******************************************************************************************************************************************/
  if(isDebug){ 
        Serial.println();
        Serial.println("******************* Esp32 WakeUp ******************************************************************");
        Serial.print("WakeUpCounter : ");
        Serial.println(wakeUpCounter);
        Serial.print("DaysCounter : ");
        Serial.println(daysCounter);
  }
  
  // Cykliczne pomiarary poziomu wodu i innych warości        
    if(wakeUpCounter < FullCycleWakeups){
        if(isDebug){Serial.println("******************* Sekwencja cyklicznego pomiaru *************************************************");}
          RTC_buforCyclical[index].debugWakeupReason = deepSleepGetResetReason();  // debug, monitoring błędów
          hourlyTaskList(index);               // Cogodzinna stała lista zadań
          goIntoDeepSleep(timeToSleepSec);     // Uśpienie esp32
    }

  /*************************************************************************************************************************************************************/
    // Ostatni cykl dziennego wybudzenia i wysyłka danych 
    if(wakeUpCounter >= FullCycleWakeups){
        if(isDebug){ Serial.println("******************* Sekwencja wysyłki danych ******************************************************");}
          hourlyTaskList(index);
 
        // backup do flash przed praca modemu
        if(BackUpFlg == false){
          // zapis statusu moze bez sensu
          FLASH_saveBackupAll(RTC_buforCyclical, FullCycleWakeups, wakeUpCounter, daysCounter);
          FLASH_setFlgBckp();
        }
          
        // 5 prób (2xConnect 2xSend)
        // *kazda ponowna proba rozpoczeta resetem i reset miedzy probami conncect
        // Na 4 próbie pełne wyłączenie włączenie modemu
        bool modemReady = false;
        bool sendReady = false;  
        for(uint8_t retry = 0; (retry < 5) && (sendReady == false); retry++){
          if(retry == 0){
            modemPowerOn();
          }else if(retry == 3){
            modemPowerOff();
            modemPowerOn();
          }
      
          if(retry > 0){ 
            modemReset();  // reset po nieudanej probie
            RTC_StatusBufor.modemFail_count++;
            RTC_StatusBufor.scndRetry_count++;
          }

          modemReady = false;  
          if(modemTestConnection()){
            modemReady = true;
          }else{
            RTC_StatusBufor.modemFail_count++;
            RTC_StatusBufor.connectionFail_count++;
            if(!modemReset()){
              RTC_StatusBufor.modemFail_count++;
            }else if(!modemTestConnection()){
              RTC_StatusBufor.modemFail_count++;
              RTC_StatusBufor.connectionFail_count++;
            }else{
              modemReady = true;
            }
          }
          if(modemReady){
            // GPS tylko przy pierwszej probie - dane sie nie zmienily
              FLASH_readBackupDayCount(&daysCounter);
              if(daysCounter >= sendGpsDay && sendGps_flg){
                modem_getPositionAGPS(&RTC_buforSingle.latitude_rtc, &RTC_buforSingle.longitude_rtc);
              }else{
                RTC_buforSingle.latitude_rtc = GPS_NULL;
                RTC_buforSingle.longitude_rtc = GPS_NULL;
              }

            // zapis aktualnego statusu do flash i odczyt wszystkiego przed budowa JSON
            // FLASH_saveStatusBufor(&RTC_StatusBufor); //podtrzeba zapisu do flash przed odczytem z flash
            FLASH_readBackupAll(RTC_buforCyclical, FullCycleWakeups, &wakeUpCounter, &daysCounter, &RTC_wakeUpCodes, &dailySendFailsCount);
            RTC_buforCyclical[index].debugWakeupReason = deepSleepGetResetReason();  // debug i kontrola

            String JsonBody = buildJson(RTC_buforCyclical, FullCycleWakeups, &RTC_buforSingle, &RTC_StatusBufor, daysCounter, &RTC_wakeUpCodes, dailySendFailsCount);

            if(!sendToThingSpeak(JsonBody)){
              RTC_StatusBufor.modemFail_count++;
              RTC_StatusBufor.sendFail_count++;
              lightSleepDelay(3000);
              if(!sendToThingSpeak(JsonBody)){
                RTC_StatusBufor.modemFail_count++;
                RTC_StatusBufor.sendFail_count++;
              }else{
                sendReady = true;
              }
            }else{
              sendReady = true;
            }
          }
        }
        

        // Resetowanie licznika dziennego, obsługa licznika tygodniwego, czyszczenie buforu dla nowego dnia
        wakeUpCounter = 0;
        FLASH_readBackupDayCount(&daysCounter);
        if(daysCounter >= sendGpsDay){
            daysCounter = 1;
          }else{
            daysCounter++;
        }      
        memset(&RTC_buforSingle, 0, sizeof(RTC_buforSingle));
        memset(RTC_buforCyclical, 0, sizeof(RTC_buforCyclical));

        // Zapis totalnej porazki wysylki danychi utrady danych z calego dnia
        if(!sendReady){          
          FLASH_readDailySendFailCount(&dailySendFailsCount);
          dailySendFailsCount++;
          FLASH_saveDailySendFailCount(dailySendFailsCount);
        }

        modemPowerOff();                    // Wyłączenie modemu
        FLASH_clearFlgBckp();               // Zaknończenie dnia, przygotowanie do zapisu flash następnego dnia
        goIntoDeepSleep(timeToSleepSec);    // Uśpienie esp32
    }
  }
}
void loop(){}


/*************************************************************************************************************************************************************/


void hourlyTaskList(uint16_t index){
  // Odczyt aktualnego czasu modułu zegarowego RTC             
  timeManager_read_DS3231(&RTC_buforCyclical[index].created_at_bytes);

  // Włączenie zasilania dla czujników odległości i temperatury
  mosfetON();
  lightSleepDelay(600); //wymagana stabilizacja czujników pod włączeniu zasilania 

  // Odczyt pomiarów wykonanych przez czujników odległości i temperatury 
  distanceSensor_SetUp(); 
  tempSensorSetup();
  sensorMeasurements(&RTC_buforCyclical[index].temperature_rtc,
    &RTC_buforCyclical[index].rawdDistance_rtc,
    &RTC_buforCyclical[index].compensatedDistance_rtc,
                      distance_samples, DISTANCE_SAMPLE_COUNT , meanLimit);
  
  // Odcięcia zasilania dla  czujników odległości i temperatury
  distanceSensorSleep();
  tempSensorSleep();
  rtcPinsSleep();
  mosfetOFF();

  // Odczyt napięcia baterii i napięcia wyjściowego panelu słonecznego
  batteryMonitor_readVoltage(&RTC_buforCyclical[index].batteryVoltage_rtc,
  &RTC_buforCyclical[index].solarVoltage_rtc);
}