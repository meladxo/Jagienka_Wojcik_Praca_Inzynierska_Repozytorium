# Opis 
Repozytorium zawierające program wykorzystany w pracy inzynierskiej **_"Terenowy rejestrator poziomu wody w rzekach okresowych z mozliwością odczytu zdalnego"_**.
Kod źródłowy jest udostępniony na licencji [GNU_GPL_LICENSE.md](GNU_GPL_LICENSE.md).

## Bilbioteki, licencje, źródła
Wykorzystane w projekcie biblioteki, oraz inne zewnętrzne źródła zaimplementowanego kodu są zostały w [3RD_PARTY_LICENCES.md](3RD_PARTY_LICENCES.md)

## Kod źródlowy
Plik konfiguracyjny projektu w PlatformIO :
* platformio.ini 

Pliki w katalogu src:
* src/
	* main.cpp
	* communication/
		* modemManager.cpp
		* thingSpeakManager.cpp
		* timeManager.cpp
	* memmory/
		* Memmory.cpp
	* power/
		* batteryMonitor.cpp
		* espSleep.cpp
		* mosfet.cpp
	* sensors/
		* distanceSensor.cpp
		* measurementManager.cpp
		* temperatureSensor.cpp
		* trimmedMeanFilter.cpp

Pliki w katalogu include:
* include/
	* debug.h
	* lilygoDefines.h
	* communication/
		* modemManager.h
		* secrets.h
		* thingSpeakManager.h
		* timeManager.h
	* memmory/
		* Memmory.h
		* rtcStruct.h
	* power/
		* batteryMonitor.h
		* espSleep.h
		* mosfet.h
	* sensors/
		* distanceSensor.h
		* measurementManager.h
		* temperatureSensor.h
		* trimmedMeanFilter.h
