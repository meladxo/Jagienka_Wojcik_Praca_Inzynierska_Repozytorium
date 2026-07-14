/*
    Zbiór makr dla modelu T-A7670 zaadaptowany na podstawie przykładu udostępnionego producenta LilyGo
    https://github.com/Xinyuan-LilyGO/LilyGo-Modem-Series/blob/main/examples/ModemSleep/utilities.h
        author    Lewis He (lewishe@outlook.com)
        license   MIT
        copyright Copyright (c) 2025  Shenzhen Xin Yuan Electronic Technology Co., Ltd
*/
#ifndef LILYGO_DEFINES_H
#define LILYGO_DEFINES_H 

    #define LILYGO_T_A7670
    #define PRODUCT_MODEL_NAME                  "LilyGo-A7670 ESP32 Version"

    #define MODEM_BAUDRATE                      (115200)
    #define MODEM_DTR_PIN                       (25)    //pin kontroluje stan sleep i wakeup modemu, pull-up(AT+CSCLK=1) usypia modem, pull-down(AT+CSCLK=0) wybudza
    #define MODEM_TX_PIN                        (26)
    #define MODEM_RX_PIN                        (27)

    // pin boot modemu musi przejsc przed odpowiednią sekwencje startup  
    #define BOARD_PWRKEY_PIN                    (4)

    // przy zasilaniu bateryjnym pin musi zostać uawiony w stan wysoki po starcie esp32 w przeciwnym wypadku zajdzie zjawisko resetu
    #define BOARD_POWERON_PIN                   (12)

    #define MODEM_RING_PIN                      (33) 
    #define MODEM_RESET_PIN                     (5)
    #define BOARD_MISO_PIN                      (2)
    #define BOARD_MOSI_PIN                      (15)
    #define BOARD_SCK_PIN                       (14)
    #define BOARD_SD_CS_PIN                     (13)
    #define BOARD_BAT_ADC_PIN                   (35)
    #define MODEM_RESET_LEVEL                   HIGH
    #define SerialAT                            Serial1


    #define MODEM_GPS_ENABLE_GPIO               (-1)
    #define MODEM_GPS_ENABLE_LEVEL              (-1)

    #ifndef TINY_GSM_MODEM_A7670
        #define TINY_GSM_MODEM_A7670
    #endif

    #define BOARD_SOLAR_ADC_PIN                 (36)

    #define MODEM_REG_SMS_ONLY

    // sekwencja włączenia/wyłączenia mocy modemu 
    #define MODEM_POWERON_PULSE_WIDTH_MS      (100)
    #define MODEM_POWEROFF_PULSE_WIDTH_MS     (3000)

    #ifndef MODEM_START_WAIT_MS
        #define MODEM_START_WAIT_MS           (3000)
    #endif

#endif