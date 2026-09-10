#include "NoteLED.h"

CRGB leds[NUM_LEDS];

void setLED(uint8_t ledIndex, bool on) {
  if (ledIndex >= NUM_LEDS) return;
  leds[ledIndex] = on ? CRGB::White : CRGB::Black;
  FastLED.show();
}
