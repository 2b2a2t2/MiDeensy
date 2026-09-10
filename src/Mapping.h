#pragma once

#include <cstdint>

enum ButtonType : uint8_t {
  TYPE_PAD,
  TYPE_CONTROL,
  TYPE_KEYBOARD
};

struct SensorMap {
  uint8_t sensor;
  uint8_t channel;
  uint8_t ledIndex;
  const char* name;
  ButtonType type;
};

// Single source of truth: sensor address → LED index
// Pads (0-15), Controls (16-24, 32-36), Keyboard (25-31, 37-41)
constexpr SensorMap BUTTON_MAP[] = {
  // Pads
  { 1,  1,  0, "PAD1",  TYPE_PAD },
  { 1,  4,  1, "PAD2",  TYPE_PAD },
  { 1,  2,  2, "PAD3",  TYPE_PAD },
  { 3, 10,  3, "PAD4",  TYPE_PAD },
  { 1,  0,  4, "PAD5",  TYPE_PAD },
  { 3,  8,  5, "PAD6",  TYPE_PAD },
  { 3,  2,  6, "PAD7",  TYPE_PAD },
  { 2,  0,  7, "PAD8",  TYPE_PAD },
  { 3,  0,  8, "PAD9",  TYPE_PAD },
  { 2,  5,  9, "PAD10", TYPE_PAD },
  { 3,  5, 10, "PAD11", TYPE_PAD },
  { 2,  1, 11, "PAD12", TYPE_PAD },
  { 0,  0, 12, "PAD13", TYPE_PAD },
  { 2,  7, 13, "PAD14", TYPE_PAD },
  { 2,  9, 14, "PAD15", TYPE_PAD },
  { 0,  1, 15, "PAD16", TYPE_PAD },

  // Controls
  { 3,  9, 16, "PREV", TYPE_CONTROL },
  { 1,  5, 17, "NEXT", TYPE_CONTROL },
  { 1,  3, 18, "M1",   TYPE_CONTROL },
  { 3,  6, 19, "M2",   TYPE_CONTROL },
  { 3, 11, 20, "M3",   TYPE_CONTROL },
  { 3,  7, 21, "F1",   TYPE_CONTROL },
  { 3,  3, 22, "F2",   TYPE_CONTROL },
  { 3,  1, 23, "F3",   TYPE_CONTROL },
  { 3,  4, 24, "F4",   TYPE_CONTROL },
  { 0, 10, 32, "ENC",  TYPE_CONTROL },
  { 0, 11, 33, "SEQ",  TYPE_CONTROL },
  { 0,  9, 34, "KEY",  TYPE_CONTROL },
  { 0,  8, 35, "PLAY", TYPE_CONTROL },
  { 0,  7, 36, "REC",  TYPE_CONTROL },

  // Keyboard
  { 2,  2, 25, "KEY_C",  TYPE_KEYBOARD },
  { 0,  5, 37, "KEY_C#", TYPE_KEYBOARD },
  { 2,  3, 26, "KEY_D",  TYPE_KEYBOARD },
  { 0,  4, 38, "KEY_D#", TYPE_KEYBOARD },
  { 2,  4, 27, "KEY_E",  TYPE_KEYBOARD },
  { 2, 10, 28, "KEY_F",  TYPE_KEYBOARD },
  { 0,  2, 39, "KEY_F#", TYPE_KEYBOARD },
  { 2,  6, 29, "KEY_G",  TYPE_KEYBOARD },
  { 0,  3, 40, "KEY_G#", TYPE_KEYBOARD },
  { 2, 11, 30, "KEY_A",  TYPE_KEYBOARD },
  { 0,  6, 41, "KEY_A#", TYPE_KEYBOARD },
  { 2,  8, 31, "KEY_B",  TYPE_KEYBOARD },
};

constexpr uint8_t NUM_BUTTONS = sizeof(BUTTON_MAP) / sizeof(BUTTON_MAP[0]);

inline const SensorMap* findButton(uint8_t sensor, uint8_t channel) {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (BUTTON_MAP[i].sensor == sensor && BUTTON_MAP[i].channel == channel)
      return &BUTTON_MAP[i];
  }
  return nullptr;
}
