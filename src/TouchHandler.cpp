#include "TouchHandler.h"
#include "NoteLED.h"

extern int8_t keyboardOctave;

ButtonMap padButtons[] = {
  { 1, 1, "PAD1", TYPE_PAD, 59, 0, false, 0, false },
  { 1, 4, "PAD2", TYPE_PAD, 60, 1, false, 0, false },
  { 1, 2, "PAD3", TYPE_PAD, 61, 2, false, 0, false },
  { 3, 10, "PAD4", TYPE_PAD, 62, 3, false, 0, false },
  { 1, 0, "PAD5", TYPE_PAD, 63, 4, false, 0, false },
  { 3, 8, "PAD6", TYPE_PAD, 64, 5, false, 0, false },
  { 3, 2, "PAD7", TYPE_PAD, 65, 6, false, 0, false },
  { 2, 0, "PAD8", TYPE_PAD, 66, 7, false, 0, false },
  { 3, 0, "PAD9", TYPE_PAD, 67, 8, false, 0, false },
  { 2, 5, "PAD10", TYPE_PAD, 68, 9, false, 0, false },
  { 3, 5, "PAD11", TYPE_PAD, 69, 10, false, 0, false },
  { 2, 1, "PAD12", TYPE_PAD, 70, 11, false, 0, false },
  { 0, 0, "PAD13", TYPE_PAD, 71, 12, false, 0, false },
  { 2, 7, "PAD14", TYPE_PAD, 72, 13, false, 0, false },
  { 2, 9, "PAD15", TYPE_PAD, 73, 14, false, 0, false },
  { 0, 1, "PAD16", TYPE_PAD, 74, 15, false, 0, false }
};
const int NUM_PADS = sizeof(padButtons) / sizeof(padButtons[0]);

ButtonMap controlButtons[] = {
  { 3, 9, "PREV", TYPE_CONTROL, 75, 16, false, 0, false },
  { 1, 5, "NEXT", TYPE_CONTROL, 76, 17, false, 0, false },
  { 1, 3, "M1", TYPE_CONTROL, 77, 18, false, 0, false },
  { 3, 6, "M2", TYPE_CONTROL, 78, 19, false, 0, false },
  { 3, 11, "M3", TYPE_CONTROL, 79, 20, false, 0, false },
  { 3, 7, "F1", TYPE_CONTROL, 80, 21, false, 0, false },
  { 3, 3, "F2", TYPE_CONTROL, 81, 22, false, 0, false },
  { 3, 1, "F3", TYPE_CONTROL, 82, 23, false, 0, false },
  { 3, 4, "F4", TYPE_CONTROL, 83, 24, false, 0, false },
  { 0, 10, "ENC", TYPE_CONTROL, 96, 32, false, 0, true },
  { 0, 11, "SEQ", TYPE_CONTROL, 97, 33, false, 0, true },
  { 0, 9, "KEY", TYPE_CONTROL, 98, 34, false, 0, true },
  { 0, 8, "PLAY", TYPE_CONTROL, 99, 35, false, 0, false },
  { 0, 7, "REC", TYPE_CONTROL, 100, 36, false, 0, false }
};
const int NUM_CONTROLS = sizeof(controlButtons) / sizeof(controlButtons[0]);

ButtonMap keyboardButtons[] = {
  { 2, 2, "KEY_C", TYPE_KEYBOARD, 84, 25, false, 0, false },
  { 0, 5, "KEY_C#", TYPE_KEYBOARD, 85, 37, false, 0, false },
  { 2, 3, "KEY_D", TYPE_KEYBOARD, 86, 26, false, 0, false },
  { 0, 4, "KEY_D#", TYPE_KEYBOARD, 87, 38, false, 0, false },
  { 2, 4, "KEY_E", TYPE_KEYBOARD, 88, 27, false, 0, false },
  { 2, 10, "KEY_F", TYPE_KEYBOARD, 89, 28, false, 0, false },
  { 0, 2, "KEY_F#", TYPE_KEYBOARD, 90, 39, false, 0, false },
  { 2, 6, "KEY_G", TYPE_KEYBOARD, 91, 29, false, 0, false },
  { 0, 3, "KEY_G#", TYPE_KEYBOARD, 92, 40, false, 0, false },
  { 2, 11, "KEY_A", TYPE_KEYBOARD, 93, 30, false, 0, false },
  { 0, 6, "KEY_A#", TYPE_KEYBOARD, 94, 41, false, 0, false },
  { 2, 8, "KEY_B", TYPE_KEYBOARD, 95, 31, false, 0, false }
};
const int NUM_KEYS = sizeof(keyboardButtons) / sizeof(keyboardButtons[0]);

ButtonMap* findButton(uint8_t sensor, uint8_t channel) {
  for (int i = 0; i < NUM_PADS; i++) {
    if (padButtons[i].sensor == sensor && padButtons[i].channel == channel)
      return &padButtons[i];
  }
  for (int i = 0; i < NUM_CONTROLS; i++) {
    if (controlButtons[i].sensor == sensor && controlButtons[i].channel == channel)
      return &controlButtons[i];
  }
  for (int i = 0; i < NUM_KEYS; i++) {
    if (keyboardButtons[i].sensor == sensor && keyboardButtons[i].channel == channel)
      return &keyboardButtons[i];
  }
  return nullptr;
}

static void handlePadButton(ButtonMap* button, bool pressed) {
  button->isPressed = pressed;
  if (pressed) {
    button->pressTime = millis();
    Serial.print(button->name);
    Serial.println(" pressed");
    Control_Surface.sendNoteOn({button->midiNote, Channel_1}, 127);
    setLED(button->ledIndex, true);
  } else {
    Serial.print(button->name);
    Serial.println(" released");
    Control_Surface.sendNoteOff({button->midiNote, Channel_1}, 0);
    setLED(button->ledIndex, false);
  }
}

static void handleControlButton(ButtonMap* button, bool pressed) {
  button->isPressed = pressed;
  if (pressed) {
    Serial.print(button->name);
    Serial.println(" pressed");
    Control_Surface.sendNoteOn({button->midiNote, Channel_1}, 127);
    setLED(button->ledIndex, true);
  } else {
    Serial.print(button->name);
    Serial.println(" released");
    Control_Surface.sendNoteOff({button->midiNote, Channel_1}, 0);
    setLED(button->ledIndex, false);
  }
}

static void handleKeyboardButton(ButtonMap* button, bool pressed) {
  button->isPressed = pressed;

  Channel channel = Channel_2;
  uint8_t note = button->midiNote + (keyboardOctave - 6) * 12;

  if (pressed) {
    Serial.print(button->name);
    Serial.println(" pressed");
    Control_Surface.sendNoteOn({note, channel}, 100);
    setLED(button->ledIndex, true);
  } else {
    Serial.print(button->name);
    Serial.println(" released");
    Control_Surface.sendNoteOff({note, channel}, 0);
    setLED(button->ledIndex, false);
  }
}

void retriggerHeldNotes(uint8_t newOctave) {
  Channel channel = Channel_2;
  for (int i = 0; i < NUM_KEYS; i++) {
    if (keyboardButtons[i].isPressed) {
      uint8_t oldNote = keyboardButtons[i].midiNote + (keyboardOctave - 6) * 12;
      uint8_t newNote = keyboardButtons[i].midiNote + (newOctave - 6) * 12;
      Control_Surface.sendNoteOff({oldNote, channel}, 0);
      Control_Surface.sendNoteOn({newNote, channel}, 100);
    }
  }
}

void handleTouchEvent(uint8_t sensorIndex, uint8_t channel, bool touched) {
  ButtonMap* button = findButton(sensorIndex, channel);
  if (!button) return;

  switch (button->type) {
    case TYPE_PAD: handlePadButton(button, touched); break;
    case TYPE_CONTROL: handleControlButton(button, touched); break;
    case TYPE_KEYBOARD: handleKeyboardButton(button, touched); break;
  }
}

void handleGestureEvent(uint8_t sensorIndex, uint8_t channel, const char* gestureName, unsigned long duration) {
  // Not used in minimal version
}
