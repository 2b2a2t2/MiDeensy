#pragma once

#include "Globals.h"

// Global flag: when true, midiled skips pad LEDs (0-15) so the sequencer owns them
extern bool seqLEDsActive;

template<uint8_t RangeLen>
class CustomNoteLED : public MatchingMIDIInputElement<MIDIMessageType::NoteOn,
                                                      TwoByteRangeMIDIMatcher> {
public:
  CustomNoteLED(CRGB *ledcolors, const uint8_t *ledIndexMap, MIDIAddress address)
    : MatchingMIDIInputElement<MIDIMessageType::NoteOn,
                               TwoByteRangeMIDIMatcher>({ address, RangeLen }),
      ledcolors(ledcolors), ledIndexMap(ledIndexMap) {}

  void begin() override {}
  void handleUpdate(typename TwoByteRangeMIDIMatcher::Result match) override {
    updateLED(match.index, match.value);
  }

  void updateLED(uint8_t index, uint8_t velocity) {
    if (index >= totalNotes) return;
    // When sequencer is active, don't touch pad LEDs 0-15
    if (seqLEDsActive && index < 16) return;
    uint8_t ledIndex = ledIndexMap[index];
    if (velocity > 0) {
      ledcolors[ledIndex] = CHSV(map(velocity, 0, 127, 0, 255), 255, 255);
    } else {
      ledcolors[ledIndex] = CRGB::Black;
    }
    dirty = true;
  }

  bool getDirty() const { return dirty; }
  void clearDirty() { dirty = false; }

private:
  CRGB *ledcolors;
  const uint8_t *ledIndexMap;
  bool dirty = false;
  static const uint8_t totalNotes = NUM_LEDS;
};

extern const byte ledMapping[NUM_LEDS];
extern CustomNoteLED<NUM_LEDS> midiled;
