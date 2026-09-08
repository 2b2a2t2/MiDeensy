#include "Sequencer.h"
#include "ChordTimeline.h"
#include "VoiceManager.h"

Sequencer::Sequencer()
  : playing_(false), currentStep_(0), clockPulseCount_(0),
    clockRunning_(false), lastStepTime_(0), bpm_(120),
    lastNote_(60), lastVelocity_(127), hasNote_(false),
    chordEditStep_(-1), playingMask_(0),
    pendingLongPressStep_(-1), pendingLongPressStart_(0),
    stepChangedFlag_(false), timeline_(nullptr) {
  clearPattern();
}

void Sequencer::begin() {
  playing_ = false;
  currentStep_ = 0;
  clockPulseCount_ = 0;
  clockRunning_ = false;
  lastStepTime_ = millis();
  chordEditStep_ = -1;
  seqLEDsActive = true;
  clearAllStepLEDs();
  updateStepLEDs();
}

void Sequencer::update() {
  if (!clockRunning_ && playing_) {
    unsigned long stepInterval = 60000UL / ((uint32_t)bpm_ * 4);
    unsigned long now = millis();
    if (now - lastStepTime_ >= stepInterval) {
      advanceStep();
      lastStepTime_ = now;
    }
  }
}

void Sequencer::processClockByte(uint8_t byte) {
  switch (byte) {
    case MIDI_CLOCK_PULSE:
      clockPulseCount_++;
      if (clockPulseCount_ >= PULSES_PER_STEP) {
        clockPulseCount_ = 0;
        if (playing_) {
          advanceStep();
        }
      }
      break;
    case MIDI_CLOCK_START:
      clockPulseCount_ = 0;
      clockRunning_ = true;
      playing_ = true;
      currentStep_ = 0;
      lastStepTime_ = millis();
      sendStepNotes();
      updateStepLEDs();
      break;
    case MIDI_CLOCK_CONTINUE:
      clockRunning_ = true;
      playing_ = true;
      break;
    case MIDI_CLOCK_STOP:
      clockRunning_ = false;
      playing_ = false;
      clockPulseCount_ = 0;
      sendStepNotesOff();
      updateStepLEDs();
      break;
  }
}

void Sequencer::advanceStep() {
  sendStepNotesOff();
  currentStep_ = (currentStep_ + 1) % 16;
  stepChangedFlag_ = true;
  sendStepNotes();
  updateStepLEDs();
  lastStepTime_ = millis();
}

void Sequencer::sendStepNotesOff() {
  extern VoiceManager voiceManager;
  Channel ch = Channel::createChannel(voiceManager.getActiveChannel());
  uint8_t baseNote = steps_[currentStep_].primaryNote - (steps_[currentStep_].primaryNote % 12);
  for (uint8_t i = 0; i < 12; i++) {
    if (playingMask_ & (1 << i)) {
      Control_Surface.sendNoteOff({baseNote + i, ch}, 0);
    }
  }
  playingMask_ = 0;
}

void Sequencer::sendStepNotes() {
  extern VoiceManager voiceManager;
  Channel ch = Channel::createChannel(voiceManager.getActiveChannel());

  if (!steps_[currentStep_].active) return;

  uint16_t chord = steps_[currentStep_].chordMask;
  if (chord == 0) {
    Control_Surface.sendNoteOn({steps_[currentStep_].primaryNote, ch}, 127);
    playingMask_ = 1;
  } else {
    uint8_t baseNote = steps_[currentStep_].primaryNote - (steps_[currentStep_].primaryNote % 12);
    playingMask_ = chord;
    for (uint8_t i = 0; i < 12; i++) {
      if (chord & (1 << i)) {
        Control_Surface.sendNoteOn({baseNote + i, ch}, 127);
      }
    }
  }
}

void Sequencer::setLastNote(uint8_t note) {
  lastNote_ = note;
  lastVelocity_ = 127;
  hasNote_ = true;
}

void Sequencer::toggleStep(uint8_t step, uint16_t chordMask) {
  if (step >= 16) return;
  if (!steps_[step].active) {
    steps_[step].active = true;
    if (steps_[step].chordMask != 0) {
      // Step already has a stored chord (reactivation) — recall it
    } else if (chordMask) {
      steps_[step].chordMask = chordMask;
      lastChord = chordMask;
      for (uint8_t i = 0; i < 12; i++) {
        if (chordMask & (1 << i)) {
          steps_[step].primaryNote = (hasNote_ ? lastNote_ : 60);
          steps_[step].primaryNote = (steps_[step].primaryNote - (steps_[step].primaryNote % 12)) + i;
          break;
        }
      }
    } else if (lastChord) {
      steps_[step].chordMask = lastChord;
      for (uint8_t i = 0; i < 12; i++) {
        if (lastChord & (1 << i)) {
          steps_[step].primaryNote = (hasNote_ ? lastNote_ : 60);
          steps_[step].primaryNote = (steps_[step].primaryNote - (steps_[step].primaryNote % 12)) + i;
          break;
        }
      }
    } else {
      steps_[step].primaryNote = hasNote_ ? lastNote_ : 60;
      steps_[step].chordMask = 1 << (steps_[step].primaryNote % 12);
    }
    if (playing_ && currentStep_ == step) {
      sendStepNotesOff();
      sendStepNotes();
    }
  } else {
    if (playing_ && currentStep_ == step) {
      sendStepNotesOff();
    }
    steps_[step].active = false;
  }
  updateStepLEDs();
}

void Sequencer::clearPattern() {
  sendStepNotesOff();
  for (int i = 0; i < 16; i++) {
    steps_[i].active = false;
    steps_[i].chordMask = 0;
    steps_[i].primaryNote = 60;
  }
  lastChord = 0;
  updateStepLEDs();
}

void Sequencer::togglePlayback() {
  if (playing_) {
    stop();
  } else {
    start();
  }
}

void Sequencer::start() {
  playing_ = true;
  seqLEDsActive = true;
  currentStep_ = 0;
  clockPulseCount_ = 0;
  lastStepTime_ = millis();
  sendStepNotes();
  updateStepLEDs();
}

void Sequencer::stop() {
  playing_ = false;
  seqLEDsActive = false;
  sendStepNotesOff();
  clearAllStepLEDs();
}

void Sequencer::beginChordEdit(uint8_t step) {
  if (step >= 16) return;
  chordEditStep_ = step;
  updateStepLEDs();
}

void Sequencer::endChordEdit() {
  chordEditStep_ = -1;
  pendingLongPressStep_ = -1;
  updateStepLEDs();
}

void Sequencer::checkPendingLongPress() {
  if (pendingLongPressStep_ >= 0 && !isChordEditing()) {
    if (millis() - pendingLongPressStart_ >= 300) {
      beginChordEdit(pendingLongPressStep_);
    }
  }
}

void Sequencer::setPendingLongPress(uint8_t step) {
  pendingLongPressStep_ = step;
  pendingLongPressStart_ = millis();
}

void Sequencer::clearPendingLongPress() {
  pendingLongPressStep_ = -1;
}

void Sequencer::toggleChordNote(uint8_t keyIndex) {
  if (chordEditStep_ < 0 || chordEditStep_ >= 16) return;
  if (keyIndex >= 12) return;

  StepData &s = steps_[chordEditStep_];
  if (!s.active) return;

  s.chordMask ^= (1 << keyIndex);

  // Update primary note to the most recently toggled-on note
  if (s.chordMask & (1 << keyIndex)) {
    s.primaryNote = (s.primaryNote - (s.primaryNote % 12)) + keyIndex;
  }

  // If chord is now empty, deactivate the step
  if (s.chordMask == 0) {
    s.active = false;
  } else {
    lastChord = s.chordMask;
  }

  updateStepLEDs();
}

void Sequencer::updateStepLEDs() {
  for (int i = 0; i < 16; i++) {
    leds[i] = CRGB::Black;
  }

  for (int i = 0; i < 16; i++) {
    if (steps_[i].active) {
      leds[i] = CRGB(0, 30, 0);   // ON steps = dim green
    } else {
      leds[i] = CRGB(0, 0, 30);   // OFF steps = dim blue
    }
  }

  // Chord editing step = bright purple
  if (chordEditStep_ >= 0) {
    leds[chordEditStep_] = CRGB(80, 0, 80);
  }

  // Current step cursor = bright green (overrides all)
  if (playing_) {
    leds[currentStep_] = CRGB::Green;
  }
}

void Sequencer::clearAllStepLEDs() {
  for (int i = 0; i < 16; i++) {
    leds[i] = CRGB::Black;
  }
}

bool Sequencer::stepChanged() {
  if (stepChangedFlag_) {
    stepChangedFlag_ = false;
    return true;
  }
  return false;
}

void Sequencer::syncFromTimeline() {
  if (!timeline_) return;
  extern uint8_t currentKey;
  extern ScaleType currentScale;

  // Resolve timeline events into steps_[] for playback
  for (uint16_t i = 0; i < 16; i++) {
    uint16_t timelineStep = (currentStep_ / 16) * 16 + i;  // map to timeline position
    ResolvedChord rc = timeline_->resolveAtStep(timelineStep, currentKey, currentScale);
    if (rc.active) {
      steps_[i].active = true;
      steps_[i].chordMask = rc.chordMask;
      steps_[i].primaryNote = rc.primaryNote;
    }
    // Don't overwrite inactive steps — they stay as-is
  }
  updateStepLEDs();
}

Sequencer sequencer;
