#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <OneButton.h>

#include "config.h"

// Uncomment to enable debug print statements
//#define DEBUG

#define BUTTON_PIN D0 // Wemos D1 Mini D0
#define DATA_PIN D5 // Wemos D1 Mini D5

#define RAINBOW_MAX_VALUE 1280

#ifdef LED_STRIP
const int NUM_LEDS = 48;
const int NUM_STRIPS = 8;
const int LEDS_PER_STRIP = 6;
const int GRADIENT_LENGTH = LEDS_PER_STRIP;
const int STRIPE_WIDTH = LEDS_PER_STRIP;
#endif

#ifdef LED_SPIRAL
const int NUM_LEDS = 60;
const int GRADIENT_LENGTH = NUM_LEDS;
const int STRIPE_WIDTH = 10;
#endif

#ifndef MQTT_PROTECTED
const char* mqtt_username = NULL;
const char* mqtt_password = NULL;
#endif

#ifdef MQTT_PROTECTED
const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
#endif

// Delay Settings for Patterns
unsigned long prevRainbowTime = 0;
unsigned long rainbowInterval = 5;
unsigned int rainbowCycleCount = 0;
unsigned long prevWaveTime = 0;
unsigned long waveInterval = 400;
unsigned long prevBreatheTime = 0;
unsigned long breatheInterval = 5000;
unsigned long breatheHoldInterval = 1000;

enum lampStates {
  OFF,
  SOLID,
  STRIPES,
  GRADIENT,
  MOVING_GRADIENT,
  BREATHE,
  RAINBOW
};

enum colorTypes {
  RGB_COLOR,
  HSV_COLOR
};

lampStates lampState;
bool stateChange = false;
int color1[] = {0, 0, 0};
int color2[] = {0, 0, 0};
colorTypes colorType;

CRGB leds[NUM_LEDS];
CRGB temp_leds[LEDS_PER_STRIP];
CRGB gradient[GRADIENT_LENGTH];


int gradientStartPos = 0;  // Starting position of the gradient.
int gradientDelta = 1;  // 1 or -1.  (Negative value reverses direction.)

String mqtt_root_topic = MQTT_TOPIC_ROOT;
String mqtt_set_topic = mqtt_root_topic + "/" + DEVICE_ID + "/set";
String mqtt_status_topic = mqtt_root_topic + "/" + DEVICE_ID + "/status";
String mqtt_out_topic = mqtt_root_topic + "/" + PAIR_DEVICE_ID + "/set";
String mqttMessage;

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);

OneButton touch_btn = OneButton(BUTTON_PIN, false, false);
int currentPattern = 0;

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
  Serial.println("] ");
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
    JsonObject color_doc = doc["color"];
    const char* color_type = color_doc["type"];
    JsonArray colors = color_doc["values"].as<JsonArray>();

    switch (newState) {
      // OFF
      case 0: {
          lampState = OFF;
          stateChange = true;
          break;
        }

      // SOLID
      case 1: {
          lampState = SOLID;
          JsonObject color = colors[0];

          if (strcmp(color_type, "rgb") == 0) {
            colorType = RGB_COLOR;
            color1[0] = color["r"];
            color1[1] = color["g"];
            color1[2] = color["b"];
          }
          else if (strcmp(color_type, "hsv") == 0) {
            colorType = HSV_COLOR;
            color1[0] = color["h"];
            color1[1] = color["s"];
            color1[2] = color["v"];
          }
          stateChange = true;
          break;
        }

      // STRIPES
      case 2: {
          lampState = STRIPES;
          JsonObject c1 = colors[0];
          JsonObject c2 = colors[1];

          if (strcmp(color_type, "rgb") == 0) {
            colorType = RGB_COLOR;
            color1[0] = c1["r"];
            color1[1] = c1["g"];
            color1[2] = c1["b"];
            color2[0] = c2["r"];
            color2[1] = c2["g"];
            color2[2] = c2["b"];
          }
          else if (strcmp(color_type, "hsv") == 0) {
            colorType = HSV_COLOR;
            color1[0] = c1["h"];
            color1[1] = c1["s"];
            color1[2] = c1["v"];
            color2[0] = c2["h"];
            color2[1] = c2["s"];
            color2[2] = c2["v"];
          }
          stateChange = true;
          break;
        }

      // GRADIENT
      case 3: {
          lampState = GRADIENT;
          JsonObject c1 = colors[0];
          JsonObject c2 = colors[1];

          if (strcmp(color_type, "rgb") == 0) {
            colorType = RGB_COLOR;
            color1[0] = c1["r"];
            color1[1] = c1["g"];
            color1[2] = c1["b"];
            color2[0] = c2["r"];
            color2[1] = c2["g"];
            color2[2] = c2["b"];
          }
          else if (strcmp(color_type, "hsv") == 0) {
            colorType = HSV_COLOR;
            color1[0] = c1["h"];
            color1[1] = c1["s"];
            color1[2] = c1["v"];
            color2[0] = c2["h"];
            color2[1] = c2["s"];
            color2[2] = c2["v"];
          }
          stateChange = true;
          break;
        }

      // MOVING GRADIENT
      case 4: {
          lampState = MOVING_GRADIENT;
          JsonObject c1 = colors[0];
          JsonObject c2 = colors[1];

          if (strcmp(color_type, "rgb") == 0) {
            colorType = RGB_COLOR;
            color1[0] = c1["r"];
            color1[1] = c1["g"];
            color1[2] = c1["b"];
            color2[0] = c2["r"];
            color2[1] = c2["g"];
            color2[2] = c2["b"];
            fill_gradient_RGB(gradient, 0, CRGB(color1[0], color1[1], color1[2]), GRADIENT_LENGTH - 1, CRGB(color2[0], color2[1], color2[2]));
          }
          else if (strcmp(color_type, "hsv") == 0) {
            colorType = HSV_COLOR;
            color1[0] = c1["h"];
            color1[1] = c1["s"];
            color1[2] = c1["v"];
            color2[0] = c2["h"];
            color2[1] = c2["s"];
            color2[2] = c2["v"];
            fill_gradient(gradient, 0, CHSV(color1[0], color1[1], color1[2]), GRADIENT_LENGTH - 1, CHSV(color2[0], color2[1], color2[2]), SHORTEST_HUES);
          }

          stateChange = true;
          break;
        }

      // BREATHE
      case 5: {
          lampState = BREATHE;
          JsonObject color = colors[0];

          if (strcmp(color_type, "rgb") == 0) {
            colorType = RGB_COLOR;
            color1[0] = color["r"];
            color1[1] = color["g"];
            color1[2] = color["b"];
          }
          else if (strcmp(color_type, "hsv") == 0) {
            colorType = HSV_COLOR;
            color1[0] = color["h"];
            color1[1] = color["s"];
            color1[2] = (color["v"] == 0) ? 10 : color["v"];
          }
          stateChange = true;
          break;
        }

      // RAINBOW
      case 6: {
          lampState = RAINBOW;
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
    if (client.connect(DEVICE_ID, mqtt_username, mqtt_password)) {
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

/******************************* Button Callbacks *****************************/

static void handleDoubleTap() {
  static bool lightToggle = false;

  // Turn off lamp if on
  if (lampState != OFF) {
    lightToggle = true;
  }

  // Toggle Lamp
  if (lightToggle) {
    lampState = OFF;
    lightToggle = false;
    stateChange = true;
  } else {
    lampState = SOLID;
    colorType = HSV_COLOR;
    color1[0] = 0;
    color1[1] = 0;
    color1[2] = 255;
    lightToggle = true;
    stateChange = true;
  }
}

static void handleTap() {
#ifdef DEBUG
  Serial.println("Tapped!");
#endif
  StaticJsonDocument<256> doc;

  doc["state"] = lampState;

  JsonObject color = doc.createNestedObject("color");
  JsonArray color_values = color.createNestedArray("values");
  JsonObject color_values_0 = color_values.createNestedObject();
  JsonObject color_values_1 = color_values.createNestedObject();
  if (colorType == RGB_COLOR) {
    color["type"] = "rgb";
    color_values_0["r"] = color1[0];
    color_values_0["g"] = color1[1];
    color_values_0["b"] = color1[2];
    color_values_1["r"] = color2[0];
    color_values_1["g"] = color2[1];
    color_values_1["b"] = color2[2];
  }
  else if (colorType == HSV_COLOR) {
    color["type"] = "hsv";
    color_values_0["h"] = color1[0];
    color_values_0["s"] = color1[1];
    color_values_0["v"] = color1[2];
    color_values_1["h"] = color2[0];
    color_values_1["s"] = color2[1];
    color_values_1["v"] = color2[2];
  }

  mqttMessage = "";
  serializeJson(doc, mqttMessage);
  bool success = client.publish(mqtt_out_topic.c_str(), mqttMessage.c_str());
#ifdef DEBUG
  if (success) {
    Serial.println("Successfully sent!");
  } else {
    Serial.println("Failed to send...");
  }
#endif
}

static void handleLongTapStart() {
#ifdef DEBUG
  Serial.println("Long Tap Start!");
#endif
  currentPattern = 0;
  FastLED.clear();
  FastLED.show();
}

static void handleDuringLongPress() {
  static long lastPressed = 0;
  long duration = 800;
  long nextTime = lastPressed + duration;
  if (millis() >= nextTime) {
    if (touch_btn.isLongPressed()) {
#ifdef DEBUG
      Serial.println("During Long Tap!");
      Serial.print("currentPattern:");
      Serial.println(currentPattern);
#endif
      currentPattern++;
      switch (currentPattern) {

        //Red
        case 1:
          lampState = SOLID;
          colorType = HSV_COLOR;
          color1[0] = 4;
          color1[1] = 255;
          color1[2] = 180;
          stateChange = true;
          break;
        //Orange
        case 2:
          lampState = SOLID;
          colorType = HSV_COLOR;
          color1[0] = 11;
          color1[1] = 255;
          color1[2] = 180;
          stateChange = true;
          break;
        //Yellow
        case 3:
          lampState = SOLID;
          colorType = HSV_COLOR;
          color1[0] = 40;
          color1[1] = 255;
          color1[2] = 180;
          stateChange = true;
          break;
        //Green
        case 4:
          lampState = SOLID;
          colorType = HSV_COLOR;
          color1[0] = 78;
          color1[1] = 255;
          color1[2] = 180;
          stateChange = true;
          break;
        //Blue
        case 5:
          lampState = SOLID;
          colorType = HSV_COLOR;
          color1[0] = 160;
          color1[1] = 255;
          color1[2] = 180;
          stateChange = true;
          break;
        //Pink
        case 6:
          lampState = SOLID;
          colorType = HSV_COLOR;
          color1[0] = 225;
          color1[1] = 255;
          color1[2] = 180;
          stateChange = true;
          break;
        //Purple
        case 7:
          lampState = SOLID;
          colorType = HSV_COLOR;
          color1[0] = 200;
          color1[1] = 255;
          color1[2] = 180;
          stateChange = true;
          break;
        //White
        case 8:
          lampState = SOLID;
          colorType = HSV_COLOR;
          color1[0] = 0;
          color1[1] = 0;
          color1[2] = 180;
          stateChange = true;
          break;
      }
      lastPressed = millis();
    }
  }
}

/******************************* Lamp Loop *****************************/


void lamp_loop() {
  switch (lampState) {

    case OFF: {
        if (stateChange) {
          FastLED.clear();
          FastLED.show();
#ifdef DEBUG
          Serial.println("LED to off");
#endif
        }
        break;
      }

    case SOLID: {
        if (stateChange) {
          if (colorType == RGB_COLOR)
            fill_solid(leds, NUM_LEDS, CRGB(color1[0], color1[1], color1[2]));

          else if (colorType == HSV_COLOR)
            fill_solid(leds, NUM_LEDS, CHSV(color1[0], color1[1], color1[2]));

          FastLED.show();
#ifdef DEBUG
          Serial.println("LED to solid");
#endif
        }
        break;
      }

    case STRIPES: {
        if (stateChange) {
          stripes(STRIPE_WIDTH);
#ifdef DEBUG
          Serial.println("LED to stripes");
#endif
        }
        break;
      }

    case GRADIENT: {
        if (stateChange) {
          if (colorType == RGB_COLOR) {
#ifdef LED_STRIP
            fill_gradient_RGB(temp_leds, 0, CRGB(color1[0], color1[1], color1[2]), LEDS_PER_STRIP - 1, CRGB(color2[0], color2[1], color2[2]));
#endif
#ifdef LED_SPIRAL
            fill_gradient_RGB(leds, 0, CRGB(color1[0], color1[1], color1[2]), NUM_LEDS - 1, CRGB(color2[0], color2[1], color2[2]));
#endif
          }

          else if (colorType == HSV_COLOR) {
#ifdef LED_STRIP
            fill_gradient(temp_leds, LEDS_PER_STRIP, CHSV(color1[0], color1[1], color1[2]), CHSV(color2[0], color2[1], color2[2]), SHORTEST_HUES);
#endif
#ifdef LED_SPIRAL
            fill_gradient(leds, NUM_LEDS, CHSV(color1[0], color1[1], color1[2]), CHSV(color2[0], color2[1], color2[2]), SHORTEST_HUES);
#endif
          }
          applyToStrips();
          FastLED.show();
#ifdef DEBUG
          Serial.println("LED to gradient");
#endif
        }
        break;
      }

    case MOVING_GRADIENT: {
        unsigned long currentMillis = millis();
        if (currentMillis - prevWaveTime > waveInterval) {
          prevWaveTime = currentMillis;
          moving_gradient();
        }
        break;
      }

    case BREATHE: {
        static bool holdColor = false;
        unsigned long currentMillis = millis();

        if (!holdColor) {
          unsigned long interval = (color1[2] > 0) ? (breatheInterval / color1[2]) : 10;
          if (currentMillis - prevBreatheTime > interval) {
            prevBreatheTime = currentMillis;
            holdColor = breathe(stateChange);
          }
        } else {
          if (currentMillis - prevBreatheTime > breatheHoldInterval) {
            prevBreatheTime = currentMillis;
            holdColor = breathe(stateChange);
          }
        }

        break;
      }

    case RAINBOW: {
        if (stateChange || (rainbowCycleCount >= RAINBOW_MAX_VALUE)) {
          rainbowCycleCount = 0;
        }
        unsigned long currentMillis = millis();
        if (currentMillis - prevRainbowTime > rainbowInterval) {
          prevRainbowTime = currentMillis;
          rainbow();
          rainbowCycleCount++;
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
#ifdef DEBUG
  Serial.begin(115200);
#endif

  // Setup button
  touch_btn.setClickTicks(200);
  touch_btn.setPressTicks(500);
  touch_btn.attachClick(handleTap);
  touch_btn.attachDoubleClick(handleDoubleTap);
  touch_btn.attachLongPressStart(handleLongTapStart);
  touch_btn.attachDuringLongPress(handleDuringLongPress);
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

  bool isPressed = (digitalRead(BUTTON_PIN) == HIGH);
  touch_btn.tick(isPressed);
  lamp_loop();
}

/******************************* LED Patterns *****************************/

// Rainbow pattern is based on the rainbow function from the FastLED-Patterns repo by Resseguie

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

CRGB randomColor() {
  return Wheel(random(256));
}

// Rainbow colors that slowly cycle across LEDs
void rainbow() {
#ifdef LED_SPIRAL
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = Wheel(((i * 256 / NUM_LEDS) + rainbowCycleCount) & 255);
  }
#endif
#ifdef LED_STRIP
  for (int i = 0; i < LEDS_PER_STRIP; i++) {
    temp_leds[i] = Wheel(((i * 256 / LEDS_PER_STRIP) + rainbowCycleCount) & 255);
  }
  applyToStrips();
#endif
  FastLED.show();
}

// Breathing
bool breathe(bool firstRun) {
  static int value;
  static int fade;
  static long prevMillis;
  bool holdColor;

  if (firstRun) {
    fade = (color1[2] > 100) ? 5 : 1;
    value = fade;
  }

  if ((value <= color1[2]) && (value >= 0)) {
    holdColor = false;
    fill_solid(leds, NUM_LEDS, CHSV(color1[0], color1[1], value));
  }
  else if (value > color1[2]) {
    holdColor = true;
    fade = -fade;
  }
  else if (value < 0) {
    holdColor = true;
    fade = -fade;
  }

  FastLED.show();
  value += fade;
  return holdColor;
}

// Moving Gradient
void moving_gradient() {
#ifdef LED_SPIRAL
  uint8_t count = 0;
  for (uint8_t i = gradientStartPos; i < gradientStartPos + GRADIENT_LENGTH; i++) {
    leds[i % NUM_LEDS] = gradient[count];
    count++;
  }

  FastLED.show();
  FastLED.clear();

  gradientStartPos = gradientStartPos + gradientDelta;  // Update start position.
  if ( (gradientStartPos > NUM_LEDS - 1) || (gradientStartPos < 0) ) { // Check if outside NUM_LEDS range
    gradientStartPos = gradientStartPos % NUM_LEDS;  // Loop around as needed.
  }
#endif

#ifdef LED_STRIP
  uint8_t count = 0;
  for (uint8_t i = gradientStartPos; i < gradientStartPos + GRADIENT_LENGTH; i++) {
    temp_leds[i % LEDS_PER_STRIP] = gradient[count];
    count++;
  }
  applyToStrips();
  FastLED.show();
  FastLED.clear();

  gradientStartPos = gradientStartPos + gradientDelta;  // Update start position.
  if ( (gradientStartPos > LEDS_PER_STRIP - 1) || (gradientStartPos < 0) ) { // Check if outside NUM_LEDS range
    gradientStartPos = gradientStartPos % LEDS_PER_STRIP;  // Loop around as needed.
  }
#endif
}

// Display alternating stripes
void stripes(int width) {
  if (colorType == RGB_COLOR) {
    CRGB c1 = CRGB(color1[0], color1[1], color1[2]);
    CRGB c2 = CRGB(color2[0], color2[1], color2[2]);
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i % (width * 2) < width) {
        leds[i] = c1;
      }
      else {
        leds[i] = c2;
      }
    }
  }

  else if (colorType == HSV_COLOR) {
    CHSV c1 = CHSV(color1[0], color1[1], color1[2]);
    CHSV c2 = CHSV(color2[0], color2[1], color2[2]);
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i % (width * 2) < width) {
        leds[i] = c1;
      }
      else {
        leds[i] = c2;
      }
    }
  }
  FastLED.show();
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

void applyToStrips() {
  int led_index = 0;
  for (int j = 0; j < NUM_STRIPS; j++) {
    if (j % 2 == 0) {
      for (int i = 0; i < LEDS_PER_STRIP; i++) {
        leds[led_index] = temp_leds[i];
        led_index++;
      }
    } else {
      for (int i = LEDS_PER_STRIP - 1; i >= 0; i--) {
        leds[led_index] = temp_leds[i];
        led_index++;
      }
    }
  }
}
