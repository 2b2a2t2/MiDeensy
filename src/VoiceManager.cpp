#include "VoiceManager.h"

VoiceManager::VoiceManager() : activeSlot_(0) {
  begin();
}

void VoiceManager::begin() {
  for (uint8_t i = 0; i < 16; i++) {
    slots_[i].midiChannel = i + 1;  // channels 1-16
    slots_[i].role = ROLE_NONE;
    slots_[i].enabled = true;
    slots_[i].locked = false;
    slots_[i].octaveOffset = 0;
    slots_[i].velocity = 100;
  }
  // Default role assignments for first 5 slots
  slots_[0].role = ROLE_DRUMS;
  slots_[1].role = ROLE_BASS;
  slots_[2].role = ROLE_PAD;
  slots_[3].role = ROLE_ARP;
  slots_[4].role = ROLE_LEAD;
  activeSlot_ = 0;
}

void VoiceManager::setSlotChannel(uint8_t idx, uint8_t channel) {
  if (idx < 16 && channel >= 1 && channel <= 16) {
    slots_[idx].midiChannel = channel;
  }
}

void VoiceManager::setSlotRole(uint8_t idx, VoiceRole role) {
  if (idx < 16) {
    slots_[idx].role = role;
  }
}

void VoiceManager::setSlotEnabled(uint8_t idx, bool enabled) {
  if (idx < 16) {
    slots_[idx].enabled = enabled;
  }
}

void VoiceManager::setSlotLocked(uint8_t idx, bool locked) {
  if (idx < 16) {
    slots_[idx].locked = locked;
  }
}

void VoiceManager::setSlotOctaveOffset(uint8_t idx, int8_t offset) {
  if (idx < 16) {
    slots_[idx].octaveOffset = offset;
  }
}

void VoiceManager::setSlotVelocity(uint8_t idx, uint8_t velocity) {
  if (idx < 16) {
    slots_[idx].velocity = velocity;
  }
}

int8_t VoiceManager::getSlotOctaveOffset(uint8_t idx) const {
  if (idx < 16) return slots_[idx].octaveOffset;
  return 0;
}

uint8_t VoiceManager::getSlotVelocity(uint8_t idx) const {
  if (idx < 16) return slots_[idx].velocity;
  return 100;
}

uint8_t VoiceManager::getActiveChannel() const {
  return slots_[activeSlot_].midiChannel;
}

uint8_t VoiceManager::getChannel(uint8_t idx) const {
  if (idx < 16) return slots_[idx].midiChannel;
  return 1;
}

VoiceRole VoiceManager::getActiveRole() const {
  return slots_[activeSlot_].role;
}
