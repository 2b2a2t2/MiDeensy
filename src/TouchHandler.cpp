#include "TouchHandler.h"
#include "BankHandler.h"
#include "Sequencer.h"
#include "HarmonyEngine.h"
#include "ChordTimeline.h"
#include "VoiceManager.h"

extern uint8_t selectedSlot;
extern SeqFunction seqFunction;
extern uint8_t currentKey;
extern ScaleType currentScale;
extern ChordTimeline chordTimeline;
extern VoiceManager voiceManager;

// 16 PADS
ButtonMap padButtons[] = {
  { 1, 1, "PAD1", TYPE_PAD, 59, false, 0, false },
  { 1, 4, "PAD2", TYPE_PAD, 60, false, 0, false },
  { 1, 2, "PAD3", TYPE_PAD, 61, false, 0, false },
  { 3, 10, "PAD4", TYPE_PAD, 62, false, 0, false },
  { 1, 0, "PAD5", TYPE_PAD, 63, false, 0, false },
  { 3, 8, "PAD6", TYPE_PAD, 64, false, 0, false },
  { 3, 2, "PAD7", TYPE_PAD, 65, false, 0, false },
  { 2, 0, "PAD8", TYPE_PAD, 66, false, 0, false },
  { 3, 0, "PAD9", TYPE_PAD, 67, false, 0, false },
  { 2, 5, "PAD10", TYPE_PAD, 68, false, 0, false },
  { 3, 5, "PAD11", TYPE_PAD, 69, false, 0, false },
  { 2, 1, "PAD12", TYPE_PAD, 70, false, 0, false },
  { 0, 0, "PAD13", TYPE_PAD, 71, false, 0, false },
  { 2, 7, "PAD14", TYPE_PAD, 72, false, 0, false },
  { 2, 9, "PAD15", TYPE_PAD, 73, false, 0, false },
  { 0, 1, "PAD16", TYPE_PAD, 74, false, 0, false }
};
const int NUM_PADS = sizeof(padButtons) / sizeof(padButtons[0]);

// CONTROLS
ButtonMap controlButtons[] = {
  { 3, 9, "PREV", TYPE_CONTROL, 75, false, 0, false },
  { 1, 5, "NEXT", TYPE_CONTROL, 76, false, 0, false },
  { 1, 3, "M1", TYPE_CONTROL, 77, false, 0, false },
  { 3, 6, "M2", TYPE_CONTROL, 78, false, 0, false },
  { 3, 11, "M3", TYPE_CONTROL, 79, false, 0, false },
  { 3, 7, "F1", TYPE_CONTROL, 80, false, 0, false },
  { 3, 3, "F2", TYPE_CONTROL, 81, false, 0, false },
  { 3, 1, "F3", TYPE_CONTROL, 82, false, 0, false },
  { 3, 4, "F4", TYPE_CONTROL, 83, false, 0, false },
  { 0, 10, "ENC", TYPE_CONTROL, 96, false, 0, true },
  { 0, 11, "SEQ", TYPE_CONTROL, 97, false, 0, true },
  { 0, 9, "KEY", TYPE_CONTROL, 98, false, 0, true },
  { 0, 8, "PLAY", TYPE_CONTROL, 99, false, 0, false },
  { 0, 7, "REC", TYPE_CONTROL, 100, false, 0, false }
};
const int NUM_CONTROLS = sizeof(controlButtons) / sizeof(controlButtons[0]);

// KEYBOARD
ButtonMap keyboardButtons[] = {
  { 2, 2, "KEY_C", TYPE_KEYBOARD, 84, false, 0, false },
  { 0, 5, "KEY_C#", TYPE_KEYBOARD, 85, false, 0, false },
  { 2, 3, "KEY_D", TYPE_KEYBOARD, 86, false, 0, false },
  { 0, 4, "KEY_D#", TYPE_KEYBOARD, 87, false, 0, false },
  { 2, 4, "KEY_E", TYPE_KEYBOARD, 88, false, 0, false },
  { 2, 10, "KEY_F", TYPE_KEYBOARD, 89, false, 0, false },
  { 0, 2, "KEY_F#", TYPE_KEYBOARD, 90, false, 0, false },
  { 2, 6, "KEY_G", TYPE_KEYBOARD, 91, false, 0, false },
  { 0, 3, "KEY_G#", TYPE_KEYBOARD, 92, false, 0, false },
  { 2, 11, "KEY_A", TYPE_KEYBOARD, 93, false, 0, false },
  { 0, 6, "KEY_A#", TYPE_KEYBOARD, 94, false, 0, false },
  { 2, 8, "KEY_B", TYPE_KEYBOARD, 95, false, 0, false }
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

  static bool stepToggledOnPress = false;

  if (pressed) {
    button->pressTime = millis();
    Serial.print(button->name);
    Serial.println(" pressed");

    if (currentBankMode == BANK_SEQ) {
      int padIndex = button->midiNote - PAD_NOTE_BASE;

      // HOLD SEQ + PAD → select active slot
      if (modeButtonHeld) {
        selectedSlot = padIndex;
        voiceManager.setActiveSlot(padIndex);
        Serial.print("Selected slot ");
        Serial.print(selectedSlot);
        Serial.print(" (ch ");
        Serial.print(voiceManager.getActiveChannel());
        Serial.print(", role ");
        Serial.println(voiceManager.getActiveRole());
        BankLEDHandler::updateBankLEDs();
        return;
      }

      stepToggledOnPress = false;
      if (sequencer.isStepActive(padIndex)) {
        // Step is ON: start long-press timer
        sequencer.setPendingLongPress(padIndex);
      } else {
        // Step is OFF: turn on with last held chord
        sequencer.toggleStep(padIndex, lastHeldChord);
        stepToggledOnPress = true;
      }
      return;
    }

    if (currentBankMode == BANK_NONE) {
      Control_Surface.sendNoteOn({button->midiNote, Channel_1}, 127);
    }

    if (modeButtonHeld) {
      int padIndex = button->midiNote - PAD_NOTE_BASE;
      if (currentBankMode == BANK_KEYS) {
        bankKeys.select(padIndex);
      } else if (currentBankMode == BANK_ENC) {
        bankEnc.select(padIndex);
      }
      BankLEDHandler::updateBankLEDs();
    }
  } else {
    Serial.print(button->name);
    Serial.println(" released");

    if (currentBankMode == BANK_SEQ) {
      if (sequencer.isChordEditing()) {
        // Was in chord edit: exit on release
        sequencer.endChordEdit();
      } else if (!stepToggledOnPress && sequencer.isStepActive(button->midiNote - PAD_NOTE_BASE)) {
        // Step was already active and we didn't toggle it on this press
        // Short press → toggle off
        unsigned long elapsed = millis() - button->pressTime;
        if (elapsed < 300) {
          sequencer.toggleStep(button->midiNote - PAD_NOTE_BASE, 0);
        }
        sequencer.clearPendingLongPress();
      }
      return;
    }
    if (currentBankMode == BANK_NONE) {
      Control_Surface.sendNoteOff({button->midiNote, Channel_1}, 0);
    }
  }
}

static void handleControlButton(ButtonMap* button, bool pressed) {
  button->isPressed = pressed;

  if (pressed) {
    Serial.print(button->name);
    Serial.println(" pressed");

    if (strcmp(button->name, "ENC") == 0) {
      BankLEDHandler::enterBankMode(BANK_ENC);
    } else if (strcmp(button->name, "KEY") == 0) {
      BankLEDHandler::enterBankMode(BANK_KEYS);
    } else if (strcmp(button->name, "SEQ") == 0) {
      if (currentBankMode == BANK_SEQ) {
        BankLEDHandler::exitBankMode();
      } else {
        BankLEDHandler::enterBankMode(BANK_SEQ);
        if (!sequencer.isPlaying()) {
          sequencer.begin();
        }
      }
    } else if (strcmp(button->name, "PLAY") == 0 && currentBankMode == BANK_SEQ) {
      sequencer.togglePlayback();
    } else if (strcmp(button->name, "REC") == 0 && currentBankMode == BANK_SEQ) {
      sequencer.clearPattern();
    } else if (currentBankMode == BANK_SEQ) {
      // F1-F4: set SEQ function layer
      if (strcmp(button->name, "F1") == 0) {
        seqFunction = SEQ_THEME;
        Serial.println("SEQ function: THEME");
      } else if (strcmp(button->name, "F2") == 0) {
        seqFunction = SEQ_HARMONY;
        Serial.println("SEQ function: HARMONY");
      } else if (strcmp(button->name, "F3") == 0) {
        seqFunction = SEQ_RHYTHM;
        Serial.println("SEQ function: RHYTHM");
      } else if (strcmp(button->name, "F4") == 0) {
        seqFunction = SEQ_VOICE;
        Serial.println("SEQ function: VOICE");
      }
      // M1-M3: immediate actions
      else if (strcmp(button->name, "M1") == 0) {
        Serial.print("NEW in context ");
        Serial.println(seqFunction);
        if (seqFunction == SEQ_HARMONY) {
          extern HarmonyEngine harmonyEngine;
          HarmonyParams params;
          params.key = currentKey;
          params.scale = currentScale;
          params.density = 4;  // default 4 chords per loop
          params.tension = 64;
          params.variation = 64;
          params.loopLength = 16;
          harmonyEngine.generate(chordTimeline, params);
          sequencer.syncFromTimeline();
          Serial.println("Harmony: NEW generated");
        }
      } else if (strcmp(button->name, "M2") == 0) {
        Serial.print("EVOLVE in context ");
        Serial.println(seqFunction);
        if (seqFunction == SEQ_HARMONY) {
          extern HarmonyEngine harmonyEngine;
          HarmonyParams params;
          params.key = currentKey;
          params.scale = currentScale;
          params.density = 4;
          params.tension = 64;
          params.variation = 64;
          params.loopLength = 16;
          HarmonyLock locks;
          locks.progression = false;
          locks.density = false;
          locks.tension = false;
          harmonyEngine.evolve(chordTimeline, params, locks);
          sequencer.syncFromTimeline();
          Serial.println("Harmony: EVOLVED");
        }
      } else if (strcmp(button->name, "M3") == 0) {
        Serial.print("LOCK in context ");
        Serial.println(seqFunction);
        // TODO Phase 6: toggle lock for current context
      }
    } else {
      if (currentBankMode == BANK_NONE) {
        Control_Surface.sendNoteOn({button->midiNote, Channel_1}, 127);
      }
    }
  } else {
    Serial.print(button->name);
    Serial.println(" released");

    if (currentBankMode == BANK_NONE) {
      Control_Surface.sendNoteOff({button->midiNote, Channel_1}, 0);
    }

    // Mode buttons: only exit if NOT in SEQ mode (SEQ is sticky)
    if (button->isModeButton && strcmp(button->name, "SEQ") != 0) {
      modeButtonHeld = false;
      for (int i = 0; i < NUM_CONTROLS; i++) {
        if (controlButtons[i].isModeButton && strcmp(controlButtons[i].name, "SEQ") != 0 && controlButtons[i].isPressed) {
          modeButtonHeld = true;
          break;
        }
      }
      if (!modeButtonHeld) {
        BankLEDHandler::exitBankMode();
      }
    }
  }
}

static void handleKeyboardButton(ButtonMap* button, bool pressed) {
  button->isPressed = pressed;

  // Update lastHeldChord: bitmask of currently pressed keyboard keys
  lastHeldChord = 0;
  for (int i = 0; i < NUM_KEYS; i++) {
    if (keyboardButtons[i].isPressed) {
      lastHeldChord |= (1 << i);
    }
  }

  Channel channel = Channel_2;
  uint8_t bankOffset = bankKeys.getSelection();
  if (bankOffset > 0) {
    channel = Channel_2 + bankOffset;
  }

  // Find which key index (0-11) this button corresponds to
  int keyIndex = -1;
  for (int i = 0; i < NUM_KEYS; i++) {
    if (&keyboardButtons[i] == button) {
      keyIndex = i;
      break;
    }
  }

  uint8_t note = button->midiNote + (keyboardOctave - 6) * 12;

  if (pressed) {
    // Chord edit mode: toggle note in the chord
    if (sequencer.isChordEditing() && keyIndex >= 0) {
      sequencer.toggleChordNote(keyIndex);
      return;
    }

    Serial.print(button->name);
    Serial.print(" pressed octave=");
    Serial.println(keyboardOctave);
    Control_Surface.sendNoteOn({note, channel}, 100);
    if (currentBankMode == BANK_SEQ) {
      sequencer.setLastNote(note);
    }
  } else {
    if (sequencer.isChordEditing()) return;

    Serial.print(button->name);
    Serial.println(" released");
    Control_Surface.sendNoteOff({note, channel}, 0);
  }
}

void retriggerHeldNotes(uint8_t newOctave) {
  if (sequencer.isChordEditing()) return;

  Channel channel = Channel_2;
  uint8_t bankOffset = bankKeys.getSelection();
  if (bankOffset > 0) {
    channel = Channel_2 + bankOffset;
  }

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
  ButtonMap* button = findButton(sensorIndex, channel);
  if (!button) return;

  if (strcmp(gestureName, "double_tap") == 0) {
    Serial.print(button->name);
    Serial.println(" double_tap");
    if (button->type == TYPE_PAD && currentBankMode == BANK_NONE) {
      Control_Surface.sendNoteOn({button->midiNote, Channel_1}, 100);
    }
  } else if (strcmp(gestureName, "long_press") == 0) {
    Serial.print(button->name);
    Serial.println(" long_press");
    if (button->type == TYPE_PAD && currentBankMode == BANK_NONE) {
      Control_Surface.sendControlChange({button->midiNote, Channel_1}, 127);
    }
  }
}
