This project is a DIY CO2 indicator lamp integrated with HomeAssistant via MQTT.
There is a sketch file showing the wiring and parts used.
There are STL files for a case (inspired by https://www.printables.com/de/model/302781-air-quality-lamp-for-co2-metering-mh-z19-esp32-and).

To get going you need to create your own secrets.h file containing your specifics and place it in /include

```
//this needs to be adapted for each device
#define HA_NODE_ID "CO2DEVICENAME"

#define WIFI_PASSWORD "YOURWIFIPASSWORD"
#define WIFI_SSID "YOURWIFISSID"
#define MQTT_BROKER_IP "YOURMQTTBROKERIP"
```

Hardware:

1x ESP8266 (D1 mini)
1x Logic level shifter
1x 24 bit WS2812 Ring
1x MH-Z19B

HomeAssistant integration:

This is done via MQTT autodiscovery (https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) which is why HA_NODE_ID needs to be uniq per device. 
The device self registers CO2 and temperature sensors (temperature is not really useful since it is some internal measurement of the MH-Z19B used for correction). 
There is also a button registered to manually trigger a calibration. 
There is also a switch reigstered (with state sync to Home Assistant) to turn the lights on/off.