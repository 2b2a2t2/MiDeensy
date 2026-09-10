#include "Encoder.h"
#include <Arduino.h>
#include <usb_midi.h>

static constexpr uint8_t PINS[NUM_ENCODERS][2] = {
  {40, 39}, {36, 35}, {34, 33}, {31, 32},
  {38, 37}, {26, 25}, {27, 28}, {29, 30}
};

static uint8_t prev[NUM_ENCODERS];
static uint8_t values[NUM_ENCODERS];

void encBegin() {
  for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
    pinMode(PINS[i][0], INPUT_PULLUP);
    pinMode(PINS[i][1], INPUT_PULLUP);
    uint8_t a = digitalRead(PINS[i][0]);
    uint8_t b = digitalRead(PINS[i][1]);
    prev[i] = (a << 1) | b;
    values[i] = 64;
  }
}

int8_t encUpdate(uint8_t index) {
  uint8_t a = digitalRead(PINS[index][0]);
  uint8_t b = digitalRead(PINS[index][1]);
  uint8_t state = (a << 1) | b;
  uint8_t combined = (prev[index] << 2) | state;
  prev[index] = state;

  int8_t d = 0;
  switch (combined) {
    case 0b0001: case 0b0111: case 0b1110: case 0b1000: d =  1; break;
    case 0b0010: case 0b0100: case 0b1101: case 0b1011: d = -1; break;
    default: return 0;
  }

  int16_t v = values[index] + d;
  v = constrain(v, 0, 127);
  values[index] = v;
  usbMIDI.sendControlChange(ENC_CC_BASE + index, v, 1);
  return d;
}

int8_t encValue(uint8_t index) {
  return values[index];
}
