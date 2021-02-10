#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#define FASTLED_ESP8266_D1_PIN_ORDER
#include <FastLED.h>

#include "config.h"

#define LED_PIN 16 // Wemos D1 Mini D5

int hue = 0;
float brightness = 0.0;
float saturation = 0.0;
long previousMillis = 0; 

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

  brightness = doc["brightness"];
  saturation = doc["saturation"];
  hue = doc["hue"]; 
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

void setLights(int new_hue, float new_saturation, float new_brightness){
  hue = new_hue;
  saturation = new_saturation;
  brightness = new_brightness;
}

void setup() {
  Serial.begin(9600);
  
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
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis > 5000) {
    previousMillis = currentMillis;  
    Serial.print("Hue: ");
    Serial.print(hue);
    Serial.print(", Saturation: ");
    Serial.print(saturation);
    Serial.print(", Brightness: ");
    Serial.print(brightness);
    Serial.println("");
  }
}
