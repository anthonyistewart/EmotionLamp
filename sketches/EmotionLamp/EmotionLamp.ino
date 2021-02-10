#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#include <ArduinoJson.h>
#include "config.h"

int hue = 0;
float brightness = 0.0;
float saturation = 0.0;

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

void callback(char* topic, byte* payload_in, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  /*

  char raw[length];

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload_in[i]);
    raw[i] = payload_in[i];
  }
  deserializeJson(json_doc, raw);

  uint16_t n = json_doc["id"];
  const char* c = json_doc["cmd"];
  Serial.println(n);
  Serial.println(c);*/
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
      client.subscribe(MQTT_TOPIC_IN);
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
  Serial.begin(9600);
  
  // Setup WiFi
  setup_wifi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  // Send out initialization message
  if (!client.connected()) {
    reconnect();
  }
  client.publish(MQTT_TOPIC_OUT, "init");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
