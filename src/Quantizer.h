#pragma once

#include "Globals.h"

enum PendingChangeType : uint8_t {
  PENDING_NONE = 0,
  PENDING_KEY_CHANGE,
  PENDING_SCALE_CHANGE,
  PENDING_DENSITY_CHANGE,
  PENDING_CHORD_CHANGE,
  PENDING_PROGRESSION_CHANGE
};

struct PendingChange {
  PendingChangeType type;
  bool active;
  union {
    struct { uint8_t key; uint8_t scale; } keyScale;
    struct { uint8_t density; } density;
    struct { uint16_t step; uint8_t degree; uint8_t quality; } chord;
  } data;
};

class Quantizer {
public:
  Quantizer();

  // Queue a pending change
  void queueKeyChange(uint8_t key, uint8_t scale);
  void queueDensityChange(uint8_t density);
  void queueChordChange(uint16_t step, uint8_t degree, uint8_t quality);

  // Check if there's a pending change that should be applied at this step
  bool hasPending() const { return pending_.active; }
  PendingChangeType pendingType() const { return pending_.type; }

  // Apply the pending change and clear it
  PendingChange consume();

  // Clear all pending changes
  void clear();

private:
  PendingChange pending_;
};
