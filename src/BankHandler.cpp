#include "BankHandler.h"
#include "Display.h"
#include "Sequencer.h"

extern Array<CRGB, NUM_LEDS> leds;
extern Bank<16> bankKeys;
extern Bank<16> bankEnc;
extern BankMode currentBankMode;
extern BankMode lastActiveMode;
extern bool modeButtonHeld;
extern bool seqLEDsActive;
extern uint8_t selectedSlot;
extern SeqFunction seqFunction;

void BankLEDHandler::updateBankLEDs() {
  if (currentBankMode == BANK_SEQ) {
    // In SEQ mode, let the sequencer manage its own LEDs
    sequencer.updateStepLEDs();
  } else {
    for (int i = 0; i < 16; i++) {
      leds[i] = CRGB::Black;
    }
  }
  leds[SEQ_LED_INDEX] = CRGB::Black;

  switch (currentBankMode) {
    case BANK_KEYS:
      if (bankKeys.getSelection() < 16)
        leds[bankKeys.getSelection()] = CRGB::SeaGreen;
      break;
    case BANK_ENC:
      if (bankEnc.getSelection() < 16)
        leds[bankEnc.getSelection()] = CRGB::Purple;
      break;
    case BANK_SEQ:
      leds[SEQ_LED_INDEX] = CRGB::Blue;
      break;
    case BANK_NONE:
      break;
  }

  FastLED.show();
}

void BankLEDHandler::enterBankMode(BankMode mode) {
  currentBankMode = mode;
  lastActiveMode = mode;
  modeButtonHeld = true;

  if (mode == BANK_SEQ) {
    seqLEDsActive = true;
    seqFunction = SEQ_NONE;  // no function layer selected yet
  }

  updateBankLEDs();
  display.drawBackground();

  if (mode == BANK_KEYS) {
    display.displayBankLabels();
  } else if (mode == BANK_SEQ) {
    display.displaySequencerMode();
  }
}

void BankLEDHandler::exitBankMode() {
  currentBankMode = BANK_NONE;
  modeButtonHeld = false;
  seqFunction = SEQ_NONE;

  // Don't kill the sequencer — it keeps running in the background
  // Only clear the mode indicator LED; step LEDs stay managed by the sequencer
  leds[SEQ_LED_INDEX] = CRGB::Black;

  if (!sequencer.isPlaying()) {
    // If sequencer isn't playing, clear step LEDs
    for (int i = 0; i < 16; i++) {
      leds[i] = CRGB::Black;
    }
    seqLEDsActive = false;
  }

  FastLED.show();

  if (lastActiveMode != BANK_ENC && lastActiveMode != BANK_SEQ) {
    display.displayNormalMode();
  }
}
