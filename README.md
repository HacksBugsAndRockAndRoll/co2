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