# Architecture Specification

## Design Corrections (from user)

### 1. Timeline Size
- **MAX_THEME_STEPS = 256** (not hardcoded to 16)
- `loopLength` is configurable (default 16)
- 16 physical pads = editing window into the timeline
- Pad mapping: `padIndex` → `currentViewOffset + padIndex` where `currentViewOffset` scrolls through the timeline

### 2. ChordEvent Representation
- Overlapping events are NOT allowed
- A new event at a position occupied by an existing event replaces it
- Events are resolved at playback time: `getEventAtStep(step)` iterates events sorted by `startStep`, returns the event whose `startStep ≤ step < startStep + duration`
- `StepData` (chordMask + primaryNote) remains the resolved playback representation
- `ChordEvent` is the musical representation stored in the timeline

### 3. PendingChange — No void*
- Strongly typed union/struct:
```cpp
struct PendingChange {
  enum Type { CHORD_CHANGE, PROGRESSION_CHANGE, KEY_CHANGE, RHYTHM_CHANGE };
  Type type;
  uint8_t targetStep;
  union {
    struct { uint8_t degree; uint8_t quality; uint8_t duration; } chord;
    struct { uint8_t key; uint8_t scale; } keyChange;
    struct { uint8_t density; } rhythm;
  } data;
};
```

### 4. SEQ Slot vs MIDI Channel vs VoiceRole
- `selectedSlot` = 0-15, the SEQ slot currently being edited (selected via SEQ+PAD)
- Each slot has independent config:
```cpp
struct SeqSlot {
  uint8_t midiChannel;    // 1-16
  VoiceRole role;         // DRUMS, BASS, PAD, ARP, LEAD, NONE
  bool enabled;
  bool locked;
};
```
- 16 slots, each with its own ChordTimeline, voice state, and locks
- No ambiguous double-channel semantics

### 5. ScaleDegree
- `DEGREE_1` through `DEGREE_7` (not roman numeral quality)
- Quality resolved from scale/mode at playback time
- Major scale default: 1=Maj, 2=min, 3=min, 4=Maj, 5=Maj, 6=min, 7=dim
- Natural minor default: 1=min, 2=dim, 3=Maj, 4=min, 5=min, 6=Maj, 7=Maj

### 6. Encoder Pickup
- Entering a new SEQ context does NOT cause parameter jumps
- Encoder values are captured on context entry as baseline
- Only encoder movement AFTER entry changes the parameter
- BANK_ENC remains completely independent (MIDI CC controller)

### 7. Phase 0 Baseline
- Documented in PHASE0_BASELINE.md
- Compile verified: clean (only USB MIDI board config error)

---

## File Structure

```
src/
├── Globals.h              — shared types, constants, enums
├── TouchHandler.h/.cpp    — button arrays, touch/gesture callbacks
├── Display.h/.cpp         — OLED display
├── DisplayInterfaceU8G2.hpp — U8G2 adapter
├── BankHandler.h/.cpp     — mode transitions, bank LEDs
├── NoteLED.h/.cpp         — MIDI-to-LED mapping
├── MIDIBuffer.h           — Serial2 ring buffer
├── MPR121_GestureHelper.h/.cpp — touch sensor abstraction
├── Sequencer.h/.cpp       — 16-step sequencer (preserved)
├── ChordTimeline.h/.cpp   — NEW: musical timeline (up to 256 steps)
├── HarmonyEngine.h/.cpp   — NEW: progression generation
├── VoiceManager.h/.cpp    — NEW: MIDI slot/role/channel config
├── Quantizer.h/.cpp       — NEW: pending/quantized changes
└── JamState.h/.cpp        — NEW: central musical state
```

---

## State Architecture

### Global State (Globals.h)

```cpp
// Existing (preserved)
BankMode currentBankMode;      // BANK_NONE, BANK_KEYS, BANK_ENC, BANK_SEQ
bool modeButtonHeld;
BankMode lastActiveMode;
int8_t keyboardOctave;         // 0-7, default 3
bool seqLEDsActive;
uint16_t lastHeldChord;
uint16_t lastChord;
Bank<16> bankKeys;
Bank<16> bankEnc;

// New
uint8_t selectedSlot;          // 0-15, SEQ slot being edited
SeqFunction seqFunction;       // SEQ_THEME, SEQ_HARMONY, SEQ_RHYTHM, SEQ_VOICE
```

### SeqSlot (VoiceManager.h)

```cpp
enum VoiceRole { ROLE_NONE, ROLE_DRUMS, ROLE_BASS, ROLE_PAD, ROLE_ARP, ROLE_LEAD };

struct SeqSlot {
  uint8_t midiChannel;    // 1-16
  VoiceRole role;
  bool enabled;
  bool locked;            // prevents EVOLVE from changing this slot
};
```

### ChordEvent (ChordTimeline.h)

```cpp
enum ScaleDegree : uint8_t { DEGREE_1 = 0, DEGREE_2, DEGREE_3, DEGREE_4, DEGREE_5, DEGREE_6, DEGREE_7 };
enum ChordQuality : uint8_t { QUALITY_AUTO, MAJ, MIN, DIM, AUG, DOM7, MIN7, MAJ7 };

struct ChordEvent {
  bool active;
  uint16_t startStep;       // 0-255
  uint8_t duration;         // 1-255 (clamped to remaining timeline)
  ScaleDegree degree;
  ChordQuality quality;     // QUALITY_AUTO = resolve from scale
  uint8_t inversion;        // 0=root, 1=1st, 2=2nd
  uint8_t extensions;       // bitmask: b7, 9, 11, 13
  uint8_t tension;          // 0-127
  uint8_t velocity;         // 0-127
};
```

### ChordTimeline (ChordTimeline.h)

```cpp
class ChordTimeline {
public:
  static const uint16_t MAX_STEPS = 256;

  void clear();
  bool addEvent(const ChordEvent& evt);          // false if overlapping
  bool replaceEvent(uint16_t step, const ChordEvent& evt);
  bool removeEvent(uint16_t step);
  const ChordEvent* getEventAtStep(uint16_t step) const;
  uint16_t getEventCount() const;
  const ChordEvent* getEventByIndex(uint8_t idx) const;

  // Resolution: ChordEvent → MIDI notes
  void resolveToStepData(uint16_t step, uint8_t* chordMask, uint8_t* primaryNote,
                          uint8_t key, const uint8_t* scaleIntervals) const;

private:
  ChordEvent events_[MAX_STEPS];  // sparse: most are inactive
  uint8_t eventCount_;
};
```

### PendingChange (Quantizer.h)

```cpp
struct PendingChange {
  enum Type { CHORD_CHANGE, KEY_CHANGE, RHYTHM_CHANGE, PROGRESSION_CHANGE };
  Type type;
  uint8_t targetStep;
  union {
    struct { uint16_t step; uint8_t degree; uint8_t quality; uint8_t duration; uint8_t inversion; } chord;
    struct { uint8_t key; uint8_t scale; } keyChange;
    struct { uint8_t density; uint8_t swing; } rhythm;
  } data;
  bool active;
};
```

### JamState (JamState.h)

```cpp
struct JamState {
  // Transport
  uint8_t bpm;
  uint16_t loopLength;         // 1-256
  uint16_t currentStep;
  uint16_t currentBar;
  uint8_t currentPhrase;       // which phrase in the section

  // Harmony
  uint8_t key;                 // root MIDI note (0-11)
  uint8_t scaleType;           // MAJOR, MINOR, DORIAN, etc.
  uint8_t tension;             // 0-127
  uint8_t density;             // events per loop

  // Locks (per layer)
  bool lockTheme;
  bool lockHarmony;
  bool lockRhythm;
  bool lockVoice[16];          // per-slot voice lock

  // Generation
  uint8_t variation;           // mutation amount for EVOLVE
  uint8_t novelty;             // how much new material to introduce
};
```

---

## Interaction Mapping

### ENC Mode (unchanged)
- Short press ENC → enter BANK_ENC
- F1 + PAD 1-16 → select CC page
- HOLD ENC + PAD → select MIDI channel
- 8 encoders → 8 CC parameters
- Display: encoder bar graphs

### KEY Mode (unchanged)
- Short press KEY → enter BANK_KEYS
- HOLD KEY + PAD → select keyboard MIDI channel
- Keyboard plays notes on selected channel

### SEQ Mode
- Tap SEQ → enter BANK_SEQ
- Tap SEQ again → exit
- **F1-F4 context layers:**
  - F1 → SEQ_THEME (theme parameters)
  - F2 → SEQ_HARMONY (chord progression editing)
  - F3 → SEQ_RHYTHM (rhythm generation)
  - F4 → SEQ_VOICE (voice configuration)
- **M1-M3 actions (immediate, no confirmation):**
  - M1 → NEW (generate new idea for current context)
  - M2 → EVOLVE (mutate current idea for current context)
  - M3 → LOCK (toggle lock for current context)
- **SEQ + PAD → select active slot (0-15)**
- **PAD 1-16 → toggle steps (preserved)**
- **PAD + selected step → select timeline position for editing**
- **Keyboard → context-dependent:**
  - SEQ_HARMONY: select scale degree for selected step
  - Other contexts: play notes (same as current behavior)
- **Encoders → context-dependent:**
  - SEQ_HARMONY: duration, inversion, tension, extensions
  - SEQ_RHYTHM: density, swing
  - SEQ_THEME: energy, tension
  - SEQ_VOICE: octave offset, velocity
- **PLAY → toggle playback (preserved)**
- **REC → clear pattern (preserved)**

### Keyboard Degree Selection (SEQ_HARMONY)
1. Press step pad → `selectedStep = padIndex`
2. Press keyboard key → assign degree to `selectedStep`
3. Degree resolved to chord using current key/scale
4. Encoder adjusts duration/inversion/tension

### Quantized Changes
- Chord changes → applied at next step boundary
- Key/scale changes → applied at next loop boundary
- Rhythm changes → applied at next phrase boundary
- Note: chord assignment via keyboard is IMMEDIATE (no quantization for direct editing)

---

## Phase Plan

### Phase 1: State Machine + SEQ Sub-modes (~30 lines)
- Add `SeqFunction` enum to `Globals.h`
- Add `selectedSlot`, `seqFunction` globals
- Modify `enterBankMode(BANK_SEQ)` to initialize `seqFunction`
- Modify `exitBankMode()` to clear `seqFunction`
- Compile and verify

### Phase 2: Button Handling (~40 lines)
- F1-F4: in BANK_SEQ, set `seqFunction`; else send NoteOn
- M1-M3: in BANK_SEQ, trigger NEW/EVOLVE/LOCK; else send NoteOn
- SEQ+PAD: set `selectedSlot`
- Compile and verify

### Phase 3: Encoder Context Mapping (~40 lines)
- Add encoder pickup/baseline mechanism
- Context-sensitive encoder routing in BANK_SEQ
- BANK_ENC remains independent
- Compile and verify

### Phase 4: ChordTimeline (~150 lines new)
- ChordEvent struct, ChordTimeline class
- Resolution to StepData
- Integration with existing Sequencer playback

### Phase 5: Harmony Engine (~300 lines new)
- Scale definitions, harmonic functions
- Progression generation (NEW)
- Progression mutation (EVOLVE)
- Lock support

### Phase 6: Voice Manager + Channel Routing (~80 new + ~20 refactor)
- SeqSlot configuration
- Channel routing in sendStepNotes
- Voice-specific generation

### Phase 7: Quantized Changes (~60 new + ~10 refactor)
- PendingChange struct
- Quantizer logic
- Integration with advanceStep()

### Phase 8: Display Updates (~20 lines)
- Show selected step, seq function, key/scale, locked layers
