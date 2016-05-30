# ESP8266-Sensors

IoT sensor modules based on ESP8266 for my home automation project.

This little module can connect the following devices:
* DS18B20 1-wire temperature sensors
* DHT-22 combined temperature and humidity sensor
* HR20 controller with [OpenHR20 Firmware](https://sourceforge.net/projects/openhr20/)

It's based on the Arduino [Arduino core for ESP8266](https://github.com/esp8266/Arduino) and needs these libraries:
* [WiFiManager](https://github.com/tzapu/WiFiManager)
* [Arduino Client for MQTT](https://github.com/knolleary/pubsubclient)
* [Teensy OneWire Library](http://www.pjrc.com/teensy/td_libs_OneWire.html)
* [Arduino Library for Maxim Temperature Integrated Circuits](https://github.com/milesburton/Arduino-Temperature-Control-Library)

For more information and schematics please visit: https://www.basti79.de/mediawiki/index.php/1-Wire_Sensoren_mit_WLAN


**This is the master branch:**
* uses an ESP-12 Module
* will get an aditional interface for reading information from water meters soon
