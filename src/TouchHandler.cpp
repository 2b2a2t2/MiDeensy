#include "TouchHandler.h"
#include "NoteLED.h"
#include "MPR121_GestureHelper.h"
#include <usb_midi.h>

extern int8_t keyboardOctave;

static MPR121_GestureHelper gestureHelper;

static void handlePadButton(const SensorMap* btn, bool pressed) {
  if (pressed) {
    Serial.print(btn->name);
    Serial.println(" pressed");
    usbMIDI.sendNoteOn(59 + btn->ledIndex, 127, 1);
    setLED(btn->ledIndex, true);
  } else {
    Serial.print(btn->name);
    Serial.println(" released");
    usbMIDI.sendNoteOff(59 + btn->ledIndex, 0, 1);
    setLED(btn->ledIndex, false);
  }
}

static void handleControlButton(const SensorMap* btn, bool pressed) {
  if (pressed) {
    Serial.print(btn->name);
    Serial.println(" pressed");
    usbMIDI.sendNoteOn(btn->ledIndex, 127, 1);
    setLED(btn->ledIndex, true);
  } else {
    Serial.print(btn->name);
    Serial.println(" released");
    usbMIDI.sendNoteOff(btn->ledIndex, 0, 1);
    setLED(btn->ledIndex, false);
  }
}

static void handleKeyboardButton(const SensorMap* btn, bool pressed) {
  uint8_t note = 84 + (btn->ledIndex - 25) + (keyboardOctave - 6) * 12;

  if (pressed) {
    Serial.print(btn->name);
    Serial.println(" pressed");
    usbMIDI.sendNoteOn(note, 100, 2);
    setLED(btn->ledIndex, true);
  } else {
    Serial.print(btn->name);
    Serial.println(" released");
    usbMIDI.sendNoteOff(note, 0, 2);
    setLED(btn->ledIndex, false);
  }
}

void retriggerHeldNotes(uint8_t newOctave) {
}

void touchBegin() {
  gestureHelper.begin(300, 200, 10);
  gestureHelper.addSensor(0x5A);
  gestureHelper.addSensor(0x5B);
  gestureHelper.addSensor(0x5C);
  gestureHelper.addSensor(0x5D);
  for (int i = 0; i < 4; i++) gestureHelper.setThresholds(i, 40, 20);
  gestureHelper.onTouchEvent(handleTouchEvent);
  gestureHelper.onGesture(handleGestureEvent);
}

void touchUpdate() {
  gestureHelper.update();
}

void handleTouchEvent(uint8_t sensorIndex, uint8_t channel, bool touched) {
  const SensorMap* btn = findButton(sensorIndex, channel);
  if (!btn) return;

  switch (btn->type) {
    case TYPE_PAD:     handlePadButton(btn, touched); break;
    case TYPE_CONTROL: handleControlButton(btn, touched); break;
    case TYPE_KEYBOARD: handleKeyboardButton(btn, touched); break;
  }
}

void handleGestureEvent(uint8_t sensorIndex, uint8_t channel, const char* gestureName, unsigned long duration) {
}
