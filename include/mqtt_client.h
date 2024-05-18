//=====================MQTT DEFINES============================
#define MQTT_BROKER     "localhost"
#define MQTT_PORT       8883
    //TOPICS
#define GAS_TOPIC_VALUE         "/info/sensor/gas"
#define DOOR_TOPIC_VALUE        "/info/sensor/door"
#define PRESENCE_TOPIC_VALUE    "/info/sensor/presence"

#define DEVICE_INFO             "/info/device"
#define ALARM_TRIGGER           "/info/alarm"

#define CFG_ALARM               "/config/alarm"
#define CFG_STREAM              "/config/stream"
#define CFG_DOOR                "/config/sensor/door"
#define CFG_GAS                 "/config/sensor/gas"
#define CFG_PRESENCE            "/config/sensor/presence"
#define CFG_ALL                 "/config/sensor/all"
#define REBOOT                  "/config/device/reboot"

#define DEVICE_SENSORS          "/info/device/sensors"
#define REQ_INFO                "/info/device/req"

//=======================SENSORS===============================
#define ALARM_ID            0
#define DOOR_ID             1
#define GAS_ID              2
#define PRESENCE_ID         3
#define ALL_ID              4
#define REQ_INFO_ID         5
#define REBOOT_ID           6
#define DOOR_SENSOR         "Door Sensor"
#define PRESENCE_SENSOR     "Presence Sensor"
#define GAS_SENSOR          "Gas Sensor"