//  TODO OPIS funkcji

/*
  For full disclousure see file : 3RD_PARTY_LICENCES.txt
  This file sources :
  - NTPClient - under MIT License Copyright © taranais (https://github.com/taranais)
  - NorthernWidget/DS3231  - under Unlicense license
  - JChristensen/Timezone  - under GNU General Public License v3.0

do opisania : 
WiFi
WiFiUdp
Wire
*/

/*
  Obsługa modułu zegara czasu rzeczywsitego RTC DSDS3231 :
  - odczyt czasu konfiguracyjnego za pomocą WiFi NTP Client
      bazuje na bibliotece https://github.com/taranais/NTPClient.git autorstwa https://github.com/taranais
      kod został zaadaptowany na bazie przykladu https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/
  - zapis czasu konfiguracyjnego do DSDS3231 jako czas uniwersalny UCT+0
  - odczyt czasu DSDS3231 
      bazuje na blibliotece DSDS3231 https://github.com/NorthernWidget/DS3231.git 
  - obsługa zmiany czasu z zimowego na letni i konwersji na timestamp ISO 
      bazuje na bibliotece https://github.com/JChristensen/Timezone autorstwa https://github.com/JChristensen
*/
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Wire.h>
#include <DS3231.h>
#include <Timezone.h>
#include "memmory/rtcStruct.h"
#include <communication/timeManager.h>
#include "communication/secrets.h"
#include "debug.h"

// NTP Client - dla pobrania setUp time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
 
// DS3231
DS3231 myRTC;
bool century = false;
bool h12Flag = false;
bool pmFlag = false; 

/* <Timezone.h> struct TimeChangeRule
      char abbrev[6];    // five chars max
      int8_t week;       // First, Second, Third, Fourth, or Last week of the month
      int8_t dow;        // day of week, 1=Sun, 2=Mon, ... 7=Sat
      int8_t month;      // 1=Jan, 2=Feb, ... 12=Dec
      int8_t hour;       // 0-23
      int offset;        // offset from UTC in minutes
*/
// Definicja strefy czasowej dla Polski
TimeChangeRule plDST = {"CEST", Last, Sun, Mar, 2, 120}; // Czas letni: UTC+2
TimeChangeRule plSTD = {"CET", Last, Sun, Oct, 3, 60};   // Czas zimowy: UTC+1
Timezone polska(plDST, plSTD);

// pobranie czasu do setupt rtc
// GMT 0 - aby kontrola strefy, albo czasu zinowego letniego odbywala sie po stronie servera
// uwaga modul pracuje na UTC
void timeManager_getNtpTimeUTC(dateTime_bytes* NtpBuff){
  String formattedDate;
  String dayStamp;
  String timeStamp;
  String yearS_lastTwoDigits, monthS, dayS, hourS, minuteS, secondS, dOW;

  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    if(isDebug){ 
      Serial.println("WiFi connected.");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("Pobrana data NTP UTC+0: ");
    }

    //inicjalizacja NTPClient 
    timeClient.begin();
    // GMT podany w sekundach 0 rtc zwraca czas UTC+0
    timeClient.setTimeOffset(0);
    while(!timeClient.update()) {
      timeClient.forceUpdate();
    }

    formattedDate = timeClient.getFormattedDate(); //string daty w formacie 2004-02-12T15:19:21
    dOW = timeClient.getDay();
    if(isDebug){ Serial.println(formattedDate); }

    // wycięcie sekcji daty
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    int splitD1 = dayStamp.indexOf('-');
    int splitD2 = dayStamp.lastIndexOf('-');
    yearS_lastTwoDigits = dayStamp.substring(2, splitD1);
    monthS = dayStamp.substring(splitD1 + 1, splitD2);
    dayS = dayStamp.substring(splitD2 + 1);

    // wycięcie sekcji czasu
    timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
    int splitT1 = timeStamp.indexOf(':');
    int splitT2 = timeStamp.lastIndexOf(':');
    hourS = timeStamp.substring(0, splitT1);
    minuteS = timeStamp.substring(splitT1 + 1, splitT2);
    secondS = timeStamp.substring(splitT2 + 1);

    if(isDebug){
      Serial.println("Ekstrakt : ");
      Serial.println(yearS_lastTwoDigits);
      Serial.println(monthS);
      Serial.println(dayS);
      Serial.println(hourS);
      Serial.println(minuteS);
      Serial.println(secondS);
      Serial.println("Day of Week: "+dOW);
    }

    NtpBuff->year = (byte)yearS_lastTwoDigits.toInt();
    NtpBuff->month = (byte)monthS.toInt();
    NtpBuff->day = (byte)dayS.toInt();
    NtpBuff->hour = (byte)hourS.toInt();
    NtpBuff->minute = (byte)minuteS.toInt();
    NtpBuff->second = (byte)secondS.toInt();
    NtpBuff->dOW = (byte)dOW.toInt();
}


// ustawienie na podstawie ntpt
void timeManager_setUp_DS3231(){
    Wire.begin();
    struct dateTime_bytes setUpBuff;

    // funkcja obsługująca pobranie czasu przy pomocy Ntp 
    timeManager_getNtpTimeUTC(&setUpBuff);   

    myRTC.setClockMode(false); //24h
    myRTC.setYear(setUpBuff.year);
    myRTC.setMonth(setUpBuff.month);
    myRTC.setDate(setUpBuff.day);
    myRTC.setHour(setUpBuff.hour);
    myRTC.setMinute(setUpBuff.minute);
    myRTC.setSecond(setUpBuff.second);
    myRTC.setDoW(setUpBuff.dOW);

  if(isDebug){
      Serial.print("DS3231 time read : ");
      Serial.print(myRTC.getYear()+2000, DEC); // dla DS3231 ostatnie 2 cyfry roku
      Serial.print("-");
      Serial.print(myRTC.getMonth(century), DEC);
      Serial.print("-");
      Serial.print(myRTC.getDate(), DEC);
      Serial.print(" ");
      Serial.print(myRTC.getHour(h12Flag, pmFlag), DEC); //24-hr
      Serial.print(":");
      Serial.print(myRTC.getMinute(), DEC);
      Serial.print(":");
      Serial.println(myRTC.getSecond(), DEC);
      Serial.print(" Day of Week: ");
      Serial.print(myRTC.getDoW(), DEC);
    }
}


// odczyt czasu do struktury bajtowej
//RTC_CyclicalMeasuremens pomiar;
//getRTC_Time(&pomiar.created_at_bytes);
void timeManager_read_DS3231(dateTime_bytes *timeBuff){
    Wire.begin();
    
    timeBuff->year = myRTC.getYear();
    timeBuff->month = myRTC.getMonth(century);
    timeBuff->day = myRTC.getDate();
    timeBuff->hour = myRTC.getHour(h12Flag, pmFlag);
    timeBuff->minute = myRTC.getMinute();
    timeBuff->second = myRTC.getSecond();
    timeBuff->dOW = myRTC.getDoW();      
    if(isDebug){ 
      Serial.println("------------------- Czas UTC+0 --------------------------");
      Serial.print("DS3231 time YY-MM-DD HH:MM:SS : ");
      Serial.print(timeBuff->year, DEC); // dla DS3231 ostatnie 2 cyfry roku
      Serial.print("-");
      Serial.print(timeBuff->month, DEC);
      Serial.print("-");
      Serial.print(timeBuff->day, DEC);
      Serial.print(" ");
      Serial.print(timeBuff->hour , DEC); //24-hr
      Serial.print(":");
      Serial.print( timeBuff->minute, DEC);
      Serial.print(":");
      Serial.println(timeBuff->second , DEC);
      Serial.print(" Day of Week: ");
      Serial.println(timeBuff->dOW , DEC);    
      Serial.println("---------------------------------------------------------");
   }
}

// Konwersja struktury bajtowej czasu UTC+0 na String ISO (np. "2024-03-05T12:00:00+01:00")
// obsluga zmiany czasu 
String timeManager_bytesToISO(const dateTime_bytes &bytesBuff){
    int read_fullYear = (int)bytesBuff.year + 2000;
    int read_month = (int)bytesBuff.month;
    int read_day = (int)bytesBuff.day;
    int read_hour = (int)bytesBuff.hour;
    int read_minute = (int)bytesBuff.minute;
    int read_second = (int)bytesBuff.second;
    int read_dOW = (int)bytesBuff.dOW;

    // Dostsowanie strefy czasowej Polska i czasu letniego, zimowego
    tmElements_t tm;
    tm.Year = read_fullYear - 1970; // TimeLib liczy lata od 1970
    tm.Month = read_month;
    tm.Day = read_day;
    tm.Hour = read_hour;
    tm.Minute = read_minute;
    tm.Second = read_second;

    // Konwersja (UTC+0) -> Czas polski z obsługą zimowy/letni
    time_t utcTime = makeTime(tm);
    TimeChangeRule *tcr;                                //wskaźnik
    time_t localTime = polska.toLocal(utcTime, &tcr);   //zapis offset UTC in minutes
    
    // Format ISO dla czasu polskiego
    char buff_iso[30];   
    int offsetHour = tcr->offset / 60;
    int locYear   = year(localTime);
    int locMonth  = month(localTime);  
    int locDay    = day(localTime);     
    int locHour   = hour(localTime);
    int locMinute = minute(localTime);
    int locSecond = second(localTime);
 
    // Formatowanie ISO np: 2024-03-05T14:30:00+02:00
    sprintf(buff_iso, "%04d-%02d-%02dT%02d:%02d:%02d%+03d:00", locYear, locMonth, locDay, locHour, locMinute, locSecond, offsetHour);
    
    return String(buff_iso);
}


void rtcPinsSleep(){
  pinMode(SDA_PIN, INPUT);
  pinMode(SCL_PIN, INPUT);
  gpio_set_pull_mode((gpio_num_t)SDA_PIN, GPIO_FLOATING);
  gpio_set_pull_mode((gpio_num_t)SCL_PIN, GPIO_FLOATING);
}