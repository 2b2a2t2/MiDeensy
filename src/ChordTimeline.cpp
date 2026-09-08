#include "ChordTimeline.h"

ChordTimeline::ChordTimeline()
  : eventCount_(0), loopLength_(16) {
  clear();
}

void ChordTimeline::clear() {
  for (uint16_t i = 0; i < MAX_THEME_STEPS; i++) {
    events_[i].active = false;
  }
  eventCount_ = 0;
  loopLength_ = 16;
}

void ChordTimeline::setLoopLength(uint16_t len) {
  if (len < 1) len = 1;
  if (len > MAX_THEME_STEPS) len = MAX_THEME_STEPS;
  loopLength_ = len;
  // Remove events beyond the new loop length
  for (uint16_t i = 0; i < MAX_THEME_STEPS; i++) {
    if (events_[i].active && events_[i].startStep >= loopLength_) {
      events_[i].active = false;
      eventCount_--;
    }
  }
}

bool ChordTimeline::addEvent(const ChordEvent& evt) {
  if (evt.startStep >= MAX_THEME_STEPS) return false;
  if (evt.duration == 0) return false;

  // Check for overlap with existing events — remove conflicting ones
  uint16_t evtEnd = evt.startStep + evt.duration;
  if (evtEnd > loopLength_) evtEnd = loopLength_;

  for (uint16_t i = 0; i < MAX_THEME_STEPS; i++) {
    if (events_[i].active) {
      // Check overlap: existing event's range [start, start+duration) overlaps new event's range
      uint16_t existingEnd = events_[i].startStep + events_[i].duration;
      if (existingEnd > loopLength_) existingEnd = loopLength_;
      uint16_t evtEnd = evt.startStep + evt.duration;
      if (evtEnd > loopLength_) evtEnd = loopLength_;

      if (events_[i].startStep < evtEnd && existingEnd > evt.startStep) {
        // Check it's not the exact same event (same start, same degree, same duration)
        if (events_[i].startStep == evt.startStep &&
            events_[i].degree == evt.degree &&
            events_[i].duration == evt.duration) {
          continue;  // same event, skip removal
        }
        // Overlap found — remove existing event
        events_[i].active = false;
        eventCount_--;
      }
    }
  }

  // Find an empty slot or reuse the same slot
  for (uint16_t i = 0; i < MAX_THEME_STEPS; i++) {
    if (!events_[i].active) {
      events_[i] = evt;
      events_[i].active = true;
      events_[i].duration = evt.duration;
      if (events_[i].startStep + events_[i].duration > loopLength_) {
        events_[i].duration = loopLength_ - events_[i].startStep;
      }
      eventCount_++;
      return true;
    }
  }

  return false;  // no empty slots (shouldn't happen with 256 slots)
}

bool ChordTimeline::removeEvent(uint16_t step) {
  for (uint16_t i = 0; i < MAX_THEME_STEPS; i++) {
    if (events_[i].active && events_[i].startStep == step) {
      events_[i].active = false;
      eventCount_--;
      return true;
    }
  }
  return false;
}

const ChordEvent* ChordTimeline::getEventAtStep(uint16_t step) const {
  if (step >= loopLength_) step = step % loopLength_;

  // Find the event whose startStep <= step < startStep + duration
  for (uint16_t i = 0; i < MAX_THEME_STEPS; i++) {
    if (events_[i].active) {
      uint16_t end = events_[i].startStep + events_[i].duration;
      if (end > loopLength_) end = loopLength_;
      if (events_[i].startStep <= step && step < end) {
        return &events_[i];
      }
    }
  }
  return nullptr;
}

const ChordEvent* ChordTimeline::getEventByIndex(uint8_t idx) const {
  uint8_t count = 0;
  for (uint16_t i = 0; i < MAX_THEME_STEPS; i++) {
    if (events_[i].active) {
      if (count == idx) return &events_[i];
      count++;
    }
  }
  return nullptr;
}

uint8_t ChordTimeline::degreeToMidiNote(ScaleDegree degree, uint8_t key, ScaleType scale) const {
  if (scale >= SCALE_COUNT) scale = SCALE_MAJOR;
  uint8_t degreeIdx = (uint8_t)degree;
  if (degreeIdx > 6) degreeIdx = 6;
  return key + SCALE_INTERVALS[scale][degreeIdx];
}

ResolvedChord ChordTimeline::resolve(const ChordEvent& evt, uint8_t key, ScaleType scale) const {
  ResolvedChord result;
  result.active = evt.active;
  result.velocity = evt.velocity;

  if (!evt.active) {
    result.chordMask = 0;
    result.primaryNote = 60;
    return result;
  }

  // Get the root note of this degree
  uint8_t rootNote = degreeToMidiNote(evt.degree, key, scale);

  // Resolve quality
  ChordQuality q = evt.quality;
  if (q == QUALITY_AUTO) {
    if (scale < SCALE_COUNT) {
      q = (ChordQuality)DEFAULT_CHORD_QUALITY[scale][(uint8_t)evt.degree];
    } else {
      q = MAJ;
    }
  }

  // Build chord mask relative to root
  uint16_t mask = 0;
  switch (q) {
    case MAJ:
      mask = (1 << 0) | (1 << 4) | (1 << 7);  // root, major 3rd, perfect 5th
      break;
    case MIN:
      mask = (1 << 0) | (1 << 3) | (1 << 7);  // root, minor 3rd, perfect 5th
      break;
    case DIM:
      mask = (1 << 0) | (1 << 3) | (1 << 6);  // root, minor 3rd, diminished 5th
      break;
    case AUG:
      mask = (1 << 0) | (1 << 4) | (1 << 8);  // root, major 3rd, augmented 5th
      break;
    case DOM7:
      mask = (1 << 0) | (1 << 4) | (1 << 7) | (1 << 10);  // dominant 7th
      break;
    case MIN7:
      mask = (1 << 0) | (1 << 3) | (1 << 7) | (1 << 10);  // minor 7th
      break;
    case MAJ7:
      mask = (1 << 0) | (1 << 4) | (1 << 7) | (1 << 11);  // major 7th
      break;
    default:
      mask = (1 << 0) | (1 << 4) | (1 << 7);
      break;
  }

  // Apply extensions
  if (evt.extensions & 0x01) mask |= (1 << 10);  // b7
  if (evt.extensions & 0x02) mask |= (1 << 2);   // 9 (D = 2 semitones from C)
  if (evt.extensions & 0x04) mask |= (1 << 5);   // 11 (F = 5 semitones from C)
  if (evt.extensions & 0x08) mask |= (1 << 9);   // 13 (A = 9 semitones from C)

  // Apply inversion: rotate the mask
  if (evt.inversion == 1) {
    // 1st inversion: move root up an octave (remove bit 0, it's implied by primaryNote)
    // For MIDI playback, we shift the root class
    uint8_t rootPitchClass = rootNote % 12;
    mask &= ~(1 << rootPitchClass);
    mask |= (1 << ((rootPitchClass + 4) % 12));  // move 3rd to bottom
  } else if (evt.inversion == 2) {
    uint8_t rootPitchClass = rootNote % 12;
    mask &= ~(1 << rootPitchClass);
    mask &= ~(1 << ((rootPitchClass + 4) % 12));  // remove major 3rd
    mask |= (1 << ((rootPitchClass + 7) % 12));   // 5th to bottom
  }

  result.chordMask = mask;
  result.primaryNote = rootNote;
  return result;
}

ResolvedChord ChordTimeline::resolveAtStep(uint16_t step, uint8_t key, ScaleType scale) const {
  const ChordEvent* evt = getEventAtStep(step);
  if (!evt) {
    ResolvedChord empty;
    empty.active = false;
    empty.chordMask = 0;
    empty.primaryNote = 60;
    empty.velocity = 0;
    return empty;
  }
  return resolve(*evt, key, scale);
}

void ChordTimeline::importFromStepData(const void* steps, uint8_t count, uint8_t key, ScaleType scale) {
  clear();
  // This is a compatibility import — we can't reverse-engineer scale degrees
  // from raw chordMasks without knowing the scale. For now, import as raw events.
  // The harmony engine will replace these with proper degree-based events.
}
