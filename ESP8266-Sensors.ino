#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DHTPIN  0
#define OWPIN   5
#define TEMPERATURE_PRECISION 11
#define MAX_SENSORS 8

bool debug = false;
//#define SerialDebug(text)   Serial.print(text);
//#define SerialDebugln(text) Serial.println(text);
#define SerialDebug(text)  
#define SerialDebugln(text)

char MyIp[16];
char MyHostname[16];
char MyRoom[32]="";
DeviceAddress SensorAddr[MAX_SENSORS];
char SensorID[MAX_SENSORS][32];
char SensorName[MAX_SENSORS][32];

WiFiClient espMqttClient;
PubSubClient mqttClient(espMqttClient);
DHT dht(DHTPIN, DHT22);
OneWire  oWire(OWPIN);
DallasTemperature sensors(&oWire);
int NumSensors;

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String s;
  
  if (strstr(topic, "/config/ESP-") && strstr(topic, "/roomname")) {
    if (length>=32) length=31;
    strncpy(MyRoom, (char *) payload, length);
    MyRoom[length]='\0';
    SerialDebug("New Room-Name: ");
    SerialDebugln(MyRoom);

    s = "/room/"; s+=MyRoom; s+="/"; s+=MyHostname;
    mqttClient.publish(s.c_str(), MyIp, true);
    s = "/room/"; s+=MyRoom; s+="/HR20/send";
    mqttClient.subscribe(s.c_str());
  } else if (strstr(topic, "/config/ID-") && strstr(topic, "/roomname")) {
    if (topic[13]=='-' && topic[24]=='/') {
      char id[17];
      strncpy(id, &(topic[8]), 16);
      id[16]='\0';
      for (int i=0 ; i<NumSensors ; i++) {
        if (strcmp(SensorID[i], id) == 0) {
          if (length>=32) length=31;
          strncpy(SensorName[i], (char *) payload, length);
          SensorName[i][length]='\0';
          SerialDebug("New Name for ");
          SerialDebug(id);
          SerialDebug(": ");
          SerialDebugln(SensorName[i]);
          break;
        }
      }
    }
  } else if (strstr(topic, "/HR20/send")) {
    for (int i=0 ; i<length ; i++) {
      Serial.write(payload[i]);
      delay(5);
    }
    Serial.write('\n');
  }
}

void mqtt_reconnect() {
  String s;
  
  while (!mqttClient.connected()) {
    s = "/config/"; s+=MyHostname; s+="/online";
    if (mqttClient.connect(MyHostname, s.c_str(), 0, true, "0")) {
      mqttClient.publish(s.c_str(), "1", true);
      
      s="/config/"; s+=MyHostname; s+="/ipaddr";
      mqttClient.publish(s.c_str(), MyIp, true);
      
      s="/config/"; s+=MyHostname; s+="/roomname";
      mqttClient.subscribe(s.c_str());

      s="/config/"; s+=MyHostname; s+="/HR20/send";
      mqttClient.subscribe(s.c_str());
    } else {
      delay(5000);
    }
  }
}

void Init1Wire(int sensor) {
  String s;
  s="/config/"; s+=SensorID[sensor]; s+="/device";
  mqttClient.publish(s.c_str(), MyHostname, true);
  s="/config/"; s+=SensorID[sensor]; s+="/roomname";
  mqttClient.subscribe(s.c_str());
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(200);
  delay(100);

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(debug);
  wifiManager.setTimeout(3*60);
  if(!wifiManager.autoConnect()) {
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  IPAddress MyIP=WiFi.localIP();
  snprintf(MyIp, 16, "%d.%d.%d.%d", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
  snprintf(MyHostname, 15, "ESP-%08x", ESP.getChipId());
  SerialDebug("ESP-Hostname: ");
  SerialDebugln(MyHostname);
  
  MDNS.begin(MyHostname);
  int n = MDNS.queryService("mqtt", "tcp");
  SerialDebugln("mDNS query done");
  if (n == 0) {
    SerialDebugln("no services found");
  }
  else {
    SerialDebug(n);
    SerialDebugln(" service(s) found");
    for (int i = 0; i < n; ++i) {
      // Print details for each service found
      SerialDebug(i + 1);
      SerialDebug(": ");
      SerialDebug(MDNS.hostname(i));
      SerialDebug(" (");
      SerialDebug(MDNS.IP(i));
      SerialDebug(":");
      SerialDebug(MDNS.port(i));
      SerialDebugln(")");
      mqttClient.setServer(MDNS.IP(i), MDNS.port(i));
    }
  }
  SerialDebugln("");
  
  mqttClient.setCallback(mqtt_callback);
  
  dht.begin();
  
  sensors.begin();
  NumSensors=sensors.getDeviceCount();
  if (NumSensors>MAX_SENSORS) NumSensors=MAX_SENSORS;
  SerialDebug("1-Wire Sensors: ");
  SerialDebugln(NumSensors);
  for (int i=0 ; i<NumSensors ; i++) {
    sensors.getAddress(SensorAddr[i], i);
    sensors.setResolution(SensorAddr[i], TEMPERATURE_PRECISION);
    snprintf(SensorID[i], 32, "ID-%02x-%02x%02x%02x%02x%02x", SensorAddr[i][0],
      SensorAddr[i][6],  SensorAddr[i][5],  SensorAddr[i][4],
      SensorAddr[i][3],  SensorAddr[i][2],  SensorAddr[i][1]);
    SerialDebug("Sensor ");
    SerialDebug(i);
    SerialDebug(" ID: ");
    SerialDebugln(SensorID[i]);
    SensorName[i][0]='\0';
  }
}

void ReadDHT() {
  String s;
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(h) || isnan(t)) {
    SerialDebugln("Failed to read from DHT sensor!");
    return;
  }
  SerialDebug("DHT22: t=");
  SerialDebug(t);
  SerialDebug(" - h=");
  SerialDebugln(h);

  s="/sensor/"; s+=MyHostname; s+="-DHT/temp";
  mqttClient.publish(s.c_str(), String(t,2).c_str());
  s="/sensor/"; s+=MyHostname; s+="-DHT/humidity";
  mqttClient.publish(s.c_str(), String(h,2).c_str());
  if (MyRoom[0] != '\0') {
    s="/room/"; s+=MyRoom; s+="/DHT/temp";
    mqttClient.publish(s.c_str(), String(t,2).c_str());
    s="/room/"; s+=MyRoom; s+="/DHT/humidity";
    mqttClient.publish(s.c_str(), String(h,2).c_str());
  }
}

void Read1Wire(int sensor) {
  String s;
  float t;
  if (sensor==0) sensors.requestTemperatures();
  t = sensors.getTempC(SensorAddr[sensor]);
  SerialDebug(SensorID[sensor]);
  SerialDebug("(name=");
  SerialDebug(SensorName[sensor]);
  SerialDebug("): t=");
  SerialDebugln(t);

  s="/sensor/"; s+=SensorID[sensor]; s+="/temp";
  mqttClient.publish(s.c_str(), String(t,2).c_str());
  if (SensorName[sensor][0] != '\0') {
    s="/room/"; s+=SensorName[sensor]; s+="/temp";
    mqttClient.publish(s.c_str(), String(t,2).c_str());      
  }
}

void ReadHR20() {
  Serial.write('D');
  delay(5);
  Serial.write('\n');
}

void ReadSerial() {
  String msg,s;
  if (Serial.available() == 0) return;
  
  msg = Serial.readStringUntil('\n');
  msg.remove(0, msg.lastIndexOf('y')+1);
  msg.trim();
  if (msg.length()==0) return;
  
  s="/sensor/"; s+=MyHostname; s+="-HR20/lastmsg";
  mqttClient.publish(s.c_str(), msg.c_str());
  if (MyRoom[0] != '\0') {
    s="/room/"; s+=MyRoom; s+="/HR20/lastmsg";
    mqttClient.publish(s.c_str(), msg.c_str());
    if (msg.startsWith("D: ")) {
      // D: dW DD.MM.YY TT:TT:TT A V: VV I: IIII S: SSSS B: BBBB Is: IsIs
      // D: d4 01.01.09 12:00:16 A V: 20 I: 2240 S: 1650 B: 3240 Is: 0000 E:04
      s="/room/"; s+=MyRoom; s+="/HR20/auto";
      mqttClient.publish(s.c_str(), msg.substring(24,25).c_str());
      s="/room/"; s+=MyRoom; s+="/HR20/valve";
      mqttClient.publish(s.c_str(), msg.substring(29,31).c_str());
      s="/room/"; s+=MyRoom; s+="/HR20/temp-actual";
      mqttClient.publish(s.c_str(), msg.substring(35,39).c_str());
      s="/room/"; s+=MyRoom; s+="/HR20/temp-set";
      mqttClient.publish(s.c_str(), msg.substring(43,47).c_str());
      s="/room/"; s+=MyRoom; s+="/HR20/battery";
      mqttClient.publish(s.c_str(), msg.substring(51,55).c_str());
    }
  }
}

static long lastMsg = 0;
static int state = 0;
static int sensor = 0;
void loop() {
  if (!mqttClient.connected()) {
    mqtt_reconnect();
    lastMsg=millis();
    sensor=0;
    state=9;
  }
  mqttClient.loop();

  long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg=now;
    if (state==0) state=1;
  }
  
  switch (state) {
    case 0: ReadSerial();
      break;
    case 1: ReadDHT();
            state=2;
      break;
    case 2: Read1Wire(sensor);
            sensor++;
            if (sensor>=NumSensors) {
              sensor=0;
              state=3;
            }
      break;
    case 3: ReadHR20();
            state=0;
      break;
    case 9: Init1Wire(sensor);
            sensor++;
            if (sensor>=NumSensors) {
              sensor=0;
              state=0;
            }
      break;
    default: state=0;
  }
}
