#ifndef CONTROLS_H
#define CONTROLS_H

#include <vector>

class HardwareControl {
  friend class HardwareControls;
protected:
  virtual void update() = 0;
  int pin;
  void handleHandler(void (*handler)(void)) {
    if (handler) {
      (*handler)();
    }
  }

  template<typename T>
  void handleHandler(void (*handler)(T), T arg) {
    if (handler) {
      (*handler)(arg);
    }
  }
public:
  HardwareControl(int pin) : pin(pin) {};
  virtual ~HardwareControl() {};
};

/* ------------------ s*/

class AnalogDial : public HardwareControl {
  void (*changeHandler)(int);
  int lastValue = -1;
  unsigned long lastChange;

  void update() {
    int thumbdial1 = analogRead(pin);
    // potentiometer reads are noisy, jitter may be around ±30 or so. 
    // Wait for significant change to notify the handler, but then still allow smooth updates as the pot is turned
    bool significantChange = abs(lastValue - thumbdial1) > updateThreshold;
    bool recentSignificantChange = (millis() - lastChange < smoothUpdateDuration);
    bool endpointsChange = lastValue != thumbdial1 && (thumbdial1 == 0 || thumbdial1 == 1023);
    
    if (significantChange || recentSignificantChange || endpointsChange) {
      if (significantChange || endpointsChange) {
        lastChange = millis();
        lastValue = thumbdial1;
      }
      handleHandler(changeHandler, thumbdial1);
    }
  }

public:
  int updateThreshold = 40; // Value threshold to suppress jitter in the thumbdial
  unsigned smoothUpdateDuration = 500; // Duration (ms) to call handler on every change after threshold is met

  AnalogDial(int pin) : HardwareControl(pin) {
  }

  void onChange(void (*handler)(int)) {
    changeHandler = handler;
  }
};

/* ----------------------- */

class SPSTButton : public HardwareControl {
  long buttonDownTime = -1;
  long buttonUpTime = -1;
  long singlePressTime = -1;
  bool waitForButtonUp = false;
  
  void (*singlePressHandler)(void);
  void (*doublePressHandler)(void);
  void (*longPressHandler)(void);
  void (*doubleLongPressHandler)(void);

  void update() {
    bool buttonPressed = digitalRead(pin) == LOW;
    long readTime = millis();

    if (waitForButtonUp) {
      if (!buttonPressed) {
        waitForButtonUp = false;
      }
    } else {
      if (!buttonPressed && singlePressTime != -1) {
        if (readTime - singlePressTime > doublePressInterval) {
          // double-press timeout
          handleHandler(singlePressHandler);
          singlePressTime = -1;
        }
      }
      if (!buttonPressed && buttonDownTime != -1) {
        if (singlePressTime != -1) {
          // button-up from second press
          handleHandler(doublePressHandler);
          singlePressTime = -1;
        } else {
          singlePressTime = readTime;
        }
      } else if (buttonPressed && buttonDownTime == -1) {
        buttonDownTime = readTime;
      } else if (buttonPressed && readTime - buttonDownTime > longPressInterval) {
        if (singlePressTime != -1) {
          handleHandler(doubleLongPressHandler);
          singlePressTime = -1;
        } else {
          handleHandler(longPressHandler);
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

  void onSinglePress(void (*handler)(void)) {
    singlePressHandler = handler;
  }
 
  void onDoublePress(void (*handler)(void)) {
    doublePressHandler = handler;
  }

  void onLongPress(void (*handler)(void)) {
    longPressHandler = handler;
  }

  void onDoubleLongPress(void (*handler)(void)) {
    doubleLongPressHandler = handler;
  }
};

/* --------------------- */

class HardwareControls {
private:
  std::vector<HardwareControl *> controlsVec;
public:
  ~HardwareControls() {
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
