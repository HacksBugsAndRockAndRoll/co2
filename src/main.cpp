#include <Arduino.h>
#include "MHZ19.h"
#include <SoftwareSerial.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <secrets.h>
#include <ha.h>


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
#define CO2AVG 10
#define PAUSE 30000

MHZ19 myMHZ19;
SoftwareSerial mySerial(RX_PIN, TX_PIN);                   // Uno example
unsigned long getDataTimer = 0;
//Rx - blau
//Tx - gruen
//Gnd - schwarz
//Vin - rot


const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASSWORD;
WiFiClient net;
PubSubClient client(net);

#define MSG_BUFFER_SIZE	(512)
char mqttmsg[MSG_BUFFER_SIZE];
char ppmValue [16];


void setupled();
void changestate(HSVHue hue);
void changestate();
void breathe(unsigned long duration, int delay);
void pattern(HSVHue hue, int iterations, long del);
void kringel();
void pause(unsigned long wait);

void setupco2();
void printco2info();
int readavgco2(void (*delayfun)());

int co2lamp();


void connect();
void connectWiFi();
void connectMqtt();
void setupWiFi();
void setupMqtt();
void setupHA();
void setupHAPPM();
void setupHATemp();
void setupHACal();
void callback(char* topic, byte* payload, unsigned int length);

//debug functions
#ifdef CO2DEBUG
void ledtest();
void ledeffecttest();
void co2test();
void calibrateco2();
#endif

CRGBArray<NUM_LEDS> leds;
HSVHue currenthue;

void setup() {
  Serial.begin(9600);
  setupWiFi();
  setupMqtt();
  setupHA();
  #ifndef TESTING
  setupled();
  setupco2();
  #endif
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

void connect(){
  connectWiFi();
  connectMqtt();
}

void connectWiFi(){
    Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
}

void connectMqtt(){
  Serial.print("\nMQTT connecting...");
  while (!client.connect(HA_NODE_ID, "", "")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
}

void setupWiFi(){
  WiFi.begin(ssid, pass);
  connectWiFi();
}

void setupMqtt(){
  client.setServer(MQTT_BROKER_IP, 1883);
  client.setCallback(callback);
  connectMqtt();
}

void setupHA(){
  setupHAPPM();
  setupHATemp();
}


void setupHAPPM(){
  DynamicJsonDocument doc(256);
  doc["name"]=HA_NODE_ID " " HA_OBJ_CO2;
  doc["stat_t"]=HA_STAT_PPM;
  doc["unit_of_meas"]="ppm";
  doc["uniq_id"]=HA_NODE_ID HA_OBJ_CO2;
  doc["dev_cla"]="carbon_dioxide";
  doc["stat_cla"]="measurement";
  JsonObject device = doc.createNestedObject("dev");
  device["name"]=HA_NODE_ID;
  device["ids"]=HA_NODE_ID;
  device["mf"]="SoxTech";
  device["mdl"]="MH-Z19";
  Serial.printf("publishing discovery info to %s\n",HA_DISC_PPM);
  serializeJson(doc,mqttmsg);
  Serial.print("publishing: ");
  Serial.println(mqttmsg);
  client.publish(HA_DISC_PPM,mqttmsg,true);
}


void setupHATemp(){
  DynamicJsonDocument doc(256);
  doc["name"]=HA_NODE_ID " " HA_OBJ_TEMP;
  doc["stat_t"]=HA_STAT_TEMP;
  doc["unit_of_meas"]="°C";
  doc["uniq_id"]=HA_NODE_ID HA_OBJ_TEMP;
  doc["dev_cla"]="temperature";
  doc["stat_cla"]="measurement";
  JsonObject device = doc.createNestedObject("dev");
  device["name"]=HA_NODE_ID;
  device["ids"]=HA_NODE_ID;
  device["mf"]="SoxTech";
  device["mdl"]="MH-Z19";
  Serial.printf("publishing discovery info to %s\n",HA_DISC_TEMP);
  serializeJson(doc,mqttmsg);
  Serial.print("publishing: ");
  Serial.println(mqttmsg);
  client.publish(HA_DISC_TEMP,mqttmsg,true);
}

void setupHACal(){
  DynamicJsonDocument doc(256);
  doc["name"]=HA_NODE_ID " " HA_OBJ_BTN_CAL;
  doc["uniq_id"]=HA_NODE_ID HA_OBJ_BTN_CAL;
  doc["stat_t"]=HA_STAT_CAL;
  doc["cmd_t"]=HA_CMND_CAL;
  JsonObject device = doc.createNestedObject("dev");
  device["name"]=HA_NODE_ID;
  device["ids"]=HA_NODE_ID;
  device["mf"]="SoxTech";
  device["mdl"]="MH-Z19";
  Serial.printf("publishing discovery info to %s\n",HA_DISC_CAL);
  serializeJson(doc,mqttmsg);
  Serial.print("publishing: ");
  Serial.println(mqttmsg);
  client.subscribe(HA_CMND_CAL);
  client.publish(HA_DISC_CAL,mqttmsg,true);
  client.publish(HA_STAT_CAL,"ON");
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

  #ifndef TESTING
  client.publish(HA_STAT_PPM,itoa(co2lamp(),ppmValue,10));
  pause(PAUSE);
  #endif
  #ifdef TESTING
  client.publish(HA_STAT_PPM,itoa(millis()%1200,ppmValue,10));
  delay(10000);
  #endif
}

int co2lamp(){
  int avgppm = readavgco2(kringel);

  if(avgppm < 0){
    //error reading CO2 avg
    currenthue = HSVHue::HUE_PINK; 
    changestate();
    return -1;
  }
  if(avgppm < CO2HIGH){
    currenthue = HSVHue::HUE_GREEN;
  }else if(avgppm > CO2CRITICAL){
    currenthue = HSVHue::HUE_RED;
  }else{
    currenthue = HSVHue::HUE_ORANGE;
  }
  changestate();
  return avgppm;
  //breathe(PAUSE,33);
  //kringel(hue,3000);
  //pattern(hue, 3,1000);
}

int readavgco2(void (*delayfun)()){
  int ppm = 0;
  int errcount=0;
  int count = 0;
  int avg = 0;
    while(errcount < 3 && count < CO2AVG){
      ppm = myMHZ19.getCO2(false);
      if(ppm < 1){
        errcount++;
        Serial.println("error reading CO2 ppm");
        continue;
      }
      Serial.printf("CO2 ppm: %d\n",ppm);
      count++;
      avg += ppm;
      delayfun();
  }
  if(errcount == 3){
    return -1;
  }
  avg /= CO2AVG;
  Serial.printf("AVG CO2 ppm: %d\n",avg);
  return avg;
}

void pause(unsigned long wait){
  unsigned long start = millis();
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
  unsigned long remaining = start + wait - millis();
  if(remaining > 0 ){
    FastLED.delay(remaining);
  }
}

void changestate(){
  changestate(currenthue);
}

void changestate(HSVHue hue){
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

void breathe(unsigned long duration, int delay) {
  unsigned long start = millis();
  while(start + duration > millis()){
    for(int b=BRIGHTNESS;b>0;b--){
      FastLED.setBrightness(b);
      FastLED.delay(delay);
    }
    for(int b=0;b<BRIGHTNESS;b++){
      FastLED.setBrightness(b);
      FastLED.delay(delay);
    }
  }
}

void pattern(HSVHue hue, int interations, long del){
  FastLED.setBrightness(BRIGHTNESS);
  for(int l = 0; l< interations; l++){
    for (int i = 0; i < NUM_LEDS; i+=2){
      leds[i].setHSV(hue,255,0);
      leds[i+1].setHSV(hue,255,BRIGHTNESS);
    }
    FastLED.delay(del);
    for (int i = 0; i < NUM_LEDS; i+=2){
      leds[i+1].setHSV(hue,255,0);
      leds[i].setHSV(hue,255,BRIGHTNESS);
    }
    FastLED.delay(del);
  }
}

void kringel(){
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
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

#ifdef CO2DEBUG
void calibrateco2(){
  myMHZ19.autoCalibration(false);
  Serial.println("Calibration, waiting 20 minutes to stabalise...");
  unsigned long timeElapse = 12e5;   //  20 minutes in milliseconds
  delay(timeElapse);
  Serial.println("Calibrating..");
  myMHZ19.calibrate();    // Take a reading which be used as the zero point for 400 ppm 
}

void co2test()
{
    if (millis() - getDataTimer >= 2000)
    {
        int CO2; 

        /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even 
        if below background CO2 levels or above range (useful to validate sensor). You can use the 
        usual documented command with getCO2(false) */

        CO2 = myMHZ19.getCO2(false);                             // Request CO2 (as ppm)
        
        Serial.print("CO2 (ppm): ");                      
        Serial.println(CO2);                                

        int8_t Temp;
        Temp = myMHZ19.getTemperature();                     // Request Temperature (as Celsius)
        Serial.print("Temperature (C): ");                  
        Serial.println(Temp);                               

        getDataTimer = millis();
    }
}

void ledtest(){
  static uint8_t hue;
  for(int i = 0; i < NUM_LEDS/2; i++) {   
    // fade everything out
    leds.fadeToBlackBy(40);

    // let's set an led value
    leds[i] = CHSV(hue++,255,255);

    // now, let's first 20 leds to the top 20 leds, 
    leds(NUM_LEDS/2,NUM_LEDS-1) = leds(NUM_LEDS/2 - 1 ,0);
    FastLED.delay(33);
  }
}

void ledeffecttest(){
  HSVHue hues[] = {HSVHue::HUE_GREEN,HSVHue::HUE_ORANGE,HSVHue::HUE_RED};
  for(int i =0;i<3;i++){
    changestate(hues[i]);
    breathe(3,20);
  }
}
#endif