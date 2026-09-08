#pragma once

#include "Globals.h"

class BankLEDHandler {
public:
  static void updateBankLEDs();
  static void enterBankMode(BankMode mode);
  static void exitBankMode();
};

constexpr uint8_t SEQ_LED_INDEX = 33;
