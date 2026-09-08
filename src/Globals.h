#pragma once

#include <FastLED.h>
#include <Control_Surface.h>

#define BLACK 0
#define WHITE 1

// LED hardware config
constexpr uint8_t LED_PIN = 6;
constexpr uint8_t NUM_LEDS = 42;

// Pad MIDI notes (Channel 1)
constexpr uint8_t PAD_NOTE_BASE = 59;

// Keyboard MIDI notes (Channel 2+bank)
constexpr uint8_t KEY_NOTE_BASE = 84;

// Control button MIDI notes
constexpr uint8_t CTRL_NOTE_BASE = 75;

// Bank modes
enum BankMode {
  BANK_NONE,
  BANK_KEYS,
  BANK_ENC,
  BANK_SEQ
};

// SEQ function layers (F1-F4 in SEQ mode)
enum SeqFunction {
  SEQ_NONE,       // no function selected (default on entering SEQ)
  SEQ_THEME,      // F1: theme parameters (energy, tension)
  SEQ_HARMONY,    // F2: chord progression editing
  SEQ_RHYTHM,     // F3: rhythm generation
  SEQ_VOICE       // F4: voice configuration
};

// Voice roles for SEQ slots
enum VoiceRole : uint8_t {
  ROLE_NONE = 0,
  ROLE_DRUMS,
  ROLE_BASS,
  ROLE_PAD,
  ROLE_ARP,
  ROLE_LEAD
};

// Scale types for harmony engine
enum ScaleType : uint8_t {
  SCALE_MAJOR = 0,
  SCALE_MINOR,        // natural minor (Aeolian)
  SCALE_DORIAN,
  SCALE_MIXOLYDIAN,
  SCALE_HARMONIC_MINOR,
  SCALE_MELODIC_MINOR,
  SCALE_COUNT
};

// Intervals for each scale type (semitones from root, 7 notes)
constexpr uint8_t SCALE_INTERVALS[SCALE_COUNT][7] = {
  { 0, 2, 4, 5, 7, 9, 11 },  // MAJOR
  { 0, 2, 3, 5, 7, 8, 10 },  // MINOR (natural)
  { 0, 2, 3, 5, 7, 9, 10 },  // DORIAN
  { 0, 2, 4, 5, 7, 9, 10 },  // MIXOLYDIAN
  { 0, 2, 3, 5, 7, 8, 11 },  // HARMONIC MINOR
  { 0, 2, 3, 5, 7, 9, 11 },  // MELODIC MINOR
};

// Default chord quality for each scale degree (index 0-6)
// 0=MAJ, 1=MIN, 2=DIM, 3=AUG, 4=DOM7, 5=MIN7, 6=MAJ7
enum ChordQuality : uint8_t {
  QUALITY_AUTO = 0,
  MAJ, MIN, DIM, AUG, DOM7, MIN7, MAJ7
};

constexpr uint8_t DEFAULT_CHORD_QUALITY[SCALE_COUNT][7] = {
  { MAJ, MIN, MIN, MAJ, MAJ, MIN, DIM },  // MAJOR: I ii iii IV V vi vii°
  { MIN, DIM, MAJ, MIN, MIN, MAJ, MAJ },  // MINOR: i ii° III iv v VI VII
  { MIN, MIN, MAJ, MAJ, MIN, DIM, MAJ },  // DORIAN
  { MAJ, MIN, DIM, MAJ, MIN, MIN, MAJ },  // MIXOLYDIAN
  { MIN, DIM, AUG, MIN, MAJ, MAJ, DIM },  // HARMONIC MINOR
  { MIN, MIN, AUG, MAJ, MAJ, DIM, DIM },  // MELODIC MINOR
};

// Button types
enum ButtonType {
  TYPE_PAD,
  TYPE_CONTROL,
  TYPE_KEYBOARD
};

struct ButtonMap {
  uint8_t sensor;
  uint8_t channel;
  const char* name;
  ButtonType type;
  uint8_t midiNote;
  bool isPressed;
  unsigned long pressTime;
  bool isModeButton;
};

// Extern globals
extern Bank<16> bankKeys;
extern Bank<16> bankEnc;
extern BankMode currentBankMode;
extern BankMode lastActiveMode;
extern bool modeButtonHeld;
extern uint16_t lastEncoderValues[8];
extern Array<CRGB, NUM_LEDS> leds;
extern const byte ledMapping[NUM_LEDS];
extern int8_t keyboardOctave;
extern bool seqLEDsActive;
extern uint16_t lastHeldChord;
extern uint16_t lastChord;
extern uint8_t selectedSlot;
extern SeqFunction seqFunction;
extern uint8_t currentKey;       // root MIDI note (0-11), default 60 (C)
extern ScaleType currentScale;   // current scale type, default MAJOR
