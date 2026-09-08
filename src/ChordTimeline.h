#pragma once

#include "Globals.h"

constexpr uint16_t MAX_THEME_STEPS = 256;

enum ScaleDegree : uint8_t {
  DEGREE_1 = 0, DEGREE_2, DEGREE_3, DEGREE_4, DEGREE_5, DEGREE_6, DEGREE_7
};

struct ChordEvent {
  bool active;
  uint16_t startStep;
  uint8_t duration;         // 1-255 steps
  ScaleDegree degree;
  ChordQuality quality;     // QUALITY_AUTO = resolve from scale
  uint8_t inversion;        // 0=root, 1=1st, 2=2nd
  uint8_t extensions;       // bitmask: bit0=b7, bit1=9, bit2=11, bit3=13
  uint8_t tension;          // 0-127
  uint8_t velocity;         // 0-127
};

struct ResolvedChord {
  bool active;
  uint16_t chordMask;       // 12-bit pitch class mask
  uint8_t primaryNote;      // MIDI note (root of chord in correct octave)
  uint8_t velocity;
};

class ChordTimeline {
public:
  ChordTimeline();

  void clear();
  bool addEvent(const ChordEvent& evt);
  bool removeEvent(uint16_t step);
  const ChordEvent* getEventAtStep(uint16_t step) const;
  const ChordEvent* getEventByIndex(uint8_t idx) const;
  uint8_t getEventCount() const { return eventCount_; }

  // Resolve a chord event to MIDI chord mask + primary note
  ResolvedChord resolve(const ChordEvent& evt, uint8_t key, ScaleType scale) const;

  // Resolve the event at a given step
  ResolvedChord resolveAtStep(uint16_t step, uint8_t key, ScaleType scale) const;

  // Get/set loop length
  uint16_t getLoopLength() const { return loopLength_; }
  void setLoopLength(uint16_t len);

  // Sync from raw StepData (backward compatibility)
  void importFromStepData(const void* steps, uint8_t count, uint8_t key, ScaleType scale);

private:
  ChordEvent events_[MAX_THEME_STEPS];
  uint8_t eventCount_;
  uint16_t loopLength_;

  // Get the MIDI note for a scale degree in a given key
  uint8_t degreeToMidiNote(ScaleDegree degree, uint8_t key, ScaleType scale) const;
};
