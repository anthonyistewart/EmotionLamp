// Copy this file and rename it config.h, then fill in your credentials

/******************************* WIFI **************************************/

#define WIFI_SSID       ""
#define WIFI_PASS       ""

/******************************* MQTT **************************************/

#define MQTT_BROKER     ""
#define MQTT_PORT       1883

// If MQTT broker is password protected, uncomment these lines and enter credentials
/*
#define MQTT_PROTECTED
#define MQTT_USERNAME   ""
#define MQTT_PASSWORD   ""
*/

#define MQTT_TOPIC_ROOT   ""

/******************************* DEVICE **************************************/

#define DEVICE_ID ""
#define PAIR_DEVICE_ID ""

// Uncomment one of these lines depending on the LED configuration
#define LED_STRIP
//#define LED_SPIRAL
