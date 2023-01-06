#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define HA_DISCOVERY_TOPIC "homeassistant"
#define HA_NODE_ID "co2thomas"
#define HA_COMP_SENSOR "sensor"
#define HA_COMP_BTN "button"
#define HA_OBJ_CO2 "ppm"
#define HA_OBJ_TEMP "temp"
#define HA_OBJ_BTN_CAL "calibrate"
#define HA_CONFIG "config"
#define HA_STATE "stat"
#define HA_COMMAND "cmnd"
#define HA_TYPE "co2"

#define HA_DISC_SENSOR HA_DISCOVERY_TOPIC "/" HA_COMP_SENSOR "/" HA_NODE_ID
#define HA_DISC_PPM HA_DISC_SENSOR "/" HA_OBJ_CO2 "/" HA_CONFIG
#define HA_DISC_TEMP HA_DISC_SENSOR "/" HA_OBJ_TEMP "/" HA_CONFIG
#define HA_STAT HA_TYPE "/" HA_NODE_ID
#define HA_STAT_PPM HA_STAT "/" HA_OBJ_CO2
#define HA_STAT_TEMP HA_STAT "/" HA_OBJ_TEMP

#define HA_DISC_BUTTON HA_DISCOVERY_TOPIC "/" HA_COMP_BTN "/" HA_NODE_ID
#define HA_DISC_CAL HA_DISC_BUTTON "/" HA_OBJ_BTN_CAL "/" HA_CONFIG
#define HA_STAT_CAL HA_STAT "/" HA_OBJ_BTN_CAL "/" HA_STATE
#define HA_CMND_CAL HA_STAT "/" HA_OBJ_BTN_CAL "/" HA_COMMAND
