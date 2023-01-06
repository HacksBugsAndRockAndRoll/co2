#include <Arduino.h>
#include "MHZ19.h"
#include <SoftwareSerial.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <secrets.h>
#include <HomeAssistant.h>


#define FORCE_SPAN 0                                       // < --- set to 1 as an absoloute final resort
#define RX_PIN D1
#define TX_PIN D2
#define BAUDRATE 9600 

#include <FastLED.h>
#define NUM_LEDS 24
#define DATA_PIN D7
#define LED_TYPE WS2812
#define BRIGHTNESS 60

//#define CO2DEBUG      //uncomment to include debug functions
#define CO2HIGH 800
#define CO2CRITICAL 2000

MHZ19 myMHZ19;
SoftwareSerial mySerial(RX_PIN, TX_PIN);                   // Uno example
unsigned long getDataTimer = 0;
//Rx - blau
//Tx - gruen
//Gnd - schwarz
//Vin - rot

unsigned long lastReadCo2 = 0;
int lastPPM = -1;
boolean lightOn = true;

const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASSWORD;
WiFiClient net;
PubSubClient client(net);

#define MSG_BUFFER_SIZE	(512)
char mqttmsg[MSG_BUFFER_SIZE];
char ppmValue [16];

void setupled();
void changestate(HSVHue hue);
void setColor();
int readCO2(int minWait);
void idleColor();
void changestate();
void kringel();
void setupco2();
void printco2info();
void connectWiFi();
void connectMqtt();
void setupWiFi();
void setupMqtt();
void setupHA();
void doPublish(const char *topic, DynamicJsonDocument doc, boolean retain);
void doPublish(const char *topic, const char *message, boolean retain);
void setupHAPPM();
void setupHATemp();
void setupHACal();
void setupHALight();
void callback(char *topic, byte *payload, unsigned int length);
void toggleLight();
void calibrate();

CRGBArray<NUM_LEDS> leds;
HSVHue currenthue;

void setup() {
  Serial.begin(9600);
  setupWiFi();
  setupMqtt();
  setupHA();
  
  setupled();
  setupco2();
  
}

void setupWiFi(){
  WiFi.begin(ssid, pass);
  connectWiFi();
}

void setupMqtt(){
  client.setServer(MQTT_BROKER_IP, 1883);
  client.setCallback(callback);
  client.setBufferSize(1024);
  connectMqtt();
  client.loop();
}

void setupHA(){
  setupHAPPM();
  setupHATemp();
  setupHACal();
  setupHALight();
}

JsonObject addDevice(DynamicJsonDocument& doc ){
  JsonObject device = doc.createNestedObject("dev");
  device["name"]=HA_NODE_ID;
  device["ids"]=HA_NODE_ID;
  device["mf"]="SoxTech";
  device["mdl"]="MH-Z19";
  return device;
}

void doPublish(const char* topic, DynamicJsonDocument doc, boolean retain){
  serializeJson(doc,mqttmsg);
  doPublish(topic,mqttmsg,retain);
}

void doPublish(const char* topic, const char* message, boolean retain){
  Serial.print("publishing to ");
  Serial.println(topic);
  Serial.println(message);
  client.publish(topic,message,retain);
}

void setupHAPPM(){
  DynamicJsonDocument doc(256);
  doc["name"]=HA_NODE_ID " " HA_OBJ_CO2;
  doc["stat_t"]=HA_STAT_PPM;
  doc["unit_of_meas"]="ppm";
  doc["uniq_id"]=HA_NODE_ID HA_OBJ_CO2;
  doc["dev_cla"]="carbon_dioxide";
  doc["stat_cla"]="measurement";
  addDevice(doc);
  doPublish(HA_DISC_PPM,doc,true);
}

void setupHATemp(){
  DynamicJsonDocument doc(256);
  doc["name"]=HA_NODE_ID " " HA_OBJ_TEMP;
  doc["stat_t"]=HA_STAT_TEMP;
  doc["unit_of_meas"]="°C";
  doc["uniq_id"]=HA_NODE_ID HA_OBJ_TEMP;
  doc["dev_cla"]="temperature";
  doc["stat_cla"]="measurement";
  addDevice(doc);
  doPublish(HA_DISC_TEMP,doc,true);
}

void setupHACal(){
  DynamicJsonDocument doc(256);
  doc["name"]=HA_NODE_ID " " HA_OBJ_BTN_CAL;
  doc["uniq_id"]=HA_NODE_ID HA_OBJ_BTN_CAL;
  doc["stat_t"]=HA_STAT_CAL;
  doc["cmd_t"]=HA_CMND_CAL;
  addDevice(doc);
  doPublish(HA_DISC_CAL,doc,true);
  doPublish(HA_STAT_CAL,"ON",false);
}

void setupHALight(){
  DynamicJsonDocument doc(256);
  doc["name"]=HA_NODE_ID " " HA_OBJ_SW_LIGHT;
  doc["uniq_id"]=HA_NODE_ID HA_OBJ_SW_LIGHT;
  doc["stat_t"]=HA_STAT_LIGHT;
  doc["cmd_t"]=HA_CMND_LIGHT;
  addDevice(doc);
  doPublish(HA_DISC_LIGHT,doc,true);
  doPublish(HA_STAT_LIGHT,"ON",false);
}

void setupled(){
  FastLED.addLeds<NEOPIXEL,DATA_PIN>(leds, NUM_LEDS); 
}

void setupco2()
{
  mySerial.begin(BAUDRATE);
  myMHZ19.begin(mySerial);
  myMHZ19.autoCalibration(true);
  printco2info();
}

void connectWiFi(){
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("NOT ");
  }
  Serial.println("connected!");
}

void connectMqtt(){
  Serial.print("\nMQTT connecting...");
  while (!client.connect(HA_NODE_ID, "", "")) {
    Serial.print(".");
    delay(1000);
  }
  delay(100);
  client.subscribe("co2/co2thomas/+/cmnd");
  if(!client.connected()){
    Serial.print("NOT ");
  }
  Serial.println("connected!");
}

void loop(){ 

  if (WiFi.status() != WL_CONNECTED){
    connectWiFi();
  }
  if (!client.connected()) {
    connectMqtt();
  }
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability
  int ppm = readCO2(10000);
  //Serial.printf("ppm: %d\n",ppm);
  if(ppm > 0){
    client.publish(HA_STAT_PPM,itoa(ppm,ppmValue,10));
    setColor();
    idleColor();
  }
}

void setColor(){
  //Serial.println("setColor");
  if (lastPPM < CO2HIGH){
    currenthue = HSVHue::HUE_GREEN;
    if(lastPPM < 0){
      currenthue = HSVHue::HUE_PINK; 
    }
  }else if(lastPPM > CO2CRITICAL){
    currenthue = HSVHue::HUE_RED;
  }else{
    currenthue = HSVHue::HUE_ORANGE;
  }
  changestate();
}

int readCO2(int minWait){
  int errcount = 0;
  int ppm = -1;
  if(millis() > lastReadCo2 + minWait){
    //Serial.println("readCo2");
    kringel();
    while(errcount < 3){
      ppm = myMHZ19.getCO2(false);
      if(ppm < 0){
        errcount++;
        delay(10);
        continue;
      }else{
        lastReadCo2 = millis();
        break;
      }
    }
    lastPPM = ppm;
  }
  return ppm;
}

void idleColor(){
  //Serial.println("idleColor");
  if(lightOn){
  for(int i = 0; i < NUM_LEDS;i++){
    leds[i] = CHSV(currenthue,255,255);
  }
  FastLED.delay(50);
  for(int b = 255; b > BRIGHTNESS;b--){
    for(int i = 0; i < NUM_LEDS;i++){
      leds[i] = CHSV(currenthue,255,b);
    }
    FastLED.delay(50);
  }
  }
}


void changestate(){
  changestate(currenthue);
}

void changestate(HSVHue hue){
  if(lightOn){
    FastLED.setBrightness(BRIGHTNESS);
    for(int l = 0; l<3; l++){
      for(int i = 0; i < NUM_LEDS/2; i++) {   
        leds.fadeToBlackBy(40);
        leds[i] = CHSV(hue,255,255);
        leds(NUM_LEDS/2,NUM_LEDS-1) = leds(NUM_LEDS/2 - 1 ,0);
        FastLED.delay(33);
      }
    }
  }
}

void kringel(){
  if(lightOn){
    HSVHue messung = HSVHue::HUE_AQUA;
    for(int i = 0; i < NUM_LEDS;i++){
      leds[i] = CHSV(messung,255,0);
    }  
    for(int i = 0;i<NUM_LEDS;i++){
      leds[i] = CHSV(messung,255,50);
      leds[(i+1)%NUM_LEDS]=CHSV(messung,255,100);
      leds[(i+2)%NUM_LEDS] = CHSV(messung,255,200);
      leds[(i+3)%NUM_LEDS] = CHSV(messung,255,255);
      for(int j = 0; j < NUM_LEDS; j++){
        if(j != i && j != (i+1)%NUM_LEDS && j != (i+2)%NUM_LEDS && j != (i+3)%NUM_LEDS){
          leds[j] = CHSV(messung,255,0);
        }
      }
      FastLED.delay(1000/NUM_LEDS);
    }
  }else{
    delay(1000);
  }
}

void printco2info(){
  char myVersion[4];          
  myMHZ19.getVersion(myVersion);

  Serial.print("\nFirmware Version: ");
  for(byte i = 0; i < 4; i++)
  {
    Serial.print(myVersion[i]);
    if(i == 1)
      Serial.print(".");    
  }
   Serial.println("");

   Serial.print("Range: ");
   Serial.println(myMHZ19.getRange());   
   Serial.print("Background CO2: ");
   Serial.println(myMHZ19.getBackgroundCO2());
   Serial.print("Temperature Cal: ");
   Serial.println(myMHZ19.getTempAdjustment());
   Serial.print("ABC Status: "); myMHZ19.getABC() ? Serial.println("ON") :  Serial.println("OFF");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  if(strcmp(topic,HA_CMND_CAL)==0){
    calibrate();
  }else if(strcmp(topic,HA_CMND_LIGHT)==0){
    toggleLight();
  }else{
    Serial.println("unknown");
  }
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void toggleLight(){
  lightOn = !lightOn;
  if(lightOn){
    setColor();
  }else{
    FastLED.clear(true);
  }
}

void calibrate(){
  Serial.println("calibrate now");
  myMHZ19.calibrate();
}
