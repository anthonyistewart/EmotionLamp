#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
//#define FASTLED_ESP8266_D1_PIN_ORDER
#include <FastLED.h>

#include "config.h"

#define DATA_PIN D5 // Wemos D1 Mini D5
#define NUM_LEDS 60

enum lampStates {
  OFF,
  SOLID,
  WAVE,
  HEARTBEAT,
  BREATH
};

lampStates lampState;
bool stateChange = false;
int h = 0;
int s = 0;
int v = 0;

long previousMillis = 0; 
long interval = 5000;

CRGB leds[NUM_LEDS];

String mqtt_root_topic = MQTT_TOPIC_ROOT;
String mqtt_set_topic = mqtt_root_topic+"/"+DEVICE_ID+"/set";
String mqtt_status_topic = mqtt_root_topic+"/"+DEVICE_ID+"/status";
String mqtt_out_topic = mqtt_root_topic+"/"+PAIR_DEVICE_ID+"/set";

WiFiClient espClient;
PubSubClient client(espClient);

/******************************* WIFI Setup *******************************/

void setup_wifi() {
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/******************************* MQTT Callback *****************************/

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);

  if(doc['mode'].is<int>()){
    int mode = doc['mode'];
    switch(mode){
      case 0:
        lampState = OFF;
        stateChange = true;
        break;
      case 1:
        lampState = SOLID;
        // TO-DO: Check for color field (HSV, RGB, GRADIENT, RAINBOW)
        h = doc["color"]["h"];
        s = doc["color"]["s"];
        v = doc["color"]["v"];
        stateChange = true;
        break;
      default:
        stateChange = false;
        break;
    }
  }
}

/******************************* MQTT Reconnect *****************************/

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("Connected");

      // Publish initialization message
      client.subscribe(mqtt_set_topic.c_str());
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void lamp_loop(){
  switch(lampState){
    case OFF:
      if(stateChange){
        FastLED.clear();
        FastLED.show();
        stateChange = false;
      }
      break;
    case SOLID:
      if(stateChange){
        fill_solid(leds, NUM_LEDS, CHSV(h,s,v));
        FastLED.show();
        stateChange = false;
      }
      break;
    case WAVE:
      break;
    case HEARTBEAT:
      break;
    case BREATH:
      break;
    default:
      break;
  }
}

void setup() {
  delay(2000);
  Serial.begin(9600);

  // Setup LED Strip
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  // Clear LED state and reset strip
  lampState = OFF;
  FastLED.clear();
  FastLED.show();
  
  // Setup WiFi
  setup_wifi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  // Send out initialization message
  if (!client.connected()) {
    reconnect();
  }
  client.publish(mqtt_status_topic.c_str(), "init");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  lamp_loop();
}
