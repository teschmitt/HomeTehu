# HomeTehu Station v0.1 for the ESP8266

Temperature and humidity sensor firmware for the ESP8266 with some uncommon features. HomeTehu Station will ...

1. ... switch into deep sleep mode when not reading sensors
2. ... send sensor data to an arbitrary MQTT server
2. ... buffer sensor data when no network is available
3. ... configure itself through a `config.json` in flash memory

## Plans

There is clearly a lot to do here and this project is in its earliest phase. A few todos:

1. Write backend that ingests the MQTT messages
2. Find a better name for this software
3. Write a better README
4. More error-handling, better logging/debugging
