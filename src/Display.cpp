#include "Display.h"

// Header: u8g2_font_5x7_tr — 5px wide, 7px tall, fits 2 lines in 16px
// Main: u8g2_font_6x10_tr — 6px wide, 10px tall, fits 4 lines in 48px

static constexpr uint8_t HEADER_Y_LINE1 = 7;   // baseline of first header line
static constexpr uint8_t HEADER_Y_LINE2 = 15;  // baseline of second header line
static constexpr uint8_t MAIN_Y_START = 16;     // main zone top
static constexpr uint8_t MAIN_LINE_H = 12;      // line height in main zone

void Oled::begin() {
  u8g2.begin();
  u8g2.setFlipMode(0);
  clear();
  headerLeft("MiDeensy", "v0.1");
  show();
}

void Oled::clear() {
  u8g2.clearBuffer();
}

void Oled::headerLeft(const char* line1, const char* line2) {
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, HEADER_Y_LINE1, line1);
  if (line2) {
    u8g2.drawStr(0, HEADER_Y_LINE2, line2);
  }
}

void Oled::headerRight(const char* line1, const char* line2) {
  u8g2.setFont(u8g2_font_5x7_tr);
  if (line1) {
    uint16_t w = u8g2.getStrWidth(line1);
    u8g2.drawStr(128 - w, HEADER_Y_LINE1, line1);
  }
  if (line2) {
    uint16_t w = u8g2.getStrWidth(line2);
    u8g2.drawStr(128 - w, HEADER_Y_LINE2, line2);
  }
}

void Oled::mainLine(uint8_t row, const char* text) {
  u8g2.setFont(u8g2_font_6x10_tr);
  uint8_t y = MAIN_Y_START + (row + 1) * MAIN_LINE_H;
  u8g2.drawStr(0, y, text);
}

void Oled::mainClear() {
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, MAIN_Y_START, 128, 64 - MAIN_Y_START);
  u8g2.setDrawColor(1);
}

void Oled::show() {
  u8g2.sendBuffer();
}

void Oled::showEncoders(int8_t values[8]) {
  mainClear();
  u8g2.setFont(u8g2_font_6x10_tr);
  char buf[16];
  for (uint8_t i = 0; i < 4; i++) {
    snprintf(buf, sizeof(buf), "E%d:%3d  E%d:%3d", i, values[i], i + 4, values[i + 4]);
    mainLine(i, buf);
  }
}
