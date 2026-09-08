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
int16_t initialEnc1Value = 0;  // captured once at boot
bool encoderMoved = false;

// Encoder pickup: baseline values captured on context entry
uint16_t encoderBaseline[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
bool encoderPickedUp[8] = { false, false, false, false, false, false, false, false };

// SEQ contextual encoder parameters (struct defined in Globals.h)
SeqEncoderParams seqParams = { 8, 0, 0, 0, 4, 64, 64, 64, 0, 100 };

// ==================== HELPERS ====================

// Get or create a ChordEvent starting exactly at the given step.
// If no event starts at that step, creates one with DEGREE_1, quality AUTO, duration 4.
ChordEvent getOrCreateEventStartingAtStep(uint16_t step) {
  const ChordEvent* existing = chordTimeline.getEventStartingAtStep(step);
  if (existing) {
    ChordEvent copy = *existing;
    return copy;
  }
  // No event starts here — create a default one
  ChordEvent evt;
  evt.active = true;
  evt.startStep = step;
  evt.duration = 4;
  evt.degree = DEGREE_1;
  evt.quality = QUALITY_AUTO;
  evt.inversion = 0;
  evt.extensions = 0;
  evt.tension = 0;
  evt.velocity = 100;
  return evt;
}

// Apply a modified ChordEvent back to the timeline and sync to sequencer.
// Uses updateEvent if an event exists at startStep, otherwise addEvent.
void applyChordEvent(const ChordEvent& evt) {
  if (!chordTimeline.updateEvent(evt.startStep, evt)) {
    chordTimeline.addEvent(evt);
  }
  sequencer.syncFromTimeline();
}

// ==================== ENCODERS ====================

// Gate flag: encoders only send CC when this is true (BANK_ENC mode)
bool encoderCCGate = false;

// Custom sender that wraps ContinuousCCSender with a gate flag
struct GatedCCSender {
  void send(uint8_t value, MIDIAddress address) {
    if (encoderCCGate)
      Control_Surface.sendControlChange(address, value);
  }
  constexpr static uint8_t precision() { return 7; }
};

// Physical encoder hardware — AHEncoder objects (one per encoder)
AHEncoder ahe0{40, 39}, ahe1{36, 35}, ahe2{34, 33}, ahe3{31, 32};
AHEncoder ahe4{38, 37}, ahe5{26, 25}, ahe6{27, 28}, ahe7{29, 30};

// Borrowed encoders: always track position, but CC output gated by encoderCCGate
cs::OutputBankConfig<BankType::ChangeChannel> encBankCfg{bankEnc, BankType::ChangeChannel};
cs::Bankable::BorrowedMIDIAbsoluteEncoder<16, cs::Bankable::SingleAddress, GatedCCSender> midiEnc0{ cs::Bankable::SingleAddress(encBankCfg, {74, Channel_1}), ahe0, 7, 4, {} };
cs::Bankable::BorrowedMIDIAbsoluteEncoder<16, cs::Bankable::SingleAddress, GatedCCSender> midiEnc1{ cs::Bankable::SingleAddress(encBankCfg, {71, Channel_1}), ahe1, 7, 4, {} };
cs::Bankable::BorrowedMIDIAbsoluteEncoder<16, cs::Bankable::SingleAddress, GatedCCSender> midiEnc2{ cs::Bankable::SingleAddress(encBankCfg, {75, Channel_1}), ahe2, 7, 4, {} };
cs::Bankable::BorrowedMIDIAbsoluteEncoder<16, cs::Bankable::SingleAddress, GatedCCSender> midiEnc3{ cs::Bankable::SingleAddress(encBankCfg, {76, Channel_1}), ahe3, 7, 4, {} };
cs::Bankable::BorrowedMIDIAbsoluteEncoder<16, cs::Bankable::SingleAddress, GatedCCSender> midiEnc4{ cs::Bankable::SingleAddress(encBankCfg, {91, Channel_1}), ahe4, 7, 4, {} };
cs::Bankable::BorrowedMIDIAbsoluteEncoder<16, cs::Bankable::SingleAddress, GatedCCSender> midiEnc5{ cs::Bankable::SingleAddress(encBankCfg, {92, Channel_1}), ahe5, 7, 4, {} };
cs::Bankable::BorrowedMIDIAbsoluteEncoder<16, cs::Bankable::SingleAddress, GatedCCSender> midiEnc6{ cs::Bankable::SingleAddress(encBankCfg, {94, Channel_1}), ahe6, 7, 4, {} };
cs::Bankable::BorrowedMIDIAbsoluteEncoder<16, cs::Bankable::SingleAddress, GatedCCSender> midiEnc7{ cs::Bankable::SingleAddress(encBankCfg, {7, Channel_1}), ahe7, 7, 4, {} };

cs::Bankable::BorrowedMIDIAbsoluteEncoder<16, cs::Bankable::SingleAddress, GatedCCSender>* midiEncoders[8] = {
  &midiEnc0, &midiEnc1, &midiEnc2, &midiEnc3,
  &midiEnc4, &midiEnc5, &midiEnc6, &midiEnc7
};

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
  initialEnc1Value = midiEncoders[1]->getValue();
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

  // Set encoder CC gate BEFORE loop: encoders send CC during Control_Surface.loop()
  encoderCCGate = (currentBankMode == BANK_ENC);

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

  // Read encoder values (always updated by Control_Surface.loop())
  uint16_t currentValues[8];
  for (int i = 0; i < 8; i++) {
    currentValues[i] = midiEncoders[i]->getValue();
  }

  // Capture baseline when entering a new SEQ context
  static SeqFunction lastSeqFunction = SEQ_NONE;
  if (currentBankMode == BANK_SEQ && seqFunction != lastSeqFunction) {
    for (int i = 0; i < 8; i++) {
      encoderBaseline[i] = currentValues[i];
      encoderPickedUp[i] = false;
    }
    // Initialize seqParams from the selected step's timeline event
    if (seqFunction == SEQ_HARMONY) {
      uint8_t selStep = sequencer.getSelectedStep();
      const ChordEvent* evt = chordTimeline.getEventStartingAtStep(selStep);
      if (evt) {
        seqParams.duration = evt->duration;
        seqParams.inversion = evt->inversion;
        seqParams.tension = evt->tension;
        seqParams.extensions = evt->extensions;
      }
    }
    lastSeqFunction = seqFunction;
  }
  if (currentBankMode != BANK_SEQ) {
    lastSeqFunction = SEQ_NONE;
  }

  // Context-sensitive encoder routing in BANK_SEQ
  if (currentBankMode == BANK_SEQ) {
    uint8_t selStep = sequencer.getSelectedStep();

    switch (seqFunction) {
      case SEQ_HARMONY: {
        // enc0 → duration, enc1 → inversion, enc2 → tension, enc3 → extensions
        // These modify the ChordEvent whose startStep == selectedStep
        bool changed = false;
        ChordEvent evt = getOrCreateEventStartingAtStep(selStep);

        if (encoderPickedUp[0] || currentValues[0] != encoderBaseline[0]) {
          encoderPickedUp[0] = true;
          uint8_t newDur = map(currentValues[0], 0, 127, 1, 16);
          if (evt.duration != newDur) { evt.duration = newDur; changed = true; }
        }
        if (encoderPickedUp[1] || currentValues[1] != encoderBaseline[1]) {
          encoderPickedUp[1] = true;
          uint8_t newInv = map(currentValues[1], 0, 127, 0, 2);
          if (evt.inversion != newInv) { evt.inversion = newInv; changed = true; }
        }
        if (encoderPickedUp[2] || currentValues[2] != encoderBaseline[2]) {
          encoderPickedUp[2] = true;
          if (evt.tension != currentValues[2]) { evt.tension = currentValues[2]; changed = true; }
        }
        if (encoderPickedUp[3] || currentValues[3] != encoderBaseline[3]) {
          encoderPickedUp[3] = true;
          uint8_t newExt = map(currentValues[3], 0, 127, 0, 15);
          if (evt.extensions != newExt) { evt.extensions = newExt; changed = true; }
        }

        if (changed) {
          applyChordEvent(evt);
          seqParams.duration = evt.duration;
          seqParams.inversion = evt.inversion;
          seqParams.tension = evt.tension;
          seqParams.extensions = evt.extensions;
        }
        break;
      }

      case SEQ_RHYTHM:
        // enc0 → density (global generation parameter)
        if (encoderPickedUp[0] || currentValues[0] != encoderBaseline[0]) {
          encoderPickedUp[0] = true;
          seqParams.density = map(currentValues[0], 0, 127, 1, 16);
        }
        // enc1 → swing (stored for future rhythm engine)
        if (encoderPickedUp[1] || currentValues[1] != encoderBaseline[1]) {
          encoderPickedUp[1] = true;
          seqParams.swing = currentValues[1];
        }
        break;

      case SEQ_THEME:
        // enc0 → energy, enc1 → themeTension (feed into HarmonyParams for NEW/EVOLVE)
        if (encoderPickedUp[0] || currentValues[0] != encoderBaseline[0]) {
          encoderPickedUp[0] = true;
          seqParams.energy = currentValues[0];
        }
        if (encoderPickedUp[1] || currentValues[1] != encoderBaseline[1]) {
          encoderPickedUp[1] = true;
          seqParams.themeTension = currentValues[1];
        }
        break;

      case SEQ_VOICE: {
        // enc0 → octave offset, enc1 → velocity (per-slot via VoiceManager)
        bool changed = false;
        if (encoderPickedUp[0] || currentValues[0] != encoderBaseline[0]) {
          encoderPickedUp[0] = true;
          int8_t newOct = map(currentValues[0], 0, 127, -12, 12);
          if (seqParams.octaveOffset != newOct) {
            seqParams.octaveOffset = newOct;
            voiceManager.setSlotOctaveOffset(selectedSlot, newOct);
            changed = true;
          }
        }
        if (encoderPickedUp[1] || currentValues[1] != encoderBaseline[1]) {
          encoderPickedUp[1] = true;
          if (seqParams.velocity != currentValues[1]) {
            seqParams.velocity = currentValues[1];
            voiceManager.setSlotVelocity(selectedSlot, currentValues[1]);
            changed = true;
          }
        }
        (void)changed;
        break;
      }

      case SEQ_NONE:
      default:
        break;
    }
  }

  // Encoder 1 controls keyboard octave (only in KEY mode, not SEQ or ENC)
  if (currentBankMode != BANK_SEQ && currentBankMode != BANK_ENC) {
    int8_t newOctave = map(midiEncoders[1]->getValue(), 0, 127, 0, 7);
    if (!encoderMoved) {
      if (midiEncoders[1]->getValue() != initialEnc1Value) {
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
