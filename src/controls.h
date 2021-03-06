#ifndef CONTROLS_H
#define CONTROLS_H

#include <vector>
#include <functional>
#include "Adafruit_FreeTouch.h"

class HardwareControl {
  friend class HardwareControls;
protected:
  virtual void update() = 0;
  int pin;
public:
  HardwareControl(int pin) : pin(pin) {};
  virtual ~HardwareControl() {};
};

/* ------------------ s*/

typedef std::function<void(uint32_t)> DialHandler;

class AnalogDial : public HardwareControl {
  DialHandler changeHandler = [](uint32_t){};

  uint32_t lastValue = UINT32_MAX;
  unsigned long lastChange;

  void update() {
    uint32_t value;
    if (readValueFunc) {
      value = (*readValueFunc)();
    } else {
      value = analogRead(pin);
    }
    // potentiometer reads are noisy, jitter may be around ±30 or so. 
    // Wait for significant change to notify the handler, but then still allow smooth updates as the pot is turned
    bool significantChange = abs((int)lastValue - (int)value) > updateThreshold;
    bool recentSignificantChange = (millis() - lastChange < smoothUpdateDuration);
    bool endpointsChange = lastValue != value && (value == 0 || value == maxValue);
    
    if (significantChange || recentSignificantChange || endpointsChange) {
      // logf("pot update: significantChange = %i [%u - %u > %u], recentSignificantChange = %i, endpointsChange = %i", significantChange, lastValue, value, updateThreshold, recentSignificantChange, endpointsChange);
      if (significantChange || endpointsChange) {
        lastChange = millis();
        lastValue = value;
      }
      changeHandler(value);
    }
  }

public:
  unsigned updateThreshold = 40; // Value threshold to suppress jitter in the thumbdial
  unsigned smoothUpdateDuration = 500; // Duration (ms) to call handler on every change after threshold is met

  uint32_t (*readValueFunc)(void) = NULL;
  uint32_t maxValue = 1023;

  AnalogDial(int pin) : HardwareControl(pin) {
  }

  void onChange(DialHandler handler) {
    changeHandler = handler;
  }
};

/* ----------------------- */

typedef std::function<void(void)> ButtonHandler;

class SPSTButton : public HardwareControl {
  long buttonDownTime = -1;
  long buttonUpTime = -1;
  long singlePressTime = -1;
  bool waitForButtonUp = false;
  
  bool hasDoublePressHandler = false;
  
  std::function<void(void)> singlePressHandler = []{};
  std::function<void(void)> doublePressHandler = []{};
  std::function<void(void)> longPressHandler = []{};
  std::function<void(void)> doubleLongPressHandler = []{};
  std::function<void(void)> buttonUpHandler = []{}; // called on button-up only after longPress or doubleLongPress

  bool didInit = false;

  virtual void initPin(int pin) {
    pinMode(pin, INPUT_PULLUP);
  }

  void update() {
    if (!didInit) {
      initPin(pin);
      didInit = true;
    }
    bool buttonPressed = isButtonPressed();
    long readTime = millis();

    if (waitForButtonUp) {
      if (!buttonPressed) {
        waitForButtonUp = false;
        buttonUpHandler();
      }
    } else {
      if (!buttonPressed && singlePressTime != -1) {
        if (!hasDoublePressHandler || readTime - singlePressTime > doublePressInterval) {
          // double-press timeout
          singlePressHandler();
          singlePressTime = -1;
        }
      }
      if (!buttonPressed && buttonDownTime != -1) {
        if (singlePressTime != -1) {
          // button-up from second press
          doublePressHandler();
          singlePressTime = -1;
        } else {
            singlePressTime = readTime;
        }
      } else if (buttonPressed && buttonDownTime == -1) {
        buttonDownTime = readTime;
      } else if (buttonPressed && readTime - buttonDownTime > longPressInterval) {
        if (singlePressTime != -1) {
          doubleLongPressHandler();
          singlePressTime = -1;
        } else {
          longPressHandler();
        }
        waitForButtonUp = true;
      }
    }

    if (buttonPressed) {
      buttonUpTime = -1;
    } else {
      buttonDownTime = -1;
    }
  }

public:
  long longPressInterval = 1000;
  long doublePressInterval = 400;

  SPSTButton(int pin) : HardwareControl(pin) { }

  virtual bool isButtonPressed() {
    // HACK: on EVM hardware 2, the Colors button is wired to Arduino Zero's TX LED pin, which causes sporatic HIGH reads
#if EVM_HARDWARE_VERSION == 2
    noInterrupts();
    pinMode(pin, INPUT_PULLUP);
#endif
    int read = (digitalRead(pin) == LOW);
#if EVM_HARDWARE_VERSION == 2
    interrupts();
#endif
    return read;
  }

  void onSinglePress(ButtonHandler handler) {
    singlePressHandler = handler;
  }
 
  void onDoublePress(ButtonHandler handler) {
    doublePressHandler = handler;
    hasDoublePressHandler = true;
  }

  void onLongPress(ButtonHandler handler) {
    longPressHandler = handler;
  }

  void onDoubleLongPress(ButtonHandler handler) {
    doubleLongPressHandler = handler;
    hasDoublePressHandler = true;
  }

  void onButtonUp(ButtonHandler handler) {
    buttonUpHandler = handler;
  }
};

class TouchButton : public SPSTButton {
  Adafruit_FreeTouch *touchObj;
  void initPin(int pin) { } // no-op, skip SPSTButton init and do pin init in constructor
  bool touchRegistered = false;
public:
  uint16_t touchThreshold = 700; // sample above (10-bit) threshold will be considered touch down
  uint16_t touchReleaseBuffer = 100; // sample must fall below threshold-buffer for touch release

  TouchButton(int pin, oversample_t oversample, series_resistor_t seriesResistor, freq_mode_t freqMode) : SPSTButton(pin) {
    touchObj = new Adafruit_FreeTouch(pin, oversample, seriesResistor, freqMode);
    touchObj->begin();
  }

  ~TouchButton() {
    delete touchObj;
  }

  bool isButtonPressed() {
    uint16_t sample = touchObj->measure();
    if (sample == (uint16_t)-1) {
      logf("touch sample failed");
    }

    assert(touchThreshold > touchReleaseBuffer, "touchThreshold must be > touchReleaseBuffer");
    if (sample > touchThreshold) {
      touchRegistered = true;
    } else if (sample < (uint32_t)(touchThreshold - touchReleaseBuffer)) {
      touchRegistered = false;
    }
    return touchRegistered;
  }

  int measure() {
    return touchObj->measure();
  }
};

/* --------------------- */

class HardwareControls {
private:
  std::vector<HardwareControl *> controlsVec;
public:
  ~HardwareControls() {
    for (auto control : controlsVec) {
      delete control;
    }
    controlsVec.clear();
  }

  SPSTButton *addButton(int pin) {
    SPSTButton *button = new SPSTButton(pin);
    controlsVec.push_back(button);
    return button;
  }

  TouchButton *addTouchButton(int pin, oversample_t oversample=OVERSAMPLE_4, series_resistor_t seriesResistor=RESISTOR_0, freq_mode_t freqMode=FREQ_MODE_NONE) {
    TouchButton *button = new TouchButton(pin, oversample, seriesResistor, freqMode);
    controlsVec.push_back(button);
    return button;
  }

  AnalogDial *addAnalogDial(int pin) {
    AnalogDial *dial = new AnalogDial(pin);
    controlsVec.push_back(dial);
    return dial;
  }

  void update() {
    for (HardwareControl *control : controlsVec) {
      control->update();
    }
  }
};

#endif
