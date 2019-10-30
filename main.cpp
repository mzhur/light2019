#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#define INVERCE_SENSOR

const char* ssid = "unclemax";
const char* password = "dbrbyu060978";
const char* mqtt_server = "192.168.0.1";
const char* DevName = "Hall_Light";
const int IDX_LIGHT = 17;
const int IDX_SETTINGS = 6;
const int LIGHT_PIN = 13;
const int SENSOR_PIN = 4;
const int WiFi_Reconnect_Interval = 300;
const int DEFAULT_LIGHT_ON_DELAY = 0;
const int DEFAULT_TARGET_LEVEL = 50;
const int DEFAULT_LOWLEVEL = 0;
const int DEFAULT_OFF_DELAY = 120000;
const int DEFAULT_CHANGE_SPEED = 100;


WiFiClient WiFi_client;
PubSubClient Mqtt_client(WiFi_client);
Ticker WiFi_update_connection;
Ticker Brightness_change;

int Light_Level;
struct SettingsStr {
  unsigned int light_on_delay;   
  unsigned int target_level;     
  unsigned int low_level;       
  unsigned int light_off_delay; 
  unsigned int change_speed;   
};


class MotionSensor {
public:
boolean state; // current sensor state
boolean light; // current light state depend on the delays, need to be update over update()  
boolean sended;
byte target;
unsigned long motion_time;
unsigned long no_motion_time;
void update(){
boolean temp;
temp = getState();  
}
bool getState(){
if ((light == 0) && (((state == 1) && ((millis() - motion_time) >= _settings.light_on_delay)) || (((millis() - no_motion_time) < _settings.light_off_delay) && (state == 0)))) {
light = true;
target = _settings.target_level;
}
else if ((light == 1) && (((millis() - no_motion_time) > _settings.light_off_delay) && (state == 0))) {
light = false;
target = _settings.target_level;
}
return light;
}
void update_settings(){
_settings = Get_Setiings(IDX_SETTINGS);  
}
private:
int _idx;
byte _pin;
SettingsStr _settings; 
};


MotionSensor motion_detector;


void WiFi_Connection_check(){
if (WiFi.status() == WL_DISCONNECTED) {  //If wifi disable - enable wifi connectinon
WiFi.begin(ssid, password);
}
else if (WiFi.status() != WL_CONNECTED) {
WiFi.mode(WIFI_OFF); // If wifi don`t connected before, try to disable module for 5 min
}
else if ((!Mqtt_client.connected()) && (WiFi.status() == WL_CONNECTED)){
if (Mqtt_client.connect(DevName)) {
      Mqtt_client.subscribe("domoticz/out");
      Mqtt_client.subscribe("domoticz/in");
}
}
}

void Motion()
{
#ifdef INVERCE_SENSOR    

if (motion_detector.state != !digitalRead(SENSOR_PIN)){
  motion_detector.state = !digitalRead(SENSOR_PIN)
  motion_detector.updated = false;
}

if (motion_detector.state == false)  {
motion_detector.motion_time = millis();
}
else {
motion_detector.no_motion_time = millis();  
}
#else
motion_detector.state = digitalRead(SENSOR_PIN);
if (motion_detector.state == true)  {
motion_detector.motion_time = millis();
}
else {
motion_detector.no_motion_time = millis();  
}
#endif
}


void Light_Set() {
if (Light_Level < int(motion_detector.target*10.2)) {
Light_Level ++;
analogWrite(LIGHT_PIN, int((0.1020)*pow(Light_Level,2)));
}
else if (Light_Level > int(motion_detector.target*10.2)) {
Light_Level --;   
analogWrite(LIGHT_PIN, int((0.1020)*pow(Light_Level,2)));
}
}

unsigned int Get_Level(unsigned int default_level, unsigned int rid){
HTTPClient http;
int httpCode;
unsigned int result = default_level;
String st_load;
String st_get = "http://192.168.0.1:8080/json.htm?type=devices&rid=" + String(rid) ;
const size_t capacity = JSON_OBJECT_SIZE(5) + 80; // Поправить размеры в соответсвии с возвращаемым ответом
DynamicJsonDocument json_answer(capacity);
if (WiFi.status() == WL_CONNECTED) {
           http.begin(st_get);
           httpCode = http.GET();
           if (httpCode == HTTP_CODE_OK) {
                st_load = http.getString();
                deserializeJson(json_answer, st_load);
                result =  json_answer["target_level"];  //Поправить значение поля дял возврата
           }
}
return result;
}


SettingsStr Get_Setiings(unsigned int rid){
HTTPClient http;
int httpCode;
String st_load;
SettingsStr result;
result.light_on_delay = DEFAULT_LIGHT_ON_DELAY;
result.target_level = DEFAULT_TARGET_LEVEL;
result.low_level = DEFAULT_LOWLEVEL;
result.light_off_delay = DEFAULT_OFF_DELAY;
result.change_speed = DEFAULT_CHANGE_SPEED;
String st_get = "http://192.168.0.1:8080/json.htm?type=command&param=getuservariable&idx=" + String(rid);  
const size_t capacity = JSON_OBJECT_SIZE(5) + 80;
DynamicJsonDocument json_answer(capacity);
if (WiFi.status() == WL_CONNECTED) {
           http.begin(st_get);
           httpCode = http.GET();
           if (httpCode == HTTP_CODE_OK) {
                st_load = http.getString();
                deserializeJson(json_answer, st_load); 
                result.light_on_delay = json_answer["light_on_delay"];
                result.target_level = Get_Level(json_answer["target_level"], IDX_LIGHT);
                result.low_level = json_answer["low_level"];
                result.light_off_delay = json_answer["light_off_delay"];
                result.change_speed = json_answer["change_speed"];
            }
} 

return result;

}


void setup() {
Mqtt_client.setServer(mqtt_server, 1883);
if (WiFi.getAutoConnect() != true)    //configuration will be saved into SDK flash area
{
  WiFi.setAutoConnect(true);   //on power-on automatically connects to last used hwAP
  WiFi.setAutoReconnect(true);    //automatically reconnects to hwAP in case it's disconnected
}
WiFi_update_connection.attach(WiFi_Reconnect_Interval, WiFi_Connection_check);

}

void loop() {
  // put your main code here, to run repeatedly:
}