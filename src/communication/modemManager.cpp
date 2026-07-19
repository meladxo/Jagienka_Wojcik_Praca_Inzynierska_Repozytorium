/*    
    Obłsuga modemu SIMCOM A7670 :
    - sekwencji uruchomienia i wyłączenia modemu
    - nawiązania połączenia z modemem i rejestracji do sieci LTE
    - przesyłu danych przez HTTPS POST do serveru ThingSpeak 
    - odczytu współrzędnych geograficznych z wykorzystaniem Assisted GPS
    Zastosowano optymalizacje usypiania esp32, elimincaje nieskończonych pętli 
    i dobrane doświadczalne wartości TIMEOUT.

    Kod został zaadaptowany na bazie przykładów producenta płytki Lilygo : 
    - modemPowerOff
    - modemReset 
    - DeepSleep
    - GPS_Acceleration
    - HttpsBuildPost
    https://github.com/Xinyuan-LilyGO/LilyGo-Modem-Series/tree/main/examples    
        author    Lewis He (lewishe@outlook.com)
        license   MIT
        copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
*/

#include "communication/modemManager.h"
#include "lilygoDefines.h"
#include "power/espSleep.h"
#include "debug.h"
#include <TinyGsmClient.h>
#include <math.h>

TinyGsm modem(SerialAT);
String apn = NETWORK_APN;
bool use_ipv6_access_point = false;  // false->IPv4 lub true->IPv6 

// Kluczowe timeouty dla zostały konserwatywnie dobrane z uwagi na moliwe trudne warunki terenowe i coldstartu modemu raz dziennie
static const uint32_t MODEM_REG_TIMEOUT_MS = 120000;  //  Rejestracji do sieci LTE : 2min
static const uint32_t AGPS_TIMEOUT_MS = 120000;       //  Assisted GPS : 2min
static const uint32_t MODEM_REG_RETRY_WAIT = 5000;    //  Czas uśpienia pomiędzy odpytywaniem rejestracji LTE :5s   
static const uint32_t AGPS_RETRY_WAIT = 5000;         //  Czas uśpienia pomiędzy odpytywaniem fixu AGPS :5s  
static const uint32_t POST_APN_WAIT_MS = 3000;        //  stabilizacja po rejestracji z APN
static const uint32_t POST_IP_WAIT_MS = 3000;         //  stabilizacja po IP przed https
static const uint32_t POST_REG_WAIT_MS = 3000;        //  stabilizacja po rejestracji

// jednolity zwrot błędu
const int GPS_ERROR = -1; 
const float GPS_NULL = NAN;
static const bool MODEM_ERROR = false;  
static const bool MODEM_SUCCESS = true;          

/*
    Obsługa uruchomienia modemu :
    - sekwencje zmian stanów pinów kontroli modemu
    - reset modemu (dodany po obserwacji problemu z komunikacja AT)
    - testy komunikacji AT
    - awaryjny reset PWRKEY dla problemów AT
    - domyślne wyłączneie poszespołu GPS dla oszczedzania energii
    Funkcja zwraca status sukcesu/błędu włączania.
    
    *delay() - wymagany przy sekwencjach kontroli modemu oraz kliczowych stabilizacji
     lightSleepDelay() - w pozostałych miejscach
*/
bool modemPowerOn(){

    /*  Rozwiązanie znanego problemu płytki T-A7670 "DC Boost dla modemu"
        W przypadku zasilania bateryjnego, BOARD_POWERON_PIN (IO12) musi być ustawiony na poziom wysoki po uruchomieniu esp32. 
        w przeciwnym razie nastąpi reset.  
    */
    gpio_hold_dis((gpio_num_t)BOARD_POWERON_PIN);  // Zwolnienie BOARD_POWERON_PIN
    gpio_hold_dis((gpio_num_t)BOARD_PWRKEY_PIN);   // Zwolnienie pinu PWRKEY
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
    gpio_hold_en((gpio_num_t)BOARD_POWERON_PIN);
    delay(2000); // stabilizacja 

    gpio_hold_dis((gpio_num_t)MODEM_RESET_PIN);     // zwolnienie pinu RESET

    // Ustawianie pinu DTR w stan niski wyprowadzający modem z ewentualnego stanu modem sleep 
    pinMode(MODEM_DTR_PIN, OUTPUT);
    digitalWrite(MODEM_DTR_PIN, LOW);

    // Uruchomienie modemu przez PWRKEY
    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(100);                         
    digitalWrite(BOARD_PWRKEY_PIN, HIGH);
    delay(MODEM_POWERON_PULSE_WIDTH_MS);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    lightSleepDelay(11200);   // Czas na pełne uruchomienie wg dokumentacji A7670 Ton Status "A7670 Series_Hardware Design_V1.03" https://www.simcom.com/product/A7670X.html
    if(isDebug){Serial.println("Modem włączany  ...");}

    // Inicjalizacja komunikacji UART z MODEM
     SerialAT.begin(MODEM_BAUDRATE, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    
    // Test komunikacji z modemem (reset, reset awaryjny)
    uint32_t atStartTime = millis();
    bool ATsuccess = false;
    while(!ATsuccess){
        // Startowy reset
        if (isDebug) { Serial.println("Reset modemu"); }
        pinMode(MODEM_RESET_PIN, OUTPUT);
        digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL); 
        delay(100);
        digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);  
        delay(2600);
        digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
            //  Test komunikacji AT
            atStartTime = millis();
            while (millis() - atStartTime <= 10000) {
                if(modem.testAT(500)){ 
                    ATsuccess = true; 
                    break; 
                }
                if(isDebug){ Serial.println("..."); }
                lightSleepDelay(100);
            }
            if(ATsuccess){
                break;
            }

        // Awaryjny reset PWRKEY
        pinMode(BOARD_PWRKEY_PIN, OUTPUT);
        digitalWrite(BOARD_PWRKEY_PIN, LOW);
        delay(100);                         
        digitalWrite(BOARD_PWRKEY_PIN, HIGH);
        delay(MODEM_POWERON_PULSE_WIDTH_MS);
        digitalWrite(BOARD_PWRKEY_PIN, LOW);
        lightSleepDelay(11200);   // Czas na pełne uruchomienie wg dokumentacji A7670 Ton Status
            // Drugi test komunikacji AT
            atStartTime = millis();
            while (millis() - atStartTime <= 10000) {
                if(modem.testAT(500)){ 
                    ATsuccess = true; 
                    break; 
                }
                if(isDebug){ Serial.println("..."); }
                lightSleepDelay(100);
            }
            if(ATsuccess){
                break;
            }
        
        if (isDebug) { Serial.println("Modem nie odpowiada po resecie i pwrkey!"); }
        return MODEM_ERROR;
    }
    modem.disableGPS();
    if(isDebug){ Serial.println("GPS wyłączony"); } 
    return MODEM_SUCCESS;
}


/*
    Obsługa wyłączenia uruchomionego modemu, z zabezpieczeniem przed resetami modemu.
    Z uwagi na zawodne wyłączanie modemu funkcjami TinyGsmClient wprowadzono stałe wymuszenie stanu pinów w stan wyłączonego zasilania.
    Funkcja zwraca status sukcesu/błędu wyłączania.
*/
bool modemPowerOff(){
    if(isDebug){ Serial.println("Wyłączanie modemu ... "); }
    gpio_hold_dis((gpio_num_t)BOARD_POWERON_PIN);

    if(!modem.poweroff()){
        if(isDebug){ Serial.println("Wyłączanie modemu - Fail! Próba awaryjna przez PWRKEY..."); }
        pinMode(BOARD_PWRKEY_PIN, OUTPUT);
        digitalWrite(BOARD_PWRKEY_PIN, LOW);
        delay(100);
        digitalWrite(BOARD_PWRKEY_PIN, HIGH);
        delay(MODEM_POWEROFF_PULSE_WIDTH_MS);
        digitalWrite(BOARD_PWRKEY_PIN, LOW);
    }
    lightSleepDelay(2000); // Toff stats 2s

    // Weryfikacja wyłączenia modemu = brak 3 odpowiedzi na AT
    const uint32_t MODEM_POWEROFF_TIMEOUT_MS = 5000;
    const uint8_t atFails = 3;
    uint8_t noAtCount = 0;
    uint32_t startTimer = millis();
    while(millis() - startTimer < MODEM_POWEROFF_TIMEOUT_MS){
        if(isDebug){ Serial.println("..."); }
        if(!modem.testAT(500)){
            noAtCount++;
        }
        if(noAtCount >= atFails){
            break;
        }
        lightSleepDelay(100);
    }

    // Wymuszenie bezpiecznego stanu OFF modemu i utrzymanie stanu pinów przez sleep (po obserwacji bledow)
    // Zabezpieczenie przed przypadkowym niekotrolowanym wzbudzeniem, lub bledem wyłączenia
    // BOARD_POWERON_PIN - HIGH (wymagane dla DC boost na baterii)
    // MODEM_RESET_PIN - stan wymuszajacy OFF, BOARD_PWRKEY_PIN - LOW
    /*  Rozwiązanie znanego problemu płytki T-A7670, gdzie pin (5) jest uzywany jako reset. 
            Ustawienie pinu RESET w stan niski podczas uśpienia. 
            Po wybudzaniu ze stanu sleep występuje impuls, który spowoduje uruchomienie modułu.
            https://github.com/Xinyuan-LilyGO/LilyGO-T-A76XX/issues/85

    */ 
    pinMode(BOARD_POWERON_PIN, OUTPUT);  
    digitalWrite(BOARD_POWERON_PIN, HIGH);
    gpio_hold_en((gpio_num_t)BOARD_POWERON_PIN);
    pinMode(MODEM_RESET_PIN, OUTPUT); 
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
    gpio_hold_en((gpio_num_t)MODEM_RESET_PIN);
    pinMode(BOARD_PWRKEY_PIN, OUTPUT); 
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    gpio_hold_en((gpio_num_t)BOARD_PWRKEY_PIN);
  

    if(noAtCount < atFails){
        if(isDebug){ Serial.println("Power Off - Fail! Modem nadal odpowiadal na AT"); }        
         return MODEM_ERROR;
    }else{
        if(isDebug){ Serial.println("Wyłączanie modemu zakończone SUKCESEM!"); }
        return MODEM_SUCCESS;
    }
}


/*
    Funkcja wykonujaca awaryjny reset dla wlaczonego modemu, akcje:
    - reset modemu
    - awaryjny reset poprzez BOARD_PWRKEY_PIN
    - weryfikacji komunikacji AT 
*/
bool modemReset(){
    if(isDebug){ Serial.println("Resetowanie modemu..."); }
    // Zwolnienie pinow w stan domyslny 
    gpio_hold_dis((gpio_num_t)BOARD_POWERON_PIN);
    gpio_hold_dis((gpio_num_t)MODEM_RESET_PIN);
    gpio_hold_dis((gpio_num_t)BOARD_PWRKEY_PIN);

    // Zabezpieczenie BOARD_POWERON_PIN 
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
    gpio_hold_en((gpio_num_t)BOARD_POWERON_PIN);
    // Wybudzenie z ewentualnego stanu sleep
    pinMode(MODEM_DTR_PIN, OUTPUT);      
    digitalWrite(MODEM_DTR_PIN, LOW);

    SerialAT.begin(MODEM_BAUDRATE, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN); // komunikacja UART

    // Reset modemu (sekwencja resetu)
    pinMode(MODEM_RESET_PIN, OUTPUT);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
    delay(120);
    digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
    delay(3000);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

    lightSleepDelay(6000); // Delay przed rozpoczeciem testu AT
   
    uint32_t atStartTime = millis();
    while (millis() - atStartTime <= 15000){
        if(modem.testAT(500)){
            if (isDebug) { Serial.println("Modem odpowiada (AT OK)!"); }
            return MODEM_SUCCESS;
        }
        if(isDebug){ Serial.println("..."); }
        lightSleepDelay(500);
    }
   
    // Awaryjna sekwencja PWRKEY gdy brak odpowiedzi AT po resecie
    if(isDebug){ Serial.println("Modem nie odpowiadal na AT, próba PWRKEY..."); }
    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, HIGH);
    delay(MODEM_POWERON_PULSE_WIDTH_MS);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    lightSleepDelay(11200);

    atStartTime = millis();
    while (millis() - atStartTime <= 15000){
        if(modem.testAT(500)){
            if (isDebug) { Serial.println("Modem odpowiada (AT OK)! Po awaryjnym PWRKEY!"); }
            return MODEM_SUCCESS;
        }
        if(isDebug){ Serial.println("..."); }
        lightSleepDelay(500);
    }

    if (isDebug) { Serial.println("Modem nie odpowiada po resecie awaryjnym PWRKEY"); }
    return MODEM_ERROR;
}
 
 
/*
    Obsłua wetyfikacja połączenia i rejestracji do sieci LTE. 
    Weryfikacja uwzględnia optymalizację pod kątem : 
        - eliminacji pętli nieskończonych
        - usypiania płytki esp32 w trybie ligtht sleep
        - wprowadzono przerwy lightsleep dalay miedzy nastepnymi zadaniami połaczenia z intrastrukutrą operatora 
    Weryfikacja :
        - DEBUG - weryfikacja karty SIM 
        - ustawienie APN
        - weryfikacja rejestracji z siecią komórkową  
        - weryfikacja aktywacji połączenia sieciowego z APN  
        - weryfikacja adresu IP
    Funkcja zwraca status sukcesu/błędu wyłączania.
*/
bool modemTestConnection(){    

    // Weryfikacja SIM    
    uint32_t simStartTime = millis();
    const uint32_t SIM_TIMEOUT_MS = 15000;  
    SimStatus sim = SIM_ERROR;
    if(isDebug){ Serial.println("Weryfikacja SIM ... "); }
    while (sim != SIM_READY) {
        if(millis() - simStartTime > SIM_TIMEOUT_MS) {
            if(isDebug){ Serial.println("Timeout weryfikacji karty SIM! Przekroczono Timeout ~15s"); }
            return MODEM_ERROR;
        }
        sim = modem.getSimStatus();
        switch(sim){
        case SIM_READY:
            if(isDebug){ Serial.println("Karta SIM online"); }
            break;
        case SIM_LOCKED:
            if(isDebug){ Serial.println("Karta SIM zablokowana PIN!"); }
            return MODEM_ERROR;     
        default:
            if(isDebug){ Serial.printf("Status SIM: %d\n", sim); }
            lightSleepDelay(1000);
            break;
        }
    }

    // Ustawienie APN
    if (!modem.setNetworkAPN(NETWORK_APN)) {
        if(isDebug){ Serial.println("Błąd ustawienia APN!"); }
        return MODEM_ERROR;
    }if(isDebug){ Serial.println("Ustawiono APN"); }

    // Weryfikacja sygnału i rejestracji do sieci  
    int16_t sq;
    uint32_t regStartTime = millis();
    RegStatus status = REG_NO_RESULT;
    while (status == REG_NO_RESULT || status == REG_SEARCHING || status == REG_UNREGISTERED) {
        if(millis() - regStartTime > MODEM_REG_TIMEOUT_MS) {
            if(isDebug){Serial.print("Błąd rejestracji (brak zasięgu)! Timeout ");}
            return MODEM_ERROR;
        }
        status = modem.getRegistrationStatus();
        switch (status) {
        case REG_UNREGISTERED:
        case REG_SEARCHING:
            sq = modem.getSignalQuality();
            if(isDebug){ Serial.printf("...Signal Quality:%d\n", sq); }
            lightSleepDelay(MODEM_REG_RETRY_WAIT);
            break;
            
        case REG_DENIED:
            if(isDebug){ Serial.println("Network registration was rejected, please check if the APN is correct!"); }
            return MODEM_ERROR;
        case REG_OK_HOME:
            if(isDebug){ Serial.println("Online registration successful!"); }
            break;
        case REG_OK_ROAMING:
            if(isDebug){ Serial.println("Network registration successful, currently in roaming mode!"); }
            break;
        default:
            if(isDebug){ Serial.printf("Registration Status:%d\n", status); }
            lightSleepDelay(MODEM_REG_RETRY_WAIT);
            break;
        }
    }
    if(isDebug){ Serial.println("Modem połączył się do sieci!"); }
    lightSleepDelay(POST_REG_WAIT_MS); // stabilizacja

    // Aktywacja sieci z APN 
    const uint8_t MAX_NETWORK_RETRIES = 90; // negocjacja APN ~180s
    for(uint8_t retry = 1; retry <= MAX_NETWORK_RETRIES; retry++) {        
        if(modem.setNetworkActive(apn, use_ipv6_access_point)) {
            break; 
        }
        if(retry < MAX_NETWORK_RETRIES) {
            lightSleepDelay(2000);   
            if(isDebug){ Serial.println("APN retry, uśpienie 2s"); }
        } else {
            if(isDebug){ Serial.println("Nie udało się aktywować sieci z APN! Przekroczono timeout okna APN"); }
            return MODEM_ERROR;
        }

    }
    if(isDebug){ Serial.println("Aktywacja połączenia sieciowego z APN udana!"); }
    lightSleepDelay(POST_APN_WAIT_MS); // stabilizacja

    // Weryfikacja IP zawsze, niezależnie od trybu debug.
    String ipAddress = modem.getLocalIP();
    if(isDebug){ Serial.print("Network IP: "); Serial.println(ipAddress); }
    if(ipAddress.length() == 0 || ipAddress == "0.0.0.0") {
        if(isDebug){ Serial.println("Brak prawidłowego adresu IP!"); }
        return MODEM_ERROR;
    }
    lightSleepDelay(POST_IP_WAIT_MS); // stabilizacja

    if(isDebug){ Serial.println("Połączenie sieciowe gotowe!"); }
    return MODEM_SUCCESS;  
}




/*
    Funkcja odczytująca dane pozycyjne długości i szerokości geograficznej z GSP (Assisted GPS).
    Parmetrami są wskaźniki na .mienną, do których zostaną zapisane odczytane dane longiture i latitude.
    Funkcja zwraca błąd GPS_ERROR w przypadku niepowodzenia lub przekroczonego timeoutu AGPS_TIMEOUT_MS 2min.
    Zastosowano usypianie esp32 podczas sprawdzania czy fix został zakończony na AGPS_RETRY_WAIT 15sek .
*/ 
int modem_getPositionAGPS(float *latitude, float *longitude){  
    
    uint32_t gpsEnableStart = millis();
    const uint32_t GPS_ENABLE_TIMEOUT_MS = 10000;  
    while(!modem.enableGPS(MODEM_GPS_ENABLE_GPIO, MODEM_GPS_ENABLE_LEVEL)) {
        if(millis() - gpsEnableStart > GPS_ENABLE_TIMEOUT_MS) {
            if(isDebug){ Serial.println("Timeout włączania GPS! Timeout 10s"); }
            modem.disableGPS();
            if(isDebug){ Serial.println("GPS wyłączony"); }
            *latitude = GPS_ERROR;
            *longitude = GPS_ERROR;
            return GPS_ERROR;
        }
        lightSleepDelay(500);
    }
    if(isDebug){ Serial.println("GPS włączony"); }

    modem.setGPSBaud(115200);

    // AGPS (assisted GPS)
    if(isDebug){ Serial.print("GPS acceleration enable attempt: ");}
    if(!modem.enableAGPS()) {
        if(isDebug){ Serial.print(" failed !");}
        modem.disableGPS();
        *latitude = GPS_ERROR; 
        *longitude = GPS_ERROR;
        return GPS_ERROR;
    } else {
        if(isDebug){ Serial.print(" success !");}
    }

    float lat2      = 0;
    float lon2      = 0;
    float speed2    = 0;
    float alt2      = 0;
    int   vsat2     = 0;
    int   usat2     = 0;
    float accuracy2 = 0;

    int   year2     = 0;
    int   month2    = 0;
    int   day2      = 0;
    int   hour2     = 0;
    int   min2      = 0;
    int   sec2      = 0;
    uint8_t fixMode   = 0;
    uint32_t gpsStartTime = millis();
    while(true){

        // Sprawdzenie timeoutu
        if(millis() - gpsStartTime > AGPS_TIMEOUT_MS) {
            if(isDebug){ Serial.println("Przekroczono Timeout GPS fix!"); }
            modem.disableGPS();
            if(isDebug){ Serial.println("GPS wyłączony"); }
            *latitude = GPS_ERROR; 
            *longitude = GPS_ERROR;
            return GPS_ERROR;
        }
        
        if(modem.getGPS(&fixMode, &lat2, &lon2, &speed2, &alt2, &vsat2, &usat2, &accuracy2,
                        &year2, &month2, &day2, &hour2, &min2, &sec2)){
            if(isDebug){
                Serial.print("Latitude:"); Serial.print(lat2, 6); Serial.print("\tLongitude:"); Serial.println(lon2, 6);
                Serial.print("Speed:"); Serial.print(speed2); Serial.print("\tAltitude:"); Serial.println(alt2);
                Serial.print("Visible Satellites:"); Serial.print(vsat2);
                Serial.print("\tUsed Satellites:"); Serial.println(usat2);
                Serial.print("Accuracy:"); Serial.println(accuracy2);
                Serial.print("Year:"); Serial.print(year2);
                Serial.print("\tMonth:"); Serial.print(month2);
                Serial.print("\tDay:"); Serial.println(day2);
                Serial.print("Hour:"); Serial.print(hour2);
                Serial.print("\tMinute:"); Serial.print(min2);
                Serial.print("\tSecond:"); Serial.println(sec2);
            }
            break;
        }else{
            if(isDebug){Serial.println("Brak GPS fix, retry and lightsleep");}
            lightSleepDelay(AGPS_RETRY_WAIT);
        }
    }
    
    *latitude = lat2; 
    *longitude = lon2;  
    if(isDebug){ 
        Serial.println("---------------- Pomiar Gps -----------------"); 
        Serial.print(lat2,6);
        Serial.println(" (latitude); ");
        Serial.print(lon2,6);
        Serial.println(" (longitude); ");
        Serial.println("---------------- Pomiar Gps -----------------");     
    }    
    modem.disableGPS();
    if(isDebug){ Serial.println("GPS wyłączony"); }
    return 0;
} 



/*
  Funkcja wysyłające dane do kanału ThingSpeak metodą HTTP POST "Bulk-Write JSON Data" https://www.mathworks.com/help/thingspeak/bulkwritejsondata.html
  Uwaga! Oczekiwany jest uruchomionny i gotowy do komunikacji modem.
  Parametrami są :
    - jsonBody - wysyłana tablica obiektów JSON zawierająca dane
    - ThingSpeakChanelID - idendyfikator kanału thingSpeak
  Funkcja zwraca wartość true/false zaleznie od sukcesu/błędu wykonanych sekwencji.
*/
bool modem_PostHttpsThingSpeak(const String &jsonBody, String ThingSpeakChanelID  ){
    if(jsonBody.isEmpty()){
        return false;
    }
    // Inicjalizacja HTTPS
    String url = "https://api.thingspeak.com/channels/" + ThingSpeakChanelID + "/bulk_update.json";
    if(isDebug){
        Serial.print("Url = "); 
        Serial.println(url); 
    }

    // Czyszczenie starej sesji HTTPS (problemy przy retry wysylki)
    modem.https_end();
    bool httpsReady = false;
    for(uint8_t i = 1; i <= 3; i++){
        if(modem.https_begin()){
            httpsReady = true;
            lightSleepDelay(1000);
            break;
        }
        if(isDebug){ Serial.println("Błąd https_begin - retry... ");}
        modem.https_end();
        lightSleepDelay(1000);
    }if(!httpsReady){
        return false;
    }

    modem.https_set_timeout(180, 60, 60); // Wydłuzenie timeoutu : domyślne connection=120, recieve data=10, get response=20

    // wymuszenie TLS 1.2 (stabilniejszy handshake, eliminuje auto-negocjacje wersji TLS, 1.2 jest zalecena przed thingspeak)
    if(!modem.https_set_url(url.c_str(), TINYGSM_SSL_TLS1_2, true)) { 
        if(isDebug){ Serial.println("Błąd https_set_url tls 1.2"); }
        modem.https_end();
        return false;
    }

    // Wysyłka
    if(!modem.https_set_content_type("application/json")) { 
        if(isDebug){ Serial.println("Błąd https_set_content_type"); }
        modem.https_end();
        return false;
    }
    int httpCode = modem.https_post_json_format(jsonBody);
    if(isDebug) {
        Serial.print("JSON body bytes: ");
        Serial.println(jsonBody.length());
        Serial.print("HTTP Response Code: ");
        Serial.println(httpCode);
    }

    if(httpCode > 0){
        // HTTPS header 
        String header = modem.https_header();
        if(isDebug && header.length() > 0){ 
            Serial.print("HTTP Header : ");
            Serial.println(header);
        }
        // HTTPS response
        String body = modem.https_body();
        if (isDebug && body.length() > 0) {
            Serial.print("Response Body: ");
            Serial.println(body);
        }
    }

    modem.https_end();
    if(httpCode == 200 || httpCode == 202){
        return true;
    }else{
        if(isDebug){ Serial.println("Błąd https_end"); } 
        return false;
    }
}


/*
    Specjalne wymuszenie wyłaczenia modemu przez piny sterujące.
    Uźycie : 
        - włączeny stan modemu po wgraniu programu, 
        - zabezpieczenie podczas nieplanowanych resetów ESP32 włączony modem przy resecie
*/
void modemForceOffPins(){
    // Zwolnienie pinow w stan domyslny 
    gpio_hold_dis((gpio_num_t)BOARD_POWERON_PIN);
    gpio_hold_dis((gpio_num_t)MODEM_RESET_PIN);
    gpio_hold_dis((gpio_num_t)BOARD_PWRKEY_PIN);

    // na zasilaniu bateryjnym pin DC boost musi pozostac HIGH.
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
    gpio_hold_en((gpio_num_t)BOARD_POWERON_PIN);

    pinMode(MODEM_RESET_PIN, OUTPUT);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
    gpio_hold_en((gpio_num_t)MODEM_RESET_PIN);

    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    gpio_hold_en((gpio_num_t)BOARD_PWRKEY_PIN);

    pinMode(MODEM_DTR_PIN, OUTPUT);  
    digitalWrite(MODEM_DTR_PIN, LOW);
    if(isDebug){Serial.println("Wymuszone odciecie zasilania modemu");}
}