#pragma once

#include <U8g2lib.h>
#include <Wire.h>

// SSD1306 128x64 I2C OLED
// Yellow strip: top 16px (rows 0-15) — "header"
// Blue area: rows 16-63 — "main"
class Oled {
public:
  void begin();
  void clear();

  // Header zone (yellow strip, 16px = 2 lines of 8px font)
  void headerLeft(const char* line1, const char* line2 = nullptr);
  void headerRight(const char* line1, const char* line2 = nullptr);

  // Main zone (blue area, 48px)
  void mainLine(uint8_t row, const char* text);
  void mainClear();
  void showEncoders(int8_t values[8]);

  // Force send buffer to screen
  void show();

  U8G2& raw() { return u8g2; }

private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2{U8G2_R0};
};
