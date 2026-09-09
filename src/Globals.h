#pragma once

#include <FastLED.h>
#include <Control_Surface.h>

#define BLACK 0
#define WHITE 1

constexpr uint8_t LED_PIN = 6;
constexpr uint8_t NUM_LEDS = 42;

constexpr uint8_t PAD_NOTE_BASE = 59;
constexpr uint8_t KEY_NOTE_BASE = 84;
constexpr uint8_t CTRL_NOTE_BASE = 75;

enum ButtonType {
  TYPE_PAD,
  TYPE_CONTROL,
  TYPE_KEYBOARD
};

struct ButtonMap {
  uint8_t sensor;
  uint8_t channel;
  const char* name;
  ButtonType type;
  uint8_t midiNote;
  uint8_t ledIndex;
  bool isPressed;
  unsigned long pressTime;
  bool isModeButton;
};
