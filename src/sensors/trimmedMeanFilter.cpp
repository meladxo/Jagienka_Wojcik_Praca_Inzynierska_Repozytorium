/*
    Obsługa filtracji próbek metodą "Trimmed Mean" - Średniej z obciętymi wartościami ekstremalnymi
    Filtracja przeznaczona dla serii próbek z czujnika odległości w celu obsłuzenia outlierów 
    przy pomiarze odległości od wody spowodowanych deszczem oraz wybrania wartości średniej wyniku w celu
    obsługi pomiaru falującej wody (odczytywania grzebietu fali).
*/
#include <Arduino.h>
#include <math.h>
#include "sensors/trimmedMeanFilter.h"

static void insertionSort(float* array, uint8_t n);

/*
    Funkcja zweacająca średnią obciątą na bazie tablicy próbek. 
    Usuwane jest 25% makasymalnych i minimalnych wartości, z pozostałego zbioru obliczona jest średnia arytmetyczna.
    W przypadku zbyt małego zbiory funkcja zwraca medianę.
    Parametrami są: wskaźnik do zbioru próbek, rozmiar zbioru, minimalny rozmiar zbioru dla obliczenia średniej,dla mniejszego zbioru obliczona zostanie mediana.
*/
float trimmedMean(float* samples, uint8_t dataSize , uint8_t meanLimit){
    float toTrimmSize_raw = dataSize / 4; 
    uint8_t toTrimmSize = round(toTrimmSize_raw);
    uint8_t trimmedSize = dataSize - 2*toTrimmSize;  
 
    if(samples == NULL || dataSize == 0){
        return -1;  
    }
    
    // kopia tablicy aby nie modyfikować/sortować oryginału
    float samplesCopy[dataSize] = {0};
    for(uint8_t i = 0; i < dataSize; i++){
        samplesCopy[i] = samples[i];
    }
    insertionSort(samplesCopy, dataSize);

    //dla zbyt małej liczby próbek zwracana jest mediana
    if(dataSize <= meanLimit){  
        if(dataSize % 2 == 1){
            return samplesCopy[dataSize / 2];
        }else{
            return (samplesCopy[dataSize / 2 - 1] + samplesCopy[dataSize / 2]) / 2;
        }
    }
    
    //obliczenie sumy z pominieciem pierwszych i ostatnich toTrimmSize elementów tablicy
    float sum = 0.0f;
    for(uint8_t i = toTrimmSize; i < dataSize-toTrimmSize; i++) {
        sum += samplesCopy[i];
    }
    return sum / trimmedSize;
}


/* Funkcja sortująca tablicę według algorytmu "sortowania przez wstawianie". */
static void insertionSort(float* array, uint8_t n){
    for (uint8_t i = 1; i < n; ++i) {
        float currentValue = array[i];
        int8_t j = i - 1;
        while(j >= 0 && array[j] > currentValue){
            array[j + 1] = array[j];
            j--;
        }
        array[j + 1] = currentValue;
    }
}