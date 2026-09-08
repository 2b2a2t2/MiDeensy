#include <FastLED.h>
#include <Control_Surface.h>
#include <Wire.h>
#include "src/Globals.h"
#include "src/Display.h"
#include "src/TouchHandler.h"
#include "src/Sequencer.h"
#include "src/BankHandler.h"
#include "src/NoteLED.h"
#include "src/MIDIBuffer.h"
#include "src/MPR121_GestureHelper.h"
#include "src/ChordTimeline.h"
#include "src/HarmonyEngine.h"
#include "src/VoiceManager.h"
#include "src/Quantizer.h"

// ==================== MIDI ====================

USBMIDI_Interface midi;
MIDIByteBuffer midiBuffer;
StreamMIDI_Interface serialmidi2{ midiBuffer };
BidirectionalMIDI_PipeFactory<2> pipes;

ChordTimeline chordTimeline;  // main chord timeline (up to 256 steps)
HarmonyEngine harmonyEngine;  // progression generator/evolver
VoiceManager voiceManager;    // MIDI slot/role/channel configuration
Quantizer quantizer;          // pending/quantized changes

// ==================== GLOBALS ====================

Array<CRGB, NUM_LEDS> leds{};

int8_t keyboardOctave = 3;  // 0-7, controls keyboard octave (default C3)
bool seqLEDsActive = false;  // true when sequencer owns pad LEDs
uint16_t lastHeldChord = 0;  // bitmask of last held keyboard notes
uint16_t lastChord = 0;      // last chord used in sequencer (for recalling)
uint8_t selectedSlot = 0;    // 0-15, SEQ slot being edited (selected via SEQ+PAD)
SeqFunction seqFunction = SEQ_NONE;  // current F1-F4 layer in SEQ mode
uint8_t currentKey = 60;     // root MIDI note (0-11), default 60 (C)
ScaleType currentScale = SCALE_MAJOR;  // current scale type

Bank<16> bankKeys(1);
Bank<16> bankEnc(1);

BankMode currentBankMode = BANK_NONE;
bool modeButtonHeld = false;
BankMode lastActiveMode = BANK_NONE;

uint16_t lastEncoderValues[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
uint16_t initialEnc1Value = 0;  // captured once at boot
bool encoderMoved = false;

// Encoder pickup: baseline values captured on context entry
uint16_t encoderBaseline[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
bool encoderPickedUp[8] = { false, false, false, false, false, false, false, false };

// SEQ contextual encoder parameters
struct SeqEncoderParams {
  uint8_t duration;      // enc0: 1-16 steps
  uint8_t inversion;     // enc1: 0-2
  uint8_t tension;       // enc2: 0-127
  uint8_t extensions;    // enc3: bitmask
  uint8_t density;       // enc0 (RHYTHM): chords per loop
  uint8_t swing;         // enc1 (RHYTHM): 0-127
  uint8_t energy;        // enc0 (THEME): 0-127
  uint8_t themeTension;  // enc1 (THEME): 0-127
  uint8_t octaveOffset;  // enc0 (VOICE): -12 to +12 semitones
  uint8_t velocity;      // enc1 (VOICE): 0-127
};
SeqEncoderParams seqParams = { 8, 0, 0, 0, 4, 64, 64, 64, 0, 100 };

// ==================== ENCODERS ====================

Bankable::CCAbsoluteEncoder<16> enc0{ { bankEnc, BankType::ChangeChannel }, { 40, 39 }, { 74, Channel_1 }, 7 };
Bankable::CCAbsoluteEncoder<16> enc1{ { bankEnc, BankType::ChangeChannel }, { 36, 35 }, { 71, Channel_1 }, 7 };
Bankable::CCAbsoluteEncoder<16> enc2{ { bankEnc, BankType::ChangeChannel }, { 34, 33 }, { 75, Channel_1 }, 7 };
Bankable::CCAbsoluteEncoder<16> enc3{ { bankEnc, BankType::ChangeChannel }, { 31, 32 }, { 76, Channel_1 }, 7 };
Bankable::CCAbsoluteEncoder<16> enc4{ { bankEnc, BankType::ChangeChannel }, { 38, 37 }, { 91, Channel_1 }, 7 };
Bankable::CCAbsoluteEncoder<16> enc5{ { bankEnc, BankType::ChangeChannel }, { 26, 25 }, { 92, Channel_1 }, 7 };
Bankable::CCAbsoluteEncoder<16> enc6{ { bankEnc, BankType::ChangeChannel }, { 27, 28 }, { 94, Channel_1 }, 7 };
Bankable::CCAbsoluteEncoder<16> enc7{ { bankEnc, BankType::ChangeChannel }, { 29, 30 }, { 7, Channel_1 }, 7 };

// ==================== TOUCH SENSOR ====================

MPR121_GestureHelper gestureHelper;

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("MIDI Controller Starting...");

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds.data, leds.length);
  FastLED.setCorrection(TypicalPixelString);
  FastLED.setBrightness(128);
  FastLED.clear();
  FastLED.show();

  RelativeCCSender::setMode(MACKIE_CONTROL_RELATIVE);

  Control_Surface | pipes | midi;
  Control_Surface | pipes | serialmidi2;
  Control_Surface.begin();

  Wire.begin();

  display.begin();
  display.drawBackground();
  display.displayNormalMode();

  gestureHelper.begin(300, 200, 10);
  if (!gestureHelper.addSensor(0x5A)) Serial.println("Sensor 0x5A failed");
  if (!gestureHelper.addSensor(0x5B)) Serial.println("Sensor 0x5B failed");
  if (!gestureHelper.addSensor(0x5C)) Serial.println("Sensor 0x5C failed");
  if (!gestureHelper.addSensor(0x5D)) Serial.println("Sensor 0x5D failed");

  for (int i = 0; i < 4; i++) {
    gestureHelper.setThresholds(i, 40, 20);
  }

  gestureHelper.onTouchEvent(handleTouchEvent);
  gestureHelper.onGesture(handleGestureEvent);

  // Connect ChordTimeline to sequencer
  sequencer.setChordTimeline(&chordTimeline);

  Serial.println("\n=== System Ready ===");
  Serial.println("Mode buttons: KEY, ENC, SEQ");
  Serial.println("Hold mode button + tap pads 1-16 to select bank");
  Serial.println("SEQ mode: Pads toggle steps, PLAY toggles playback");
  Serial.println("====================\n");

  // Capture initial encoder value so default octave sticks on boot
  initialEnc1Value = enc1.getValue();
}

// ==================== LOOP ====================

void loop() {
  // Read Serial2 BEFORE Control_Surface: intercept clock for sequencer,
  // buffer everything else for Control_Surface to process
  while (Serial2.available()) {
    uint8_t b = Serial2.read();
    if (b == MIDI_CLOCK_PULSE || b == MIDI_CLOCK_START ||
        b == MIDI_CLOCK_CONTINUE || b == MIDI_CLOCK_STOP) {
      sequencer.processClockByte(b);
    } else {
      midiBuffer.write(b);
    }
  }

  Control_Surface.loop();
  gestureHelper.update();

  sequencer.update();
  sequencer.checkPendingLongPress();

  // Process quantized changes at step boundaries
  if (quantizer.hasPending() && sequencer.stepChanged()) {
    PendingChange change = quantizer.consume();
    switch (change.type) {
      case PENDING_KEY_CHANGE:
        currentKey = change.data.keyScale.key;
        currentScale = (ScaleType)change.data.keyScale.scale;
        Serial.print("Key changed to ");
        Serial.print(currentKey);
        Serial.print(" scale ");
        Serial.println(currentScale);
        break;
      case PENDING_DENSITY_CHANGE:
        // Density change applied to next generation
        Serial.print("Density queued: ");
        Serial.println(change.data.density.density);
        break;
      case PENDING_CHORD_CHANGE:
        // Chord change applied to timeline
        Serial.print("Chord change queued for step ");
        Serial.println(change.data.chord.step);
        break;
      default:
        break;
    }
  }

  // Update encoders
  enc0.update(); enc1.update(); enc2.update(); enc3.update();
  enc4.update(); enc5.update(); enc6.update(); enc7.update();

  uint16_t currentValues[8] = {
    enc0.getValue(), enc1.getValue(), enc2.getValue(), enc3.getValue(),
    enc4.getValue(), enc5.getValue(), enc6.getValue(), enc7.getValue()
  };

  // Capture baseline when entering a new SEQ context
  static SeqFunction lastSeqFunction = SEQ_NONE;
  if (currentBankMode == BANK_SEQ && seqFunction != lastSeqFunction) {
    for (int i = 0; i < 8; i++) {
      encoderBaseline[i] = currentValues[i];
      encoderPickedUp[i] = false;
    }
    lastSeqFunction = seqFunction;
  }
  if (currentBankMode != BANK_SEQ) {
    lastSeqFunction = SEQ_NONE;
  }

  // Context-sensitive encoder routing in BANK_SEQ
  if (currentBankMode == BANK_SEQ) {
    switch (seqFunction) {
      case SEQ_HARMONY:
        // enc0 → duration (1-16)
        if (encoderPickedUp[0] || currentValues[0] != encoderBaseline[0]) {
          encoderPickedUp[0] = true;
          seqParams.duration = map(currentValues[0], 0, 127, 1, 16);
        }
        // enc1 → inversion (0-2)
        if (encoderPickedUp[1] || currentValues[1] != encoderBaseline[1]) {
          encoderPickedUp[1] = true;
          seqParams.inversion = map(currentValues[1], 0, 127, 0, 2);
        }
        // enc2 → tension (0-127)
        if (encoderPickedUp[2] || currentValues[2] != encoderBaseline[2]) {
          encoderPickedUp[2] = true;
          seqParams.tension = currentValues[2];
        }
        // enc3 → extensions (bitmask)
        if (encoderPickedUp[3] || currentValues[3] != encoderBaseline[3]) {
          encoderPickedUp[3] = true;
          seqParams.extensions = map(currentValues[3], 0, 127, 0, 15);
        }
        break;

      case SEQ_RHYTHM:
        // enc0 → density (1-16)
        if (encoderPickedUp[0] || currentValues[0] != encoderBaseline[0]) {
          encoderPickedUp[0] = true;
          seqParams.density = map(currentValues[0], 0, 127, 1, 16);
        }
        // enc1 → swing (0-127)
        if (encoderPickedUp[1] || currentValues[1] != encoderBaseline[1]) {
          encoderPickedUp[1] = true;
          seqParams.swing = currentValues[1];
        }
        break;

      case SEQ_THEME:
        // enc0 → energy (0-127)
        if (encoderPickedUp[0] || currentValues[0] != encoderBaseline[0]) {
          encoderPickedUp[0] = true;
          seqParams.energy = currentValues[0];
        }
        // enc1 → tension (0-127)
        if (encoderPickedUp[1] || currentValues[1] != encoderBaseline[1]) {
          encoderPickedUp[1] = true;
          seqParams.themeTension = currentValues[1];
        }
        break;

      case SEQ_VOICE:
        // enc0 → octave offset (-12 to +12)
        if (encoderPickedUp[0] || currentValues[0] != encoderBaseline[0]) {
          encoderPickedUp[0] = true;
          seqParams.octaveOffset = map(currentValues[0], 0, 127, -12, 12);
        }
        // enc1 → velocity (0-127)
        if (encoderPickedUp[1] || currentValues[1] != encoderBaseline[1]) {
          encoderPickedUp[1] = true;
          seqParams.velocity = currentValues[1];
        }
        break;

      case SEQ_NONE:
      default:
        break;
    }
    // In BANK_SEQ with no function selected, encoders do nothing contextual
  }

  // Encoder 1 controls keyboard octave (only when NOT in BANK_SEQ)
  if (currentBankMode != BANK_SEQ) {
    int8_t newOctave = map(enc1.getValue(), 0, 127, 0, 7);
    if (!encoderMoved) {
      if (enc1.getValue() != initialEnc1Value) {
        encoderMoved = true;
      }
    }
    if (encoderMoved && newOctave != keyboardOctave) {
      retriggerHeldNotes(newOctave);
      keyboardOctave = newOctave;
    }
  }

  for (int i = 0; i < 8; i++) {
    lastEncoderValues[i] = currentValues[i];
  }

  // Redraw header every 200ms to keep octave display current
  static unsigned long lastDisplayUpdate = 0;
  unsigned long now = millis();
  if (now - lastDisplayUpdate > 200) {
    display.drawBackground();
    display.display();
    lastDisplayUpdate = now;
  }

  // Update LEDs
  bool forceLED = sequencer.stepChanged();
  if (midiled.getDirty() || currentBankMode == BANK_SEQ || forceLED) {
    if (sequencer.isChordEditing()) {
      // Chord edit: show the step's chord on keyboard LEDs
      uint16_t chord = sequencer.getStepChord(sequencer.getChordEditStep());
      for (int i = 0; i < 12; i++) {
        uint8_t ledIdx = ledMapping[25 + i];
        if (chord & (1 << i)) {
          leds[ledIdx] = CRGB::Cyan;
        } else {
          leds[ledIdx] = CRGB::Black;
        }
      }
    } else if (sequencer.isPlaying() && sequencer.isCurrentStepActive()) {
      // Playing: show current step's chord on keyboard LEDs
      uint16_t chord = sequencer.getCurrentStepChord();
      for (int i = 0; i < 12; i++) {
        uint8_t ledIdx = ledMapping[25 + i];
        if (chord & (1 << i)) {
          leds[ledIdx] = CRGB(0, 50, 50);
        } else {
          leds[ledIdx] = CRGB::Black;
        }
      }
    } else if (sequencer.isPlaying()) {
      // Playing but current step is inactive: clear keyboard LEDs
      for (int i = 0; i < 12; i++) {
        leds[ledMapping[25 + i]] = CRGB::Black;
      }
    }
    FastLED.show();
    midiled.clearDirty();
  }
}
