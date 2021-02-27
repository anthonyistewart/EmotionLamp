#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#include "config.h"

#define BUTTON_PIN D0 // Wemos D1 Mini D0
#define DATA_PIN D5 // Wemos D1 Mini D5
#define NUM_LEDS 60

// Uncomment to enable debug print statements
//#define DEBUG

const int debounceDelay = 500;

enum lampStates {
  OFF,
  SOLID,
  WAVE,
  SIREN,
  RAINBOW,
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

#ifdef DEBUG
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
#endif
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
  }

  randomSeed(micros());

#ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif
}

/******************************* MQTT Callback *****************************/

void callback(char* topic, byte* payload, unsigned int length) {
#ifdef DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
#endif

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
#ifdef DEBUG
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
#endif
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
          lampState = RAINBOW;
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
      case 4: {
          lampState = WAVE;

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
      case 5: {
          lampState = SIREN;

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
#ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
#endif

    // Attempt to connect
    if (client.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
#ifdef DEBUG
      Serial.println("Connected");
#endif

      // Publish initialization message
      client.subscribe(mqtt_set_topic.c_str());
    }
    else {
#ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
#endif

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void button_check() {
  static int prevState;
  static long lastDebounceTime;
  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == prevState) return;

  bool debounce = false;

  if ((millis() - lastDebounceTime) <= debounceDelay) {
    debounce = true;
  }

  lastDebounceTime = millis();

  if (debounce) return;

  prevState = buttonState;

  client.publish(mqtt_out_topic.c_str(), "1");

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
    case RAINBOW: {
        rainbow(5, 1);
        break;
      }
    case WAVE: {
        wave(2);
        break;
      }
    case SIREN: {
        wave(50);
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

  // Setup button
  pinMode(BUTTON_PIN, INPUT);

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

  // Startup Indicator
  startup_flash();
  client.publish(mqtt_status_topic.c_str(), "init");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  button_check();
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

// Wave
void wave(int speed) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hsv[0], hsv[1], hsv[2]);
    FastLED.show();
    leds[i] = CRGB::Black;
    delay(speed);
  }
}

// Startup Flash
void startup_flash() {
  for (int i = 0; i < 3; i++) {
    fill_solid(leds, NUM_LEDS, CHSV(0, 0, 10));
    FastLED.show();
    delay(200);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(200);
  }

}
