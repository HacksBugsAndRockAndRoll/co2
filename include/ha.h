#define HA_DISCOVERY_TOPIC "homeassistant"
#define HA_NODE_ID "co2thomas"
#define HA_COMPONENT "sensor"
#define HA_OBJ_CO2 "ppm"
#define HA_OBJ_TEMP "temp"
#define HA_CONFIG "config"
#define HA_TYPE "co2"

#define HA_DISC_SENSOR HA_DISCOVERY_TOPIC "/" HA_COMPONENT "/" HA_NODE_ID

#define HA_DISC_PPM HA_DISC_SENSOR "/" HA_OBJ_CO2 "/" HA_CONFIG
#define HA_DISC_TEMP HA_DISC_SENSOR "/" HA_OBJ_TEMP "/" HA_CONFIG

#define HA_STAT HA_TYPE "/" HA_NODE_ID
#define HA_STAT_PPM HA_STAT "/" HA_OBJ_CO2
#define HA_STAT_TEMP HA_STAT "/" HA_OBJ_TEMP

