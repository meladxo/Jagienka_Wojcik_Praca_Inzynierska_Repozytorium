/*
  Obłsuga odczytu odległości mierzonej czujnikiem ultradźwiękowym A02YYUW
  - obłsuga komunikacji UART i odczytu danych z czujnika
  - inicjalizacja i konfiguacja czujnika
  - algorytm kompensacji temperaturowej wartości mierzonych czujnikiem ultradźwiękowym

  Kod został zaadaptowany na bazie przykładu producenta czujnika DFRobot udostępnionym bez określonej licencji
  https://wiki.dfrobot.com/sen0311/docs/21651
  https://wiki.dfrobot.com/sen0311/docs/21654
*/
#include <Arduino.h>
#include "sensors/distanceSensor.h"
#include "driver/gpio.h"

// Zakres operacyjny czujnika A02YYUW
const uint16_t DISTNACE_MIN_RANGE_CM = 3; 
const uint16_t DISTNACE_MAX_RANGE_CM = 450; 
const float DISTANCE_ERROR = -1.0f;     
const float DISTANCE_RISK_TEMP_C = -15.0f;


static const uint16_t DISTANCE_READ_TIMEOUT_MS = 300;  // timeout 100ms/300ms dla wybranego trybu DISTANCE_SENSOR_RX_PIN
                                                       // TX Lilygo - RX czujnika (LOW=realtime 100ms, HIGH/float=stable 300ms)
static const float INTERNAL_SOUND_SPEED = 348.4f;

/*
  Inicjalizacja kounikacji UART czujnika : 9600 band rate, no parity, 1 stop bit, 8 data bits
  Ustawienie pinu DISTANCE_SENSOR_RX_PIN w trybie stable (timeout 300ms)
*/
void distanceSensor_SetUp(){

    pinMode(DISTANCE_SENSOR_RX_PIN, OUTPUT);      
    digitalWrite(DISTANCE_SENSOR_RX_PIN, HIGH); // HIGH/float=stable 300ms
    SerialDistanceSensor.begin(9600, SERIAL_8N1, DISTANCE_SENSOR_TX_PIN, DISTANCE_SENSOR_RX_PIN);
    
    while(SerialDistanceSensor.available()) {
      //czyszczenie bufora UART 
      SerialDistanceSensor.read();    
    }
}


/*
  Funkcja odczytu odległości nieblokująca procesora. Funkcja wykonuje procesy:
    - odczyt  nagłówka, DATA_H, DATA_L, sumy kontrolnej
    - weryfikacja nagłówka i walidacja sumy kontrolnej
    - przetworzenie wyniku jako sumy DATA_H i DATA_L
    - zarządanie limitem czasu odczekiwania na odpowiedź timeout DISTANCE_READ_TIMEOUT_MS 
  Parametrem jest wstaźnik do zmiennej przychowującej zapis pomiaru w centymetrach lub błąd DISTANCE_ERROR.
  Zwraca true dla zakończoneej próby oczytu nie zaleznie od sukcesu/błędu.
*/
bool distanceSensor_isReady(float* distance){
  static uint8_t data[4] = {0};    // pełna odpowiedź UART [Header, DATA_H, DATA_L, SUM]
  static uint8_t readBytesCount = 0;
  static unsigned long startTime = 0;
  static bool startTiming = false;
  static bool readHeader = false;

  //start odczytu i odliczania timeout pełnej odpowiedzi 
  if(readBytesCount == 0 && readHeader == false && startTiming == false){
    startTime = millis();
    startTiming = true;
  }

  //kontrola przekroczenia timeout
  if(startTiming == true && (millis() - startTime > DISTANCE_READ_TIMEOUT_MS)){
    *distance = DISTANCE_ERROR;  
    readBytesCount = 0;
    readHeader = false;
    startTiming = false;
    return true;
  }
  
  if(SerialDistanceSensor.available()){
    uint8_t byte = SerialDistanceSensor.read();
    
    //odcztyt nagłówka
    if(readHeader == false){
      if(byte == 0xFF){
        data[0] = byte;
        readBytesCount = 1;
        readHeader = true;
      }
    }else{

      //odczyt pozostałych bajtów
      data[readBytesCount] = byte;
      readBytesCount++;
      if(readBytesCount == 4){

        //weryrikacja sumy kontrolnej SUM=Header+Data_H+Data_L
        uint8_t checksum = data[0] + data[1] + data[2];    
        if(checksum == data[3]){

          //kalkulacja sumy = Data_H*256+ Data_L;  
          int32_t distanceMM = (data[1] << 8) | data[2]; 
          float distanceCM = distanceMM / 10.0f;
          *distance = distanceCM;
        }else{
          *distance = DISTANCE_ERROR;
        }
        
        //oczekiwanie na nową pełną odpowiedź
        readBytesCount = 0;
        readHeader = false;
        startTiming = false;
        return true;
      }
    }
  }
  return false; //odopwiedź niegotowa, kontynuacja 
}


/*
  Funkcja wykonująca kompensacje temperaturową dla zmierzone odległości, temperatury i wzoru v=331.6+0.6T[m/s]:.
  Czujnik nie posiada kompensacji wewnętrznej stałe wewnętrzne przyjęte przez producenta to:
  stała temperatury : 28C;
  stała prędnokści dźwięku : 348m/s
  Parametry to temperatura[C] oraz odległość[cm]  
  Zwrócona zostaje skompensowana odległość[cm] DISTANCE_ERROR
*/
float distanceSensor_tempCompensation(float tempC, float distanceCm){
  if(distanceCm == DISTANCE_ERROR){
    return DISTANCE_ERROR;
  }
  else{
    return (distanceCm * (331.6f+(0.6f*tempC)) / INTERNAL_SOUND_SPEED);
  }
}





/* 
  Funkcja przygotowania czujnika do odcięcia zasilania. Zabezpieczenie przed poborem pradu przez podłaczone piny.
*/ 
void distanceSensorSleep() {
    // zatrzymanie UART czujnika odległości
    SerialDistanceSensor.flush();
    SerialDistanceSensor.end();

    // pin czujnika ustawione w stan bezpieczny (wysokiej impedancji/float)
    pinMode(DISTANCE_SENSOR_RX_PIN, INPUT);
    gpio_set_pull_mode((gpio_num_t)DISTANCE_SENSOR_RX_PIN, GPIO_FLOATING);

    pinMode(DISTANCE_SENSOR_TX_PIN, INPUT);
    gpio_set_pull_mode((gpio_num_t)DISTANCE_SENSOR_TX_PIN, GPIO_FLOATING);

 
 }
