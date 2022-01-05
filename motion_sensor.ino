/** 
 * Motion sensor nightlight.
 * Reads output from a PIR Mini Motion Detector Sensor - 12mm Diameter - AM312 Chip 
 * Lights a WS-2812B light strip when it detects motion & stays lit for 30 sec after last motion
 * Fades light out after timeout so you can wave before the room goes dark
 */
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <FastLED.h>
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define LED LED_BUILTIN
//#define DEBUG 1

const int PIN_LED_DATA = 15;
const int PIN_MOTION_SENSOR = 7;
const int NUM_LEDS = 2;
const int BRIGHTNESS = 200; // less than full-on, for heat mitigation; the wife hates when my projects catch fire
const uint32_t LED_COLOR = 0xFF9505; // bright warm yellow
const int ONE_FRAME = 16;   // ms per brightness level when animating
#ifdef DEBUG
const int INTERVAL = 3000; // stay lit for 3 sec after last motion
#else
const int INTERVAL = 30000; // stay lit for 30 sec after last motion
#endif

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
//    digitalWrite(LED, brightness ? HIGH : LOW);

    // Update the LED strip
    leds[0] = LED_COLOR;
    leds[1] = LED_COLOR;
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(ONE_FRAME);
}

void SleepCPU(void)
{
  // disable ADC
  ADCSRA = 0;
 
  // Sets the sleep mode. Using IDLE because it doesn't wake from PWR_DOWN; likely because I power the motion sensor from the Arduino.
  set_sleep_mode(SLEEP_MODE_IDLE);
  cli();
  sleep_enable(); // set sleep bit
  attachInterrupt(digitalPinToInterrupt(PIN_MOTION_SENSOR), Wake_CPU, RISING); // attach interrupt to wake CPU after sleep.
  digitalWrite(LED, LOW); // turn off LED to show sleep mode
  delay(1000);
  sei();
  sleep_cpu(); // put to sleep - will wake up here.
  sleep_disable();
  digitalWrite(LED, HIGH); // turning LED on
}

// Interrupt handler
void Wake_CPU(void)
{
  detachInterrupt(digitalPinToInterrupt(PIN_MOTION_SENSOR)); // Removes the interrupt from pin 7
#ifdef DEBUG
  Serial.println("woke up"); // first line of code executed after sleep.
#endif
}

void setup() {
  // put your setup code here, to run once:
#ifdef DEBUG
  Serial.begin(57600);            // initialize serial
#endif
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  pinMode(PIN_MOTION_SENSOR, INPUT);

  FastLED.addLeds<LED_TYPE, PIN_LED_DATA, COLOR_ORDER>(leds, NUM_LEDS);
  updateLed(0);
}

State nextState(int sensorPinValue, State statePrevious, int brightness, bool isIntervalElapsed) {
  switch (statePrevious) {
    case IDLE:
      if (sensorPinValue) {
#ifdef DEBUG
        Serial.println("Motion detected");
#endif
        return LIGHTING;
      }
      break;
    case LIGHTING:
      if (isIntervalElapsed && !sensorPinValue) {
#ifdef DEBUG
        Serial.println("Motion stopped while lighting");
#endif
        return DIMMING;
      } else if (brightness >= BRIGHTNESS) {
        return LIT;
      }
      break;
    case DIMMING: {
      if (sensorPinValue) {
#ifdef DEBUG
        Serial.println("Motion detected while dimming");
#endif
        return LIGHTING;
      }
      if (!brightness) {
        return IDLE;
      }
      break;
    }
    case LIT: 
      if (isIntervalElapsed && !sensorPinValue) {
#ifdef DEBUG
        Serial.println("Timeout while Motion stopped");
#endif
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

  const int sensorPinValue = digitalRead(PIN_MOTION_SENSOR);
  pinStateCurrent = nextState(sensorPinValue, pinStatePrevious, brightness, currentMillis - previousMillis >= INTERVAL);

  // every time we see motion, restart the timeout
  if (sensorPinValue) {
    previousMillis = currentMillis;
  }

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
    case IDLE:
      SleepCPU();
      break;
  }

  pinStatePrevious = pinStateCurrent;
}
