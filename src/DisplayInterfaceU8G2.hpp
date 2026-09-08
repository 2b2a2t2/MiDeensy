#pragma once

#include <U8g2lib.h>
#include <Display/DisplayInterface.hpp>

BEGIN_CS_NAMESPACE

class U8G2_DisplayInterface : public DisplayInterface {
  protected:
    U8G2_DisplayInterface(U8G2 &display) : disp(display) {}

  public:
    void clear() override { disp.clearBuffer(); }
    void drawBackground() override = 0;
    void display() override { disp.sendBuffer(); }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        disp.setDrawColor(color);
        disp.drawPixel(x, y);
    }

    void setTextColor(uint16_t color) override {
        disp.setDrawColor(color);
    }
    void setTextSize(uint8_t size) override {
        switch (size) {
            case 1: disp.setFont(u8g2_font_5x7_tr); break;
            case 2: disp.setFont(u8g2_font_6x12_tr); break;
            case 3: disp.setFont(u8g2_font_7x14_tr); break;
            default: disp.setFont(u8g2_font_5x7_tr); break;
        }
    }
    void setCursor(int16_t x, int16_t y) override {
        disp.setCursor(x, y);
    }

    size_t write(uint8_t c) override { return disp.write(c); }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                  uint16_t color) override {
        disp.setDrawColor(color);
        disp.drawLine(x0, y0, x1, y1);
    }
    void drawFastVLine(int16_t x, int16_t y, int16_t h,
                       uint16_t color) override {
        disp.setDrawColor(color);
        disp.drawVLine(x, y, h);
    }
    void drawFastHLine(int16_t x, int16_t y, int16_t w,
                       uint16_t color) override {
        disp.setDrawColor(color);
        disp.drawHLine(x, y, w);
    }

    void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
                     int16_t h, uint16_t color) override {
        disp.setDrawColor(color);
        disp.drawXBM(x, y, w, h, bitmap);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                  uint16_t color) override {
        disp.setDrawColor(color);
        disp.drawBox(x, y, w, h);
    }

    void drawCircle(int16_t x0, int16_t y0, int16_t r,
                    uint16_t color) override {
        disp.setDrawColor(color);
        disp.drawCircle(x0, y0, r);
    }

    void fillCircle(int16_t x0, int16_t y0, int16_t r,
                    uint16_t color) override {
        disp.setDrawColor(color);
        disp.drawDisc(x0, y0, r);
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                  uint16_t color) {
        disp.setDrawColor(color);
        disp.drawFrame(x, y, w, h);
    }

  protected:
    U8G2 &disp;
};

END_CS_NAMESPACE
