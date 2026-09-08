#include "HarmonyEngine.h"
#include <Arduino.h>

// Transition weights: [from function][to function]
// TONIC → PREDOMINANT is strong, TONIC → DOMINANT is weak
// DOMINANT → TONIC is very strong (resolution)
const uint8_t TRANSITION_WEIGHTS[HFUNC_COUNT][HFUNC_COUNT] = {
  //  TONIC  PREDOM  DOMIN  PASSING
  {     10,    30,     5,     15 },   // from TONIC
  {     15,    10,    30,     10 },   // from PREDOMINANT
  {     40,     5,     5,     10 },   // from DOMINANT (strong resolution)
  {     20,    15,    15,     10 },   // from PASSING
};

// Which degrees belong to which function
// For MAJOR scale: I=TONIC, ii=PREDOMINANT, iii=PASSING, IV=PREDOMINANT, V=DOMINANT, vi=TONIC, vii°=DOMINANT
static const HarmonicFunction MAJOR_FUNCTIONS[7] = {
  HFUNC_TONIC,       // I
  HFUNC_PREDOMINANT, // ii
  HFUNC_PASSING,     // iii
  HFUNC_PREDOMINANT, // IV
  HFUNC_DOMINANT,    // V
  HFUNC_TONIC,       // vi
  HFUNC_DOMINANT     // vii°
};

static const HarmonicFunction MINOR_FUNCTIONS[7] = {
  HFUNC_TONIC,       // i
  HFUNC_DOMINANT,    // ii°
  HFUNC_PASSING,     // III
  HFUNC_PREDOMINANT, // iv
  HFUNC_DOMINANT,    // v
  HFUNC_PASSING,     // VI
  HFUNC_PREDOMINANT  // VII
};

HarmonicFunction getHarmonicFunction(ScaleDegree degree, ScaleType scale) {
  uint8_t d = (uint8_t)degree;
  if (d > 6) d = 6;

  if (scale == SCALE_MAJOR) {
    return MAJOR_FUNCTIONS[d];
  } else if (scale == SCALE_MINOR || scale == SCALE_HARMONIC_MINOR || scale == SCALE_MELODIC_MINOR) {
    return MINOR_FUNCTIONS[d];
  } else if (scale == SCALE_DORIAN) {
    // DORIAN: I ii III IV V vi° VII
    static const HarmonicFunction dorian[7] = {
      HFUNC_TONIC, HFUNC_PREDOMINANT, HFUNC_PASSING, HFUNC_PREDOMINANT,
      HFUNC_DOMINANT, HFUNC_DOMINANT, HFUNC_PREDOMINANT
    };
    return dorian[d];
  } else if (scale == SCALE_MIXOLYDIAN) {
    // MIXOLYDIAN: I ii iii° IV v vi VII
    static const HarmonicFunction mixo[7] = {
      HFUNC_TONIC, HFUNC_PREDOMINANT, HFUNC_DOMINANT, HFUNC_PREDOMINANT,
      HFUNC_DOMINANT, HFUNC_TONIC, HFUNC_PREDOMINANT
    };
    return mixo[d];
  }
  return MAJOR_FUNCTIONS[d];
}

HarmonyEngine::HarmonyEngine() {}

uint8_t HarmonyEngine::random8() const {
  return (uint8_t)(::random(256));
}

uint16_t HarmonyEngine::random16() const {
  return (uint16_t)(::random(65536));
}

uint8_t HarmonyEngine::weightedSelect(const uint8_t* weights, uint8_t count) const {
  uint16_t total = 0;
  for (uint8_t i = 0; i < count; i++) {
    total += weights[i];
  }
  if (total == 0) return 0;

  uint16_t r = random16() % total;
  uint16_t acc = 0;
  for (uint8_t i = 0; i < count; i++) {
    acc += weights[i];
    if (r < acc) return i;
  }
  return count - 1;
}

uint8_t HarmonyEngine::phrasePosition(uint16_t step, uint16_t loopLength) const {
  if (loopLength == 0) return 0;
  return (uint8_t)((uint32_t)step * 255 / loopLength);
}

uint8_t HarmonyEngine::tensionAtStep(uint16_t step, uint16_t loopLength, uint8_t baseTension) const {
  uint8_t phrasePos = phrasePosition(step, loopLength);
  // Tension increases toward phrase end, with a bump before resolution
  uint8_t curveTension = (uint8_t)((uint16_t)phrasePos * baseTension / 255);
  // Add a bump at ~75% of phrase (dominant area)
  if (phrasePos > 160 && phrasePos < 220) {
    curveTension = 255;
  }
  return curveTension;
}

ScaleDegree HarmonyEngine::selectDegree(HarmonicFunction targetFunc, ScaleType scale,
                                         uint16_t step, uint16_t loopLength) const {
  // Find all degrees that belong to the target function
  uint8_t candidates[7];
  uint8_t candidateCount = 0;

  for (uint8_t d = 0; d < 7; d++) {
    if (getHarmonicFunction((ScaleDegree)d, scale) == targetFunc) {
      candidates[candidateCount++] = d;
    }
  }

  if (candidateCount == 0) return DEGREE_1;

  // Weight by phrase position: prefer I/i at beginning and end
  uint8_t weights[7];
  uint8_t phrasePos = phrasePosition(step, loopLength);
  for (uint8_t i = 0; i < candidateCount; i++) {
    weights[i] = 20;  // base weight
    if (candidates[i] == 0) {  // I/i
      // Strong at beginning and end of phrase
      if (phrasePos < 32 || phrasePos > 220) {
        weights[i] = 60;
      } else {
        weights[i] = 15;
      }
    }
  }

  uint8_t selected = weightedSelect(weights, candidateCount);
  return (ScaleDegree)candidates[selected];
}

ChordEvent HarmonyEngine::generateChord(uint16_t step, uint8_t duration,
                                         const HarmonyParams& params,
                                         ScaleDegree prevDegree) const {
  ChordEvent evt;
  evt.active = true;
  evt.startStep = step;
  evt.duration = duration;
  evt.inversion = 0;
  evt.extensions = 0;
  evt.velocity = 100;
  evt.quality = QUALITY_AUTO;

  // Determine target harmonic function based on previous degree and phrase position
  HarmonicFunction prevFunc = getHarmonicFunction(prevDegree, params.scale);
  uint8_t phrasePos = phrasePosition(step, params.loopLength);
  uint8_t currentTension = tensionAtStep(step, params.loopLength, params.tension);

  // Select target function based on transition weights
  uint8_t funcWeights[HFUNC_COUNT];
  for (uint8_t f = 0; f < HFUNC_COUNT; f++) {
    funcWeights[f] = TRANSITION_WEIGHTS[prevFunc][f];
    // Boost dominant function at high tension
    if (f == HFUNC_DOMINANT && currentTension > 180) {
      funcWeights[f] += 20;
    }
    // Boost tonic function at phrase resolution points
    if (f == HFUNC_TONIC && phrasePos > 220) {
      funcWeights[f] += 30;
    }
    // Prefer tonic at phrase start
    if (f == HFUNC_TONIC && phrasePos < 32) {
      funcWeights[f] += 40;
    }
  }

  HarmonicFunction targetFunc = (HarmonicFunction)weightedSelect(funcWeights, HFUNC_COUNT);
  evt.degree = selectDegree(targetFunc, params.scale, step, params.loopLength);

  // Tension influences velocity
  evt.velocity = map(params.tension, 0, 127, 80, 120);

  return evt;
}

void HarmonyEngine::generate(ChordTimeline& timeline, const HarmonyParams& params) {
  timeline.clear();
  timeline.setLoopLength(params.loopLength);

  uint16_t stepsPerChord = params.loopLength / params.density;
  if (stepsPerChord < 1) stepsPerChord = 1;

  ScaleDegree prevDegree = DEGREE_1;  // start from tonic
  uint16_t step = 0;

  while (step < params.loopLength) {
    uint8_t duration = stepsPerChord;
    if (step + duration > params.loopLength) {
      duration = params.loopLength - step;
    }

    ChordEvent evt = generateChord(step, duration, params, prevDegree);
    timeline.addEvent(evt);
    prevDegree = evt.degree;
    step += duration;
  }
}

void HarmonyEngine::evolve(ChordTimeline& timeline, const HarmonyParams& params,
                            const HarmonyLock& locks) {
  if (locks.progression) return;  // locked, don't evolve

  // Determine mutation amount from variation parameter
  uint8_t mutationChance = params.variation;  // 0-127

  // Iterate through existing events and mutate some
  for (uint8_t i = 0; i < timeline.getEventCount(); i++) {
    const ChordEvent* evt = timeline.getEventByIndex(i);
    if (!evt) continue;

    uint8_t r = random8();
    if (r < (mutationChance / 4)) {  // ~25% of variation = chance per chord
      ChordEvent mutated = *evt;

      // Pick a mutation type
      uint8_t mutationType = random8() % 4;
      switch (mutationType) {
        case 0: {
          // Change degree (stepwise or common substitution)
          int8_t shift = (random8() % 3) - 1;  // -1, 0, or +1
          int8_t newDeg = (int8_t)mutated.degree + shift;
          if (newDeg < 0) newDeg = 6;
          if (newDeg > 6) newDeg = 0;
          mutated.degree = (ScaleDegree)newDeg;
          break;
        }
        case 1: {
          // Change quality
          uint8_t q = random8() % 7;
          mutated.quality = (ChordQuality)(q + 1);  // skip QUALITY_AUTO
          break;
        }
        case 2: {
          // Change duration
          uint8_t newDur = mutated.duration + (random8() % 3) - 1;
          if (newDur < 1) newDur = 1;
          if (newDur > params.loopLength) newDur = params.loopLength;
          mutated.duration = newDur;
          break;
        }
        case 3: {
          // Change inversion
          mutated.inversion = (mutated.inversion + 1) % 3;
          break;
        }
      }

      // Remove old event and add mutated one
      timeline.removeEvent(evt->startStep);
      timeline.addEvent(mutated);
    }
  }
}
