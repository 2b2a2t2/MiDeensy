#pragma once

#include "Globals.h"
#include "MPR121_GestureHelper.h"

extern ButtonMap padButtons[];
extern const int NUM_PADS;
extern ButtonMap controlButtons[];
extern const int NUM_CONTROLS;
extern ButtonMap keyboardButtons[];
extern const int NUM_KEYS;

void handleTouchEvent(uint8_t sensorIndex, uint8_t channel, bool touched);
void handleGestureEvent(uint8_t sensorIndex, uint8_t channel, const char* gestureName, unsigned long duration);

ButtonMap* findButton(uint8_t sensor, uint8_t channel);
void retriggerHeldNotes(uint8_t newOctave);
