#pragma once

#include <cstdint>

constexpr uint8_t NUM_ENCODERS = 8;
constexpr uint8_t ENC_CC_BASE = 16;  // CC 16-23

void encBegin();
int8_t encUpdate(uint8_t index);
int8_t encValue(uint8_t index);
