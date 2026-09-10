# MiDeensy

A touch-sensitive MIDI controller built on Teensy, featuring 42 capacitive pads, a chromatic keyboard, an OLED display, and rotary encoder navigation.

## Hardware Overview

| Component | Details |
|-----------|---------|
| MCU | Teensy (USB MIDI) |
| Touch | 4x MPR121 capacitive sensors (0x5A–0x5D) via I2C |
| LEDs | 42x NeoPixel (pin 6) |
| Display | SSD1306 128x64 I2C OLED (top 16px yellow header, bottom 48px blue) |
| Encoder | 1x rotary encoder (currently mapped to octave select) |

## Conceptual Workflow

```
┌──────────────────────────────────────────────────────┐
│                    MPR121 Sensors                     │
│          4 sensors x 12 channels = 48 inputs         │
└────────────────────┬─────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────┐
│              MPR121_GestureHelper                    │
│  Debounce (10ms) ─► Long Press (300ms) ─► Double Tap │
└────────────────────┬─────────────────────────────────┘
                     │  touchCallback / gestureCallback
                     ▼
┌──────────────────────────────────────────────────────┐
│                  TouchHandler                        │
│  findButton(sensor, channel) ─► SensorMap lookup     │
│                                                       │
│  ┌─────────┬────────────┬────────────┐              │
│  │ PAD     │ CONTROL    │ KEYBOARD   │              │
│  │ CH 1    │ CH 1       │ CH 2       │              │
│  │ vel 127 │ vel 127    │ vel 100    │              │
│  │ note 59+│ note=LED   │ note=84+   │              │
│  │  ledIdx │  index     │ (led-25)+  │              │
│  └────┬────┴─────┬──────┴──(oct-6)×12│              │
│       │          │          │         │              │
└───────┼──────────┼──────────┼─────────┘              │
        │          │          │                        │
        ▼          ▼          ▼                        │
┌──────────────────────────────────────────────────────┐
│                  USB MIDI Out                        │
│  Channel 1: Pads + Controls                         │
│  Channel 2: Keyboard                                │
└──────────────────────────────────────────────────────┘
        │          │          │
        ▼          ▼          ▼
┌──────────────────────────────────────────────────────┐
│                   NoteLED                           │
│  Each touch lights the corresponding NeoPixel white │
│  (index 0–41)                                       │
└──────────────────────────────────────────────────────┘
```

## Button Types

### Pads (16x) — Channel 1

| LED | Name | Sensor | Ch |
|-----|------|--------|----|
| 0 | PAD1 | 0x5A | 1 |
| 1 | PAD2 | 0x5A | 4 |
| 2 | PAD3 | 0x5A | 2 |
| 3 | PAD4 | 0x5C | 10 |
| 4 | PAD5 | 0x5A | 0 |
| 5 | PAD6 | 0x5C | 8 |
| 6 | PAD7 | 0x5C | 2 |
| 7 | PAD8 | 0x5B | 0 |
| 8 | PAD9 | 0x5C | 0 |
| 9 | PAD10 | 0x5B | 5 |
| 10 | PAD11 | 0x5C | 5 |
| 11 | PAD12 | 0x5B | 1 |
| 12 | PAD13 | 0x5A | 0 |
| 13 | PAD14 | 0x5B | 7 |
| 14 | PAD15 | 0x5B | 9 |
| 15 | PAD16 | 0x5A | 1 |

**MIDI note = 59 + ledIndex** (note range B3–G5)

### Controls (14x) — Channel 1

| LED | Name | Sensor | Ch |
|-----|------|--------|----|
| 16 | PREV | 0x5C | 9 |
| 17 | NEXT | 0x5A | 5 |
| 18 | M1 | 0x5A | 3 |
| 19 | M2 | 0x5C | 6 |
| 20 | M3 | 0x5C | 11 |
| 21 | F1 | 0x5C | 7 |
| 22 | F2 | 0x5C | 3 |
| 23 | F3 | 0x5C | 1 |
| 24 | F4 | 0x5C | 4 |
| 32 | ENC | 0x5A | 10 |
| 33 | SEQ | 0x5A | 11 |
| 34 | KEY | 0x5A | 9 |
| 35 | PLAY | 0x5A | 8 |
| 36 | REC | 0x5A | 7 |

**MIDI note = ledIndex** (fixed notes)

### Keyboard (12x) — Channel 2

| LED | Name | Sensor | Ch | Semitone offset |
|-----|------|--------|----|-----------------|
| 25 | KEY_C | 0x5B | 2 | 0 |
| 37 | KEY_C# | 0x5A | 5 | 1 |
| 26 | KEY_D | 0x5B | 3 | 2 |
| 38 | KEY_D# | 0x5A | 4 | 3 |
| 27 | KEY_E | 0x5B | 4 | 4 |
| 28 | KEY_F | 0x5B | 10 | 5 |
| 39 | KEY_F# | 0x5A | 2 | 6 |
| 29 | KEY_G | 0x5B | 6 | 7 |
| 40 | KEY_G# | 0x5A | 3 | 8 |
| 30 | KEY_A | 0x5B | 11 | 9 |
| 41 | KEY_A# | 0x5A | 6 | 10 |
| 31 | KEY_B | 0x5B | 8 | 11 |

**MIDI note = 84 + semitone_offset + (octave − 6) × 12**
Chromatic layout: C–B (white and black keys interleaved by LED index)

## Octave Control

The rotary encoder (currently index 0) shifts the keyboard octave across **0–7**.
Turning the encoder calls `retriggerHeldNotes()` to re-send any currently held keyboard notes at the new pitch.

## Gesture Detection

The `MPR121_GestureHelper` wraps raw capacitance reads with:

- **Debounce** — 10ms dead zone after any state change
- **Long press** — fires after 300ms of continuous contact
- **Double tap** — two taps within 200ms of release

Gestures fire the `gestureCallback`; simple touch/release fires `touchCallback`. The TouchHandler dispatches to the correct handler (pad / control / keyboard) based on the `SensorMap` lookup table.

## Display Layout

```
┌─────────────────────────────────┐
│ MiDeensy            <octave>    │  ← yellow header (16px)
│ v0.1                            │
├─────────────────────────────────┤
│                                 │
│          Main zone              │  ← blue area (48px)
│        (4 lines available)      │
│                                 │
└─────────────────────────────────┘
```

- Header: `headerLeft()` / `headerRight()` — 5×7 font, 2 lines
- Main: `mainLine()` — 6×10 font, up to 4 lines; `mainClear()` to wipe

## Project Structure

```
MiDeensy.ino          ← setup() / loop()
src/
  Globals.h           ← LED count, pin
  Mapping.h           ← SensorMap table (single source of truth)
  Midi.{h,cpp}        ← usbMIDI.read()
  TouchHandler.{h,cpp}← dispatch touch → MIDI + LED
  MPR121_GestureHelper.{h,cpp}  ← debounce, long-press, double-tap
  NoteLED.{h,cpp}     ← NeoPixel on/off
  Encoder.{h,cpp}     ← rotary encoder polling
  Display.{h,cpp}     ← SSD1306 OLED helpers
```
