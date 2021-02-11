#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
//#define FASTLED_ESP8266_D1_PIN_ORDER
#include <FastLED.h>

#include "config.h"

#define DATA_PIN D5 // Wemos D1 Mini D5
#define NUM_LEDS 60

int h = 0;
int s = 0;
int v = 0;

long previousMillis = 0; 
long interval = 5000;

int movingLed = 0;

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

  h = doc["h"]; 
  s = doc["s"];
  v = doc["v"];
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

void setup() {
  delay(2000);
  Serial.begin(9600);

  // Setup LED Strip
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  
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
  fill_solid(leds, NUM_LEDS, CHSV(h,s,v));
  FastLED.show();
//  unsigned long currentMillis = millis();
//  
//  if(currentMillis - previousMillis > 100) {
//    previousMillis = currentMillis;
//
//    if(movingLed == NUM_LEDS - 1){
//      movingLed = 0;
//      leds[NUM_LEDS - 1] = CRGB::Black;
//    } else{
//      leds[movingLed-1] = CRGB::Black;
//    }
//    
//    // Turn our current led on to white, then show the leds
//    leds[movingLed].setHSV(h,s,v);
//
//    // Show the leds (only one of which is set to white, from above)
//    FastLED.show();
//    movingLed++;
//    
//  }
}
