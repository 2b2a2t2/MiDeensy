#include <FastLED.h>
#include <Wire.h>
#include "src/Globals.h"
#include "src/Midi.h"
#include "src/Display.h"
#include "src/NoteLED.h"
#include "src/TouchHandler.h"
#include "src/Encoder.h"

Oled display;
int8_t keyboardOctave = 3;

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
  FastLED.clear();
  FastLED.show();
  midiBegin();
  Wire.begin();
  display.begin();
  touchBegin();
  encBegin();
}

void loop() {
  midiUpdate();
  touchUpdate();
  int8_t d = encUpdate(0);
  if (d) {
    int8_t o = constrain(keyboardOctave + d, 0, 7);
    if (o != keyboardOctave) { retriggerHeldNotes(o); keyboardOctave = o; }
  }
  static uint8_t lastT = 0;
  uint8_t now = millis() >> 4;
  if (now != lastT) {
    lastT = now;
    int8_t vals[8];
    for (uint8_t i = 0; i < 8; i++) vals[i] = encValue(i);
    display.clear();
    display.showEncoders(vals);
    display.show();
  }
}
