#ifndef CONTROLS_H
#define CONTROLS_H

#include <vector>
#include <functional>

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

  void update() {
    bool buttonPressed = digitalRead(pin) == LOW;
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

  SPSTButton(int pin) : HardwareControl(pin) {
    pinMode(pin, INPUT_PULLUP);
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
