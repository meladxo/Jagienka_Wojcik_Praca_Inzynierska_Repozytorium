/*
    zarzadzanie akcjami pomiarowycmi w projeckei
    - pomiar temp
    - warunkowy pomiar dist
    - kompensacja dist
    - filtracja danych pomiarowyc 
*/
#include <Arduino.h>
#include "sensors/measurementManager.h"
#include "sensors/temperatureSensor.h"
#include "sensors/distanceSensor.h"
#include "sensors/trimmedMeanFilter.h"
#include "debug.h"

/*
		distanceCompensated wskaźnik na zmienną skompensowanej odległości cm lub DISTANCE_ERROR
		distance_samples - tablica buforująca próbki 
		sampleCount  - liczba próbek do zebrania (DISTANCE_SAMPLE_COUNT)
*/
void sensorMeasurements(float *temperature, float *distanceRaw, float *distanceCompensated, float *distance_samples, uint8_t sampleCount , uint8_t meanLimit){
	bool tempConverstionStart = false;
	bool tempReady = false;
	bool distReady = false;
	float distanceSampleCm = 0.0f;
	float filterDistanceCm = 0.0f;
	uint8_t samples = 0;
	uint8_t validSamples = 0;
	uint8_t debug_samples = 0;

	// pomiar temperatury
	while(tempReady == false){
		if(tempConverstionStart == false){
			requestTemp();
			tempConverstionStart = true;
		}
		if(isTempReady() == true){
			*temperature = readTemp();
			tempReady = true;
		}
	}

	// kontrola czy temp. jest bezpieczna dla pracy czujnika odległości (ignorowanie błędu czujnika temp -127C)
	if(*temperature <= DISTANCE_RISK_TEMP_C && *temperature != ERROR_TEMP_C ){
		*distanceRaw = DISTANCE_ERROR;
		*distanceCompensated = DISTANCE_ERROR;
		if(isDebug){ Serial.println("Temperatura nie jest bezpieczna dla uruchomienia czujnika odległości !");}

	}else{

		// pomiar odległości
		while(distReady == false){
			if(distanceSensor_isReady(&distanceSampleCm)){
				samples++;

				//odrzucenie znanych błędów z serii próbek przed obliczeniem 'średniej obciętej'
				if(distanceSampleCm != DISTANCE_ERROR && (distanceSampleCm >= DISTNACE_MIN_RANGE_CM && distanceSampleCm <= DISTNACE_MAX_RANGE_CM)){
					distance_samples[validSamples] = distanceSampleCm;
					validSamples++;
				}

				//filtracja serii próbek obliczając 'średnią obciętą' trimmedMean
				if(samples >= sampleCount && validSamples > 0){
					filterDistanceCm = trimmedMean(distance_samples, validSamples,  meanLimit);
					distReady = true;
					debug_samples = validSamples;
				}
				//zwracanie błędu gdy wszystkie próbki są błędne
				else if(samples >= sampleCount && validSamples == 0){
					filterDistanceCm = DISTANCE_ERROR;
					distReady = true;
					debug_samples = 0;
				}
			}
		}
		//kompensacja temperaturowa zmierzonej odległości
		*distanceCompensated = distanceSensor_tempCompensation(*temperature, filterDistanceCm);
		*distanceRaw = filterDistanceCm;
	}

	if(isDebug){
		Serial.println("------------------- Pomiary czujników -------------------------------------------------------------");
		Serial.print(*temperature);
		Serial.print("C (temp); ");
		Serial.print(*distanceRaw);
		Serial.print("cm (rawDistance); ");
		Serial.print(*distanceCompensated);
		Serial.print("cm (compensatedDistance); ");
		Serial.print(debug_samples);
		Serial.print(" valid filtered samples; ");
		Serial.println();
		Serial.println("---------------------------------------------------------------------------------------------------");
	}
}