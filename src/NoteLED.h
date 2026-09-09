#pragma once

#include "Globals.h"

extern Array<CRGB, NUM_LEDS> leds;

void setLED(uint8_t ledIndex, bool on);
