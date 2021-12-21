/** 
 * Motion sensor nightlight.
 * Reads output from a PIR Mini Motion Detector Sensor - 12mm Diameter - AM312 Chip 
 * Lights a WS-2812B light strip when it detects motion & stays lit for 30 sec after last motion
 * Fades light out after timeout so you can wave before the room goes dark
 */
#include <FastLED.h>
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define LED LED_BUILTIN

const int PIN_LED_DATA = 15;
const int PIN_MOTION_SENSOR = 16;
const int NUM_LEDS = 2;
const int BRIGHTNESS = 200; // less than full-on, for heat mitigation; the wife hates when my projects catch fire
const uint32_t LED_COLOR = 0xFF9505; // bright warm yellow
const int ONE_FRAME = 16;   // ms per brightness level when animating
const int INTERVAL = 30000; // stay lit for 30 sec after last motion

// Define the array of leds
CRGB leds[NUM_LEDS];

enum State {
  IDLE,
  LIGHTING,
  LIT,
  DIMMING
};

void updateLed(int brightness) {
    // Light the built-in LED if not idle; mostly for diagnostics
    digitalWrite(LED, brightness ? HIGH : LOW);

    // Update the LED strip
    leds[0] = LED_COLOR;
    leds[1] = LED_COLOR;
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(ONE_FRAME);
}

void setup() {
  // put your setup code here, to run once:
  // Serial.begin(9600);            // initialize serial

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(PIN_MOTION_SENSOR, INPUT);

  FastLED.addLeds<LED_TYPE, PIN_LED_DATA, COLOR_ORDER>(leds, NUM_LEDS);
  updateLed(0);
}

State nextState(int sensorPinValue, State statePrevious, int brightness, bool isIntervalElapsed) {
  switch (statePrevious) {
    case IDLE:
      if (sensorPinValue) {
        // Serial.println("Motion detected!");
        return LIGHTING;
      }
      break;
    case LIGHTING:
      if (isIntervalElapsed && !sensorPinValue) {
        // Serial.println("Motion stopped!");
        return DIMMING;
      } else if (brightness >= BRIGHTNESS) {
        return LIT;
      }
      break;
    case DIMMING: {
      if (sensorPinValue) {
        return LIGHTING;
      }
      if (!brightness) {
        return IDLE;
      }
      break;
    }
    case LIT: 
      if (isIntervalElapsed && !sensorPinValue) {
        return DIMMING;
      }
      break;
  }

  return statePrevious;
}

void loop() {
  // put your main code here, to run repeatedly:
  static State pinStateCurrent = IDLE;
  static State pinStatePrevious = IDLE;
  static int brightness = 0;
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();

  int sensorPinValue = digitalRead(PIN_MOTION_SENSOR);
  pinStateCurrent = nextState(sensorPinValue, pinStatePrevious, brightness, currentMillis - previousMillis >= INTERVAL);
  // Serial.print("State: ");
  // Serial.println(pinStateCurrent);

  switch (pinStateCurrent) {
    case LIGHTING:
      brightness = min(brightness + 1, BRIGHTNESS);
      updateLed(brightness);
      break;
    case DIMMING:
      if (brightness) {
        --brightness;
      }
      updateLed(brightness);
      break;
    case LIT:
      break;
  }

  // every time we see motion, restart the timeout
  if (sensorPinValue) {
    previousMillis = currentMillis;
  }
  pinStatePrevious = pinStateCurrent;
}
