#pragma once

#include "Mapping.h"

void touchBegin();
void touchUpdate();
void handleTouchEvent(uint8_t sensorIndex, uint8_t channel, bool touched);
void handleGestureEvent(uint8_t sensorIndex, uint8_t channel, const char* gestureName, unsigned long duration);
void retriggerHeldNotes(uint8_t newOctave);
