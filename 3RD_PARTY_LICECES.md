## Biblioteki 

W projekcie wykorzystano ponizsze biblioteki:

| Biblioteka | Autor / źródło | Licencja | Link |
|---|---|---|---|
| OneWire | Paul Stoffregen | MIT | https://github.com/PaulStoffregen/OneWire |
| DallasTemperature | Miles Burton | MIT | https://github.com/milesburton/Arduino-Temperature-Control-Library |
| ArduinoJson | Benoit Blanchon | MIT | https://github.com/bblanchon/ArduinoJson |
| NTPClient | taranais / fork biblioteki NTPClient | MIT | https://github.com/taranais/NTPClient |
| TinyGSM-fork | lewisxhe / fork TinyGSM | GNU LGPL v3.0 | https://github.com/lewisxhe/TinyGSM-fork |
| Timezone | Jack Christensen | GNU GPL v3.0 | https://github.com/JChristensen/Timezone |
| DS3231* | NorthernWidget / A. Wickert, E. Ayars, J. C. Wippler, N. W. LLC | Public domain / Unlicense | https://github.com/NorthernWidget/DS3231 |

#### *Uwaga o bibliotece DS3231

Biblioteka DS3231 bazuje na bibliotece Ayars' oraz Jeelabs/Ladyada's autorów: A. Wickert, E. Ayars, J. C. Wippler, N. W. LLC, S. T. Andersen, SimGas, Per1234 oraz Glownt. 

#### Inne wykorzystane biblioteki w ramach Arduino Core dla ESP32 lub ESP-IDF
- Preferences
- Wire
- WiFi
- WiFiUdp
- driver/gpio


## Kod źródlowy

W projekcie wytworzony kod źródłowy w niektórych funkcjach bazowały na przykładach udostępnionych
w źródłach zewnętrznych lub udostępnionyh przez autorów powyszych bibliotek. Źrodła zewn. z których zaczerpnięto sposób implementacji wymieniono ponizej (szczegoly pozostaja w komentarzach plików źródłowych).

| Zrodlo | Rodzaj | Link 
|---|---|---|
| LilyGO-Modem-Series | repozytorium producenta plytki rozwojowej | https://github.com/Xinyuan-LilyGO/LilyGo-Modem-Series 
| Interrupt Memfault noinit memory | blog | https://interrupt.memfault.com/blog/noinit-memory 
| Random Nerd Tutorials - ESP32 NTP client | blog | https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/ 
| DFRobot SEN0311 documentation | dokumentacja producenta czujnika | https://wiki.dfrobot.com/sen0311/#docs
