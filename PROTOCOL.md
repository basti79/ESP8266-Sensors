# Protocol

## Hostname
Each module generates a "hostname" starting with "ESP-" followed by the 8-digit HEX chip-id. This is used to identify a new unique Module in the MQTT-Tree.

## Default MQTT-Topics
* **/config/<hostname>/online:** 1 - if module in online, 0 - if not (generated through "will message")
* **/config/<hostname>/ipaddr:** IP address of the module
* **/config/<hostname>/roomname:** from here the modules reads its "room name", you whould publish this as retained message
* **/config/<hostname>/HR20/send:** all strings published here are send to the HR20 controller via serial line
* **/sensor/<hostname>-DHT/temp:** tempreture from DHT-22 sensor
* **/sensor/<hostname>-DHT/humidity:** humidity from DHT-22 sensor
* **/sensor/<hostname>-HR20>/lastmsg:** last message got from HR20 via serial line

For the 1-wire sensors those topics are published for each sensor with "sensor-id" being its unique 1-wire ID:
* **/config/<sensor-id>/device:** hostname of the ESP-Module to which the sensor is connected
* **/config/<sensor-id>/roomname:** room name for this sensors (may contain '/', so you can configure "livingroom/sensor1")
* **/sensor/<sensor-id>/temp:** tempreture value for each of the 1-wire sensors

The following topics are only published if the room name is set:
* **/room/<roomname>/<hostname>:** IP address of the module
* **/room/<roomname>/HR20/send:** all strings published here are send to the HR20 controller via serial line
* **/room/<roomname>/DHT/temp:** tempreture from DHT-22 sensor
* **/room/<roomname>/DHT/humidity:** humidity from DHT-22 sensor
* **/room/<sensor-roomname>/temp:** tempreture from single 1-wire sensor
* **/room/<roomname>/HR20>/valve:** valve posiotion, 0-100%
* **/room/<roomname>/HR20>/temp-actual:** messured tempreture, in °C/100
* **/room/<roomname>/HR20>/temp-set:** configured wanted temreture, in °C/100
* **/room/<roomname>/HR20>/battery:** battery voltage

