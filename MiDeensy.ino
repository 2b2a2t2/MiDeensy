#include <FastLED.h>
#include <Control_Surface.h>
#include <Wire.h>
#include "src/Globals.h"
#include "src/NoteLED.h"
#include "src/TouchHandler.h"
#include "src/MPR121_GestureHelper.h"

USBMIDI_Interface midi;
BidirectionalMIDI_PipeFactory<2> pipes;

MPR121_GestureHelper gestureHelper;

constexpr uint8_t ENCODER_A_PIN = 40;
constexpr uint8_t ENCODER_B_PIN = 39;

int8_t keyboardOctave = 3;
static int8_t lastEncA = HIGH;
static bool encoderMoved = false;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("MiDeensy Starting...");

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds.data, leds.length);
  FastLED.setCorrection(TypicalPixelString);
  FastLED.setBrightness(128);
  FastLED.clear();
  FastLED.show();

  RelativeCCSender::setMode(MACKIE_CONTROL_RELATIVE);
  Control_Surface | pipes | midi;
  Control_Surface.begin();

  Wire.begin();

  gestureHelper.begin(300, 200, 10);
  if (!gestureHelper.addSensor(0x5A)) Serial.println("Sensor 0x5A failed");
  if (!gestureHelper.addSensor(0x5B)) Serial.println("Sensor 0x5B failed");
  if (!gestureHelper.addSensor(0x5C)) Serial.println("Sensor 0x5C failed");
  if (!gestureHelper.addSensor(0x5D)) Serial.println("Sensor 0x5D failed");

  for (int i = 0; i < 4; i++) {
    gestureHelper.setThresholds(i, 40, 20);
  }

  gestureHelper.onTouchEvent(handleTouchEvent);
  gestureHelper.onGesture(handleGestureEvent);

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  Serial.println("=== Ready ===");
}

void loop() {
  Control_Surface.loop();
  gestureHelper.update();

  int8_t encA = digitalRead(ENCODER_A_PIN);
  if (encA == LOW && lastEncA == HIGH) {
    int8_t encB = digitalRead(ENCODER_B_PIN);
    int8_t newOctave = keyboardOctave + (encB == HIGH ? 1 : -1);
    if (newOctave < 0) newOctave = 0;
    if (newOctave > 7) newOctave = 7;
    if (newOctave != keyboardOctave) {
      retriggerHeldNotes(newOctave);
      keyboardOctave = newOctave;
    }
    encoderMoved = true;
  }
  lastEncA = encA;
}
