#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include "OneButton.h"

#include "config.h"

#define BUTTON_PIN D0 // Wemos D1 Mini D0
#define DATA_PIN D5 // Wemos D1 Mini D5
#define NUM_LEDS 60

#define RAINBOW_MAX_VALUE 1280
#define STRIPE_WIDTH 10

// Uncomment to enable debug print statements
//#define DEBUG

// Delay Settings for Patterns
unsigned long prevRainbowTime = 0;
unsigned long rainbowInterval = 5;
unsigned int rainbowCycleCount = 0;

enum lampStates {
  OFF,
  SOLID,
  STRIPES,
  GRADIENT,
  BREATHE,
  WAVE,
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

String mqtt_root_topic = MQTT_TOPIC_ROOT;
String mqtt_set_topic = mqtt_root_topic + "/" + DEVICE_ID + "/set";
String mqtt_status_topic = mqtt_root_topic + "/" + DEVICE_ID + "/status";
String mqtt_out_topic = mqtt_root_topic + "/" + PAIR_DEVICE_ID + "/set";

WiFiClient espClient;
PubSubClient client(espClient);

OneButton *touch_btn = new OneButton();

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

      // BREATHE
      case 4: {
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
            color1[2] = color["v"];
          }
          stateChange = true;
          break;
        }

      // WAVE
      case 5: {
          lampState = WAVE;
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

static void handleeTap() {
  Serial.println("Tapped!");
}

static void handleLongTap() {
  Serial.println("Long Tap!");
}

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
    case GRADIENT: {
        if (stateChange) {
          if (colorType == RGB_COLOR)
            fill_gradient_RGB(leds, 0, CRGB(color1[0], color1[1], color1[2]), NUM_LEDS - 1, CRGB(color2[0], color2[1], color2[2]));

          else if (colorType == HSV_COLOR)
            fill_gradient_RGB(leds, NUM_LEDS, CHSV(color1[0], color1[1], color1[2]), CHSV(color2[0], color2[1], color2[2]), FORWARD_HUES);
          FastLED.show();
#ifdef DEBUG
          Serial.println("LED to gradient");
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
    case WAVE: {
        wave(2);
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
  touch_btn->setPressTicks(2000);
  touch_btn->attachDoubleClick(handleDoubleTap);
  //touch_btn->attachClick(handleTap);
  //touch_btn->attachLongPressStart(handleLongTap);
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
  touch_btn->tick(isPressed);
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
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = Wheel(((i * 256 / NUM_LEDS) + rainbowCycleCount) & 255);
  }
  FastLED.show();
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
    fill_solid(leds, NUM_LEDS, CHSV(color1[0], color1[1], val));
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

//Wave
void wave(int speed) {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (colorType == RGB_COLOR) {
      leds[i] = CRGB(color1[0], color1[1], color1[2]);
    }
    else if (colorType == HSV_COLOR) {
      leds[i] = CHSV(color1[0], color1[1], color1[2]);
    }
    FastLED.show();
    leds[i] = CRGB::Black;
    delay(speed);
  }
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
