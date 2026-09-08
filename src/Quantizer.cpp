#include "Quantizer.h"

Quantizer::Quantizer() {
  clear();
}

void Quantizer::queueKeyChange(uint8_t key, uint8_t scale) {
  pending_.type = PENDING_KEY_CHANGE;
  pending_.active = true;
  pending_.data.keyScale.key = key;
  pending_.data.keyScale.scale = scale;
}

void Quantizer::queueDensityChange(uint8_t density) {
  pending_.type = PENDING_DENSITY_CHANGE;
  pending_.active = true;
  pending_.data.density.density = density;
}

void Quantizer::queueChordChange(uint16_t step, uint8_t degree, uint8_t quality) {
  pending_.type = PENDING_CHORD_CHANGE;
  pending_.active = true;
  pending_.data.chord.step = step;
  pending_.data.chord.degree = degree;
  pending_.data.chord.quality = quality;
}

PendingChange Quantizer::consume() {
  PendingChange result = pending_;
  pending_.active = false;
  pending_.type = PENDING_NONE;
  return result;
}

void Quantizer::clear() {
  pending_.type = PENDING_NONE;
  pending_.active = false;
}
