#pragma once

#include "Globals.h"

#define MIDI_CLOCK_PULSE   0xF8
#define MIDI_CLOCK_START   0xFA
#define MIDI_CLOCK_CONTINUE 0xFB
#define MIDI_CLOCK_STOP    0xFC
#define PULSES_PER_STEP    6

class ChordTimeline;  // forward declare

struct StepData {
  bool active;
  uint16_t chordMask;   // bit 0=C, bit 1=C#, ... bit 11=B
  uint8_t primaryNote;  // MIDI note of the primary (last-toggled-on) note
};

class Sequencer {
public:
  Sequencer();

  void begin();
  void update();
  void processClockByte(uint8_t byte);
  void toggleStep(uint8_t step, uint16_t chordMask = 0);
  void clearPattern();
  void togglePlayback();
  void start();
  void stop();
  void setLastNote(uint8_t note);

  // Chord editor
  void beginChordEdit(uint8_t step);
  void endChordEdit();
  bool isChordEditing() const { return chordEditStep_ >= 0; }
  uint8_t getChordEditStep() const { return chordEditStep_; }
  void toggleChordNote(uint8_t keyIndex);
  uint16_t getStepChord(uint8_t step) const { return steps_[step].chordMask; }
  void checkPendingLongPress();
  void setPendingLongPress(uint8_t step);
  void clearPendingLongPress();

  bool isPlaying() const { return playing_; }
  uint8_t getCurrentStep() const { return currentStep_; }
  bool isStepActive(uint8_t step) const { return steps_[step].active; }
  bool isCurrentStepActive() const { return steps_[currentStep_].active; }
  uint16_t getCurrentStepChord() const { return steps_[currentStep_].chordMask; }
  bool stepChanged();
  void updateStepLEDs();

  // ChordTimeline integration
  void setChordTimeline(ChordTimeline* tl) { timeline_ = tl; }
  void syncFromTimeline();  // resolve timeline → steps_[]

private:
  bool playing_;
  uint8_t currentStep_;
  StepData steps_[16];
  uint8_t clockPulseCount_;
  bool clockRunning_;
  unsigned long lastStepTime_;
  uint16_t bpm_;
  uint8_t lastNote_;
  uint8_t lastVelocity_;
  bool hasNote_;
  int8_t chordEditStep_;
  uint16_t playingMask_;
  int8_t pendingLongPressStep_;
  unsigned long pendingLongPressStart_;
  bool stepChangedFlag_;
  ChordTimeline* timeline_;  // optional, for chord-degree-based playback

  void advanceStep();
  void sendStepNotes();
  void sendStepNotesOff();
  void clearAllStepLEDs();
};

extern Sequencer sequencer;
