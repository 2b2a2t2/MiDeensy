# Phase 0 — Behavioral Baseline

**Date:** 2026-09-08
**Compiled:** Clean (only pre-existing USB MIDI board config error)

## Existing Behavior That Must Be Preserved

### 1. 16-Step Activation/Deactivation
- Tap OFF step pad → activates with `lastHeldChord` (held keyboard notes)
- If no keys held, uses `lastChord` (last chord set in sequencer)
- If no `lastChord`, uses `lastNote_` (default 60) as single-note chord
- Tap ON step pad (short press < 300ms) → deactivates step
- Deactivated step preserves its `chordMask` and `primaryNote`
- Reactivating step recalls stored chord (does not use `lastChord`)

### 2. Chord Recall
- `lastHeldChord` = bitmask of currently held keyboard keys
- Updated on every keyboard press/release
- `lastChord` = last chord assigned to any step (tracked globally)
- `chordMask` per step = 12-bit mask, bit 0=C, bit 11=B
- `primaryNote` per step = absolute MIDI note defining octave root

### 3. Chord Editing
- Long-press ON step pad (≥ 300ms) → enters chord edit mode
- `chordEditStep_` tracks which step is being edited
- Keyboard keys toggle notes in/out of `chordMask` via XOR
- Keyboard LED feedback: chord notes shown in Cyan
- Step LED: editing step shown in bright purple
- Release pad → exits chord edit
- If chord becomes empty, step is deactivated

### 4. PLAY
- PLAY button in SEQ mode toggles playback
- `togglePlayback()` calls `start()` or `stop()`
- `start()`: `playing_ = true`, `currentStep_ = 0`, plays notes for step 0
- `stop()`: `playing_ = false`, sends note-off, clears step LEDs

### 5. REC
- REC button in SEQ mode calls `clearPattern()`
- Sends `sendStepNotesOff()` before clearing
- Deactivates all steps, clears all chord masks, resets primary notes to 60
- Clears `lastChord`

### 6. Internal Clock
- When no external clock: `update()` runs internal timing
- Interval: `60000 / (bpm_ * 4)` ms (16th notes at 120 BPM default)
- Advances steps via `advanceStep()`

### 7. External MIDI Clock
- Serial2 bytes intercepted in `loop()` before `Control_Surface.loop()`
- Clock bytes: `0xF8` (pulse), `0xFA` (start), `0xFB` (continue), `0xFC` (stop)
- Pulses counted: advances step every 6 pulses (16th note at 24 PPQN)
- Start: resets to step 0, starts playing
- Stop: stops playback, sends note-off

### 8. Polyphonic Playback
- `sendStepNotes()`: if `chordMask != 0`, plays all set bits as separate notes
- `baseNote = primaryNote - (primaryNote % 12)`
- Each bit `i` in `chordMask` → plays `baseNote + i`
- All notes on Channel_1, velocity 127
- `playingMask_` tracks which notes are currently sounding

### 9. Correct Note-Offs
- `sendStepNotesOff()`: iterates `playingMask_`, sends note-off for each bit
- Uses step's own `primaryNote` for base calculation (not `lastNote_`)
- Called before every `sendStepNotes()` and on stop
- Also called when deactivating a step that's currently playing

### 10. Octave/Retrigger Behavior
- Encoder 1 controls keyboard octave (0-7)
- Default octave 3 on boot (sticky until encoder physically moved)
- `retriggerHeldNotes(newOctave)`: for each held key, sends old NoteOff + new NoteOn
- Keyboard note formula: `base_midiNote + (keyboardOctave - 6) * 12`
- `initialEnc1Value` captured at boot to prevent encoder override

### 11. LED Feedback
- Step LEDs 0-15: dim green (active), dim blue (inactive)
- Current playing step: bright green (overrides)
- Chord editing step: bright purple
- Keyboard LEDs 25-36: show chord in Cyan (edit) or dim cyan (playback)
- `seqLEDsActive` flag prevents `midiled` from writing to pad LEDs 0-15

### 12. Mode Transitions
- BANK_NONE: pads send NoteOn Ch1, keyboard Ch2+bank
- BANK_KEYS (hold KEY): pads select keyboard bank 1-16
- BANK_ENC (hold ENC): pads select encoder bank 1-16
- BANK_SEQ (tap SEQ): sticky, pads toggle steps, PLAY/REC active
- SEQ mode: keyboard plays notes, sets `lastNote_` and `lastHeldChord`

### 13. MIDI Channel Routing
- Pads (BANK_NONE): Channel_1
- Keyboard: Channel_2 + bankKeys.getSelection()
- Control buttons: Channel_1
- Sequencer output: Channel_1
- Encoders: Channel_1 + bankEnc.getSelection()

### 14. Display
- Header redrawn every 200ms
- Shows: "Keys:", "OCT:N", mode indicator ("Enc:" or "SEQ")
- Separator line at y=15
- Mode-specific content below
- Encoder display: 8 vertical bar graphs in BANK_ENC mode

### 15. Compile Baseline
- Pre-existing error: `USBMIDI_Interface` not available (requires Teensy USB Type change)
- All other code compiles cleanly
- Libraries: FastLED 3.4.0, Control_Surface 2.1.2, U8g2 2.36.19, Adafruit_MPR121 1.2.1
