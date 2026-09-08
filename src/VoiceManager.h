#pragma once

#include "Globals.h"

struct SeqSlot {
  uint8_t midiChannel;    // 1-16 (MIDI channel)
  VoiceRole role;         // DRUMS, BASS, PAD, ARP, LEAD, NONE
  bool enabled;
  bool locked;            // prevents EVOLVE from changing this slot's content
};

class VoiceManager {
public:
  VoiceManager();

  void begin();

  // Slot configuration
  SeqSlot& getSlot(uint8_t idx) { return slots_[idx]; }
  const SeqSlot& getSlot(uint8_t idx) const { return slots_[idx]; }
  void setSlotChannel(uint8_t idx, uint8_t channel);
  void setSlotRole(uint8_t idx, VoiceRole role);
  void setSlotEnabled(uint8_t idx, bool enabled);
  void setSlotLocked(uint8_t idx, bool locked);

  // Active slot (selected via SEQ+PAD)
  uint8_t getActiveSlot() const { return activeSlot_; }
  void setActiveSlot(uint8_t idx) { activeSlot_ = idx < 16 ? idx : 0; }

  // Get MIDI channel for the active slot
  uint8_t getActiveChannel() const;

  // Get MIDI channel for a specific slot
  uint8_t getChannel(uint8_t idx) const;

  // Get role for the active slot
  VoiceRole getActiveRole() const;

private:
  SeqSlot slots_[16];
  uint8_t activeSlot_;
};
