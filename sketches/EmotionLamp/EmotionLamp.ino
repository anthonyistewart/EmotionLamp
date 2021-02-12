#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#include "config.h"

#define DATA_PIN D5 // Wemos D1 Mini D5
#define NUM_LEDS 60

enum lampStates {
  OFF,
  SOLID,
  WAVE,
  HEARTBEAT,
  BREATHE
};

enum colorTypes {
  RGB_COLOR,
  HSV_COLOR
};

lampStates lampState;
bool stateChange = false;
int hsv[] = {0, 0, 0};
int rgb[] = {0, 0, 0};
colorTypes colorType;

CRGB leds[NUM_LEDS];

String mqtt_root_topic = MQTT_TOPIC_ROOT;
String mqtt_set_topic = mqtt_root_topic + "/" + DEVICE_ID + "/set";
String mqtt_status_topic = mqtt_root_topic + "/" + DEVICE_ID + "/status";
String mqtt_out_topic = mqtt_root_topic + "/" + PAIR_DEVICE_ID + "/set";

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
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (doc["state"].is<int>()) {
    int newState = doc["state"];

    switch (newState) {
      case 0: {
          lampState = OFF;
          stateChange = true;
          break;
        }
      case 1: {
          lampState = SOLID;

          JsonObject color = doc["color"];
          const char* color_type = color["type"];

          if (strcmp(color_type, "rgb") == 0) {
            colorType = RGB_COLOR;
            rgb[0] = doc["color"]["r"];
            rgb[1] = doc["color"]["g"];
            rgb[2] = doc["color"]["b"];
          }
          else if (strcmp(color_type, "hsv") == 0) {
            colorType = HSV_COLOR;
            hsv[0] = doc["color"]["h"];
            hsv[1] = doc["color"]["s"];
            hsv[2] = doc["color"]["v"];
          }
          stateChange = true;
          break;
        }
      case 2: {
          lampState = WAVE;
          stateChange = true;
          break;
        }
      case 3: {
          lampState = BREATHE;

          if (doc.containsKey("color")) {
            JsonObject color = doc["color"];
            const char* color_type = color["type"];

            if (strcmp(color_type, "rgb") == 0) {
              colorType = RGB_COLOR;
              rgb[0] = doc["color"]["r"];
              rgb[1] = doc["color"]["g"];
              rgb[2] = doc["color"]["b"];
            }
            else if (strcmp(color_type, "hsv") == 0) {
              colorType = HSV_COLOR;
              hsv[0] = doc["color"]["h"];
              hsv[1] = doc["color"]["s"];
              hsv[2] = doc["color"]["v"];
            }
          }

          stateChange = true;
          break;
        }
      default: {
          stateChange = false;
          break;
        }
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

void lamp_loop() {
  switch (lampState) {
    case OFF: {
        if (stateChange) {
          FastLED.clear();
          FastLED.show();
          Serial.println("LED to off");
        }
        break;
      }
    case SOLID: {
        if (stateChange) {
          if (colorType == RGB_COLOR)
            fill_solid(leds, NUM_LEDS, CRGB(rgb[0], rgb[1], rgb[2]));

          else if (colorType == HSV_COLOR)
            fill_solid(leds, NUM_LEDS, CHSV(hsv[0], hsv[1], hsv[2]));

          FastLED.show();
          Serial.println("LED to solid");
        }
        break;
      }
    case WAVE: {
        rainbow(5, 1);
        Serial.println("LED to wave");
        break;
      }
    case HEARTBEAT: {
        break;
      }
    case BREATHE: {
        if (stateChange) {
          breathe(true, 10, 500);
        } else {
          breathe(false, 10, 500);
        }
        break;
      }
    default: {
        break;
      }
  }
  stateChange = false;
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

/******************************* LED Patterns *****************************/

// Rainbow pattern is sourced from the FastLED-Patterns repo by Resseguie

// Rainbow colors that slowly cycle across LEDs
void rainbow(int cycles, int speed) {
  if (cycles == 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = Wheel(((i * 256 / NUM_LEDS)) & 255);
    }
    FastLED.show();
  }
  else {
    for (int j = 0; j < 256 * cycles; j++) {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      }
      FastLED.show();
      delay(speed);
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
CRGB Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// Breathing
void breathe(bool firstRun, int speed, int holdTime) {
  static int val;
  static int fade;
  static long prevMillis;
  static bool swap_fade;

  if (firstRun) {
    fade = 5;
    val = fade;
  }
  Serial.println(val);
  if (val < 255) {
    delay(speed);
    fill_solid(leds, NUM_LEDS, CHSV(hsv[0], hsv[1], val));
  }
  if (val == 255) {
    delay(holdTime);
    fade = -fade;
  } else if (val == 0) {
    delay(holdTime);
    fade = -fade;
  }
  
  FastLED.show();
  val += fade;
}
