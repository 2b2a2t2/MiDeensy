#include "Display.h"
#include "Sequencer.h"
#include "VoiceManager.h"
#include "ChordTimeline.h"

extern SeqFunction seqFunction;
extern uint8_t selectedSlot;
extern uint8_t currentKey;
extern ScaleType currentScale;
extern VoiceManager voiceManager;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2Display(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);

void MyU8G2_DisplayInterface::drawBackground() {
  setTextSize(1);
  setTextColor(WHITE);

  setCursor(0, 12);
  if (lastActiveMode == BANK_SEQ) {
    print("Keys:");
    setCursor(30, 12);
    print("OCT:");
    print(keyboardOctave);
    setCursor(60, 12);
    // Show active SEQ function
    switch (seqFunction) {
      case SEQ_THEME:   print("THEME"); break;
      case SEQ_HARMONY: print("HARM"); break;
      case SEQ_RHYTHM:  print("RHYTH"); break;
      case SEQ_VOICE:   print("VOICE"); break;
      default:          print("SEQ"); break;
    }
    setCursor(95, 12);
    print("S");
    print(selectedSlot + 1);
  } else {
    print("Keys:");
    setCursor(40, 12);
    print("OCT:");
    print(keyboardOctave);
    setCursor(80, 12);
    print("Enc:");
  }

  drawLine(0, 15, 127, 15, WHITE);
}

void MyU8G2_DisplayInterface::displayBankLabels() {
  fillRect(0, 17, 128, 47, BLACK);
  setTextSize(1);
  setTextColor(WHITE);

  setCursor(0, 22);
  print("Keys Bank: ");
  print(bankKeys.getSelection() + 1);

  setCursor(0, 32);
  print("Enc Bank: ");
  print(bankEnc.getSelection() + 1);

  setCursor(0, 42);
  print("Mode: ");
  switch (currentBankMode) {
    case BANK_KEYS: print("Keyboard"); break;
    case BANK_ENC:  print("Encoders"); break;
    case BANK_SEQ:  print("Sequencer"); break;
    case BANK_NONE: print("Normal"); break;
  }

  display();
}

void MyU8G2_DisplayInterface::displayNormalMode() {
  fillRect(0, 17, 128, 47, BLACK);
  setTextSize(1);
  setTextColor(WHITE);

  if (lastActiveMode == BANK_ENC || lastActiveMode == BANK_SEQ) {
    return;
  }

  setCursor(0, 22);
  print("Normal Mode Active");

  setCursor(0, 32);
  print("Pads: Notes 59-74 (CH1)");

  setCursor(0, 42);
  print("Keys: Notes 84-95 (CH2)");

  setCursor(0, 52);
  print("Hold KEY/ENC for banks");

  display();
}

void MyU8G2_DisplayInterface::displaySequencerMode() {
  fillRect(0, 17, 128, 47, BLACK);
  setTextSize(1);
  setTextColor(WHITE);

  setCursor(0, 22);
  print("SEQ: ");
  switch (seqFunction) {
    case SEQ_THEME:   print("THEME"); break;
    case SEQ_HARMONY: print("HARMONY"); break;
    case SEQ_RHYTHM:  print("RHYTHM"); break;
    case SEQ_VOICE:   print("VOICE"); break;
    default:          print("Select F1-F4"); break;
  }

  // Show slot and channel info
  setCursor(0, 32);
  print("Slot:");
  print(selectedSlot + 1);
  print("  Ch:");
  print(voiceManager.getActiveChannel());

  // Show role
  setCursor(0, 42);
  print("Role: ");
  switch (voiceManager.getActiveRole()) {
    case ROLE_DRUMS: print("Drums"); break;
    case ROLE_BASS:  print("Bass"); break;
    case ROLE_PAD:   print("Pad"); break;
    case ROLE_ARP:   print("Arp"); break;
    case ROLE_LEAD:  print("Lead"); break;
    default:         print("None"); break;
  }

  // Show key/scale
  setCursor(0, 52);
  print("Key:");
  print(currentKey);
  print(" Scale:");
  print(currentScale);

  display();
}

void MyU8G2_DisplayInterface::displayMessage(const char* message) {
  clear();
  setTextSize(2);
  setTextColor(WHITE);
  setCursor(10, 35);
  println(message);
  display();
}

MyU8G2_DisplayInterface display = u8g2Display;

// ===== EncoderDisplayElement =====

void EncoderDisplayElement::draw() {
  if (lastActiveMode != BANK_ENC) return;

  display.setTextColor(WHITE);
  display.setTextSize(1);

  for (int i = 0; i < 8; i++) {
    int col = i % 4;
    int row = i / 4;
    int x = col * 32;
    int y = 18 + row * 22;
    uint16_t val = lastEncoderValues[i];

    display.drawFastVLine(x + 1, y, 18, WHITE);

    uint8_t fillHeight = map(val, 0, 127, 0, 16);
    uint8_t fillTop = y + 1 + (16 - fillHeight);
    display.fillRect(x + 2, fillTop, 2, fillHeight, WHITE);

    display.setCursor(x + 9, y + 4);
    display.print(i + 1);

    display.setCursor(x + 9, y + 12);
    display.print(val);
  }
}

bool EncoderDisplayElement::getDirty() const {
  return lastActiveMode == BANK_ENC;
}

EncoderDisplayElement encDisplay{ display };
