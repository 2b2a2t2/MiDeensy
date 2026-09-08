#pragma once

#include "Globals.h"
#include "ChordTimeline.h"

// Harmonic function categories
enum HarmonicFunction : uint8_t {
  HFUNC_TONIC = 0,      // I, vi
  HFUNC_PREDOMINANT,    // ii, IV
  HFUNC_DOMINANT,       // V, vii°
  HFUNC_PASSING,        // iii, vi
  HFUNC_COUNT
};

// Get the harmonic function of a scale degree in a given scale
HarmonicFunction getHarmonicFunction(ScaleDegree degree, ScaleType scale);

// Transition probability from one function to another
// [from][to] = weight (higher = more likely)
extern const uint8_t TRANSITION_WEIGHTS[HFUNC_COUNT][HFUNC_COUNT];

struct HarmonyParams {
  uint8_t key;             // root MIDI note (0-11)
  ScaleType scale;
  uint8_t density;         // chords per loop (1-16)
  uint8_t tension;         // overall tension level (0-127)
  uint8_t variation;       // mutation amount for EVOLVE (0-127)
  uint16_t loopLength;     // steps in the loop
};

struct HarmonyLock {
  bool progression;        // lock chord progression
  bool density;            // lock density
  bool tension;            // lock tension
};

class HarmonyEngine {
public:
  HarmonyEngine();

  // Generate a new progression (NEW)
  void generate(ChordTimeline& timeline, const HarmonyParams& params);

  // Evolve an existing progression (EVOLVE)
  void evolve(ChordTimeline& timeline, const HarmonyParams& params,
              const HarmonyLock& locks);

  // Generate a single chord at a position
  ChordEvent generateChord(uint16_t step, uint8_t duration,
                           const HarmonyParams& params,
                           ScaleDegree prevDegree) const;

  // Select a degree weighted by harmonic function and phrase position
  ScaleDegree selectDegree(HarmonicFunction targetFunc, ScaleType scale,
                           uint16_t step, uint16_t loopLength) const;

private:
  // Simple random (will be replaced by seeded generator later)
  uint8_t random8() const;
  uint16_t random16() const;

  // Weighted random selection from an array of weights
  uint8_t weightedSelect(const uint8_t* weights, uint8_t count) const;

  // Phrase position factor (0-255): 0=beginning, 255=end of phrase
  uint8_t phrasePosition(uint16_t step, uint16_t loopLength) const;

  // Tension curve: increases toward phrase end
  uint8_t tensionAtStep(uint16_t step, uint16_t loopLength, uint8_t baseTension) const;
};
