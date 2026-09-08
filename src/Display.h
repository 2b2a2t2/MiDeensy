#pragma once

#include "Globals.h"
#include "DisplayInterfaceU8G2.hpp"

class MyU8G2_DisplayInterface : public U8G2_DisplayInterface {
public:
  MyU8G2_DisplayInterface(U8G2& display)
    : U8G2_DisplayInterface(display) {}

  void begin() override {
    disp.begin();
    disp.setFlipMode(0);
    U8G2_DisplayInterface::begin();
  }

  void drawBackground() override;
  void displayBankLabels();
  void displayNormalMode();
  void displaySequencerMode();
  void displayMessage(const char* message);
};

extern MyU8G2_DisplayInterface display;

// Display element for encoder bars
class EncoderDisplayElement : public DisplayElement {
public:
  EncoderDisplayElement(DisplayInterface &display)
    : DisplayElement(display) {}
  void draw() override;
  bool getDirty() const override;
};

extern EncoderDisplayElement encDisplay;
