#include "Display.h"
#include "Sequencer.h"
#include "VoiceManager.h"
#include "ChordTimeline.h"

extern SeqFunction seqFunction;
extern uint8_t selectedSlot;
extern uint8_t currentKey;
extern ScaleType currentScale;
extern VoiceManager voiceManager;
extern struct SeqEncoderParams seqParams;
extern KeyLayer currentKeyLayer;
extern EncLayer currentEncLayer;
extern uint16_t timelineWindowOffset;
extern uint16_t keyEditStep;
extern bool noteVsChord;
extern uint8_t globalVelocity;
extern ChordTimeline chordTimeline;
extern const uint8_t encPrimaryCC[8];
extern const uint8_t encExtendedCC[8];

// Forward declarations for helper functions
const char* chordDegreeToString(uint16_t step);
const char* chordQualityToString(uint16_t step);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2Display(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);

void MyU8G2_DisplayInterface::drawBackground() {
  // Clear header area before redrawing to prevent leftover text from other modes
  fillRect(0, 0, 128, 16, BLACK);

  setTextSize(1);
  setTextColor(WHITE);

  setCursor(0, 12);
  if (currentBankMode == BANK_SEQ) {
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
  } else if (currentBankMode == BANK_KEYS) {
    print("Keys:");
    setCursor(30, 12);
    print("OCT:");
    print(keyboardOctave);
    setCursor(60, 12);
    print(currentKeyLayer == KEY_EXTENDED ? "KEY+" : "KEY");
    setCursor(95, 12);
    print("S");
    print(selectedSlot + 1);
  } else if (currentBankMode == BANK_ENC) {
    print("Keys:");
    setCursor(30, 12);
    print("OCT:");
    print(keyboardOctave);
    setCursor(60, 12);
    print(currentEncLayer == ENC_EXTENDED ? "ENC+" : "ENC");
    setCursor(95, 12);
    print("B");
    print(bankEnc.getSelection() + 1);
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

  if (currentBankMode == BANK_ENC || currentBankMode == BANK_SEQ || currentBankMode == BANK_KEYS) {
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

void MyU8G2_DisplayInterface::displayKeyMode() {
  fillRect(0, 17, 128, 47, BLACK);
  setTextSize(1);
  setTextColor(WHITE);

  setCursor(0, 22);
  print("KEY PRIMARY");
  setCursor(80, 22);
  print("Key:");
  print(currentKey);
  print(" ");
  print(currentScale);

  setCursor(0, 32);
  print("KeyEditStep:");
  print(keyEditStep);
  print(" Deg:");
  print(chordDegreeToString(keyEditStep));
  print(" Qual:");
  print(chordQualityToString(keyEditStep));

  setCursor(0, 42);
  print("Note/Chord:");
  print(noteVsChord ? "Chord" : "Note");
  print(" Vel:");
  print(globalVelocity);
  print(" Slot:");
  print(selectedSlot + 1);
  print(" Oct:");
  print(voiceManager.getSlotOctaveOffset(selectedSlot));

  display();
}

void MyU8G2_DisplayInterface::displayKeyExtendedMode() {
  fillRect(0, 17, 128, 47, BLACK);
  setTextSize(1);
  setTextColor(WHITE);

  setCursor(0, 22);
  print("KEY EXTENDED");
  setCursor(80, 22);
  print("Oct:");
  print(keyboardOctave);
  print(" Bank:");
  print(bankKeys.getSelection() + 1);

  setCursor(0, 32);
  print("Slot Oct:");
  print(voiceManager.getSlotOctaveOffset(selectedSlot));
  print(" Vel:");
  print(voiceManager.getSlotVelocity(selectedSlot));

  setCursor(0, 42);
  print("LoopLen:");
  print(chordTimeline.getLoopLength());
  print(" Slot:");
  print(selectedSlot + 1);

  display();
}

const char* chordDegreeToString(uint16_t step) {
  const ChordEvent* evt = chordTimeline.getEventStartingAtStep(step);
  if (!evt) return "--";
  switch (evt->degree) {
    case DEGREE_1: return "I";
    case DEGREE_2: return "II";
    case DEGREE_3: return "III";
    case DEGREE_4: return "IV";
    case DEGREE_5: return "V";
    case DEGREE_6: return "VI";
    case DEGREE_7: return "VII";
    default: return "--";
  }
}

const char* chordQualityToString(uint16_t step) {
  const ChordEvent* evt = chordTimeline.getEventStartingAtStep(step);
  if (!evt) return "--";
  switch (evt->quality) {
    case QUALITY_AUTO: return "AUTO";
    case MAJ: return "MAJ";
    case MIN: return "MIN";
    case DIM: return "DIM";
    case AUG: return "AUG";
    case DOM7: return "DOM7";
    case MIN7: return "MIN7";
    case MAJ7: return "MAJ7";
    default: return "--";
  }
}

void MyU8G2_DisplayInterface::displaySequencerMode() {
  fillRect(0, 17, 128, 47, BLACK);
  setTextSize(1);
  setTextColor(WHITE);

  setCursor(0, 22);
  print("SEQ ");
  switch (seqFunction) {
    case SEQ_THEME:   print("THEME"); break;
    case SEQ_HARMONY: print("HARM"); break;
    case SEQ_RHYTHM:  print("RHYTHM"); break;
    case SEQ_VOICE:   print("VOICE"); break;
    default:          print("Select F1-F4"); break;
  }
  print(" S");
  print(selectedSlot + 1);

  setCursor(0, 32);
  switch (seqFunction) {
    case SEQ_HARMONY:
      print("Step:");
      print(sequencer.getSelectedStep());
      print(" Dur:");
      print(seqParams.duration);
      print(" Inv:");
      print(seqParams.inversion);
      break;
    case SEQ_RHYTHM:
      print("Density:");
      print(seqParams.density);
      print(" Sw:");
      print(seqParams.swing);
      break;
    case SEQ_THEME:
      print("Energy:");
      print(seqParams.energy);
      print(" Tens:");
      print(seqParams.themeTension);
      break;
    case SEQ_VOICE:
      print("Oct:");
      print(seqParams.octaveOffset);
      print(" Vel:");
      print(seqParams.velocity);
      break;
    default:
      print("Ch:");
      print(voiceManager.getActiveChannel());
      break;
  }

  setCursor(0, 42);
  if (seqFunction == SEQ_VOICE) {
    print("Role: ");
    switch (voiceManager.getActiveRole()) {
      case ROLE_DRUMS: print("Drums"); break;
      case ROLE_BASS:  print("Bass"); break;
      case ROLE_PAD:   print("Pad"); break;
      case ROLE_ARP:   print("Arp"); break;
      case ROLE_LEAD:  print("Lead"); break;
      default:         print("None"); break;
    }
  } else {
    print("Key:");
    print(currentKey);
    print(" Scale:");
    print(currentScale);
  }

  display();
}

void MyU8G2_DisplayInterface::displayEncMode() {
  fillRect(0, 17, 128, 47, BLACK);
  setTextSize(1);
  setTextColor(WHITE);

  setCursor(0, 22);
  print("ENC PRIMARY");
  setCursor(80, 22);
  print("Bank:");
  print(bankEnc.getSelection() + 1);

  setCursor(0, 32);
  print("CC: 74 71 75 76 91 92 94  7");

  setCursor(0, 42);
  for (int i = 0; i < 8; i++) {
    if (i > 0) print(" ");
    print(lastEncoderValues[i]);
  }

  display();
}

void MyU8G2_DisplayInterface::displayEncExtendedMode() {
  fillRect(0, 17, 128, 47, BLACK);
  setTextSize(1);
  setTextColor(WHITE);

  setCursor(0, 22);
  print("ENC EXTENDED");
  setCursor(80, 22);
  print("Bank:");
  print(bankEnc.getSelection() + 1);

  setCursor(0, 32);
  print("CC: 16 17 18 19 80 81 82 83");

  setCursor(0, 42);
  for (int i = 0; i < 8; i++) {
    if (i > 0) print(" ");
    print(lastEncoderValues[i]);
  }

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
  if (currentBankMode != BANK_ENC) return;

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

    // Show CC number below value
    display.setCursor(x + 9, y + 20);
    uint8_t cc = (currentEncLayer == ENC_EXTENDED) ? encExtendedCC[i] : encPrimaryCC[i];
    display.print("CC");
    display.print(cc);
  }
}

bool EncoderDisplayElement::getDirty() const {
  return currentBankMode == BANK_ENC;
}

EncoderDisplayElement encDisplay{ display };
