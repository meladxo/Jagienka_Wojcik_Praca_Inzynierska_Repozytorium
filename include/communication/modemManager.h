#ifndef MODEM_MANAGER_H
#define MODEM_MANAGER_H

    #include <Arduino.h>

    #define TINY_GSM_RX_BUFFER  1024    // Ustawienie buffora RX na 1Kb
 
    #define NETWORK_APN  "iot"          // Konfiguracja operatora karty SIM "APN Orange IoT na kartę" https://www.orange.pl/view/iot-na-karte
   
    int extern const GPS_ERROR;
    float extern const GPS_NULL;

    bool modemPowerOff();
    void modemForceOffPins();
    bool modemTestConnection();
    bool modemPowerOn();  // Alternatywna wersja bez obowiązkowego resetu
    int modem_getPositionAGPS(float *latitude, float *longitude);
    bool modem_PostHttpsThingSpeak(const String &jsonBody, String ThingSpeakChanelID);
    bool modemReset();
#endif