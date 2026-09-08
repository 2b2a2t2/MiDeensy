#include "NoteLED.h"

const byte ledMapping[NUM_LEDS] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 37, 26, 38, 27, 28, 39, 29, 40, 30, 41, 31,
  32, 33, 34, 35, 36
};

CustomNoteLED<NUM_LEDS> midiled{ leds.data, ledMapping, MIDI_Notes::B[3] };
