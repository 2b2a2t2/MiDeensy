// file "MPR121_GestureHelper.h"
#ifndef MPR121_GESTURE_HELPER_H
#define MPR121_GESTURE_HELPER_H

#include <Wire.h>
#include <Adafruit_MPR121.h>

class MPR121_GestureHelper {
public:
  MPR121_GestureHelper();

  void begin(uint16_t longPressTime = 300, uint16_t doubleTapTime = 200, uint16_t debounceTime = 10);
  bool addSensor(uint8_t address);
  void setThresholds(uint8_t sensorIndex, uint8_t touch, uint8_t release);
  void update();

  typedef void (*TouchEventCallback)(uint8_t sensorIndex, uint8_t channel, bool touched);
  typedef void (*GestureCallback)(uint8_t sensorIndex, uint8_t channel, const char* gestureName, unsigned long duration);

  void onTouchEvent(TouchEventCallback callback);
  void onGesture(GestureCallback callback);
  void setDebug(bool enable);

private:
  struct TouchChannel {
    bool currentState;
    bool lastState;
    bool longPressActive;
    bool doubleTapPossible;
    bool justHadDoubleTap;
    unsigned long pressTime;
    unsigned long releaseTime;
  };

  struct Sensor {
    Adafruit_MPR121 cap;
    uint16_t lastState;
    uint16_t currentState;
    uint8_t address;
    bool initialized;
    const char* name;
  };

  static const uint8_t MAX_SENSORS = 4;
  static const uint8_t CHANNELS_PER_SENSOR = 12;

  Sensor sensors[MAX_SENSORS];
  TouchChannel channels[MAX_SENSORS * CHANNELS_PER_SENSOR];
  uint8_t sensorCount;

  uint16_t LONG_PRESS_TIME;
  uint16_t DOUBLE_TAP_TIME;
  uint16_t DEBOUNCE_TIME;

  TouchEventCallback touchCallback;
  GestureCallback gestureCallback;
  bool debugEnabled;

  void processSensor(uint8_t sensorIndex);
  void processChannel(uint8_t sensorIndex, uint8_t channel, bool isTouched);
  int getChannelIndex(uint8_t sensorIndex, uint8_t channel);
};

#endif
