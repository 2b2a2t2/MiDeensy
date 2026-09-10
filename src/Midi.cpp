#include "Midi.h"

void midiBegin() {
}

void midiUpdate() {
  usbMIDI.read();
}
