#ifndef PATTERNMANAGER_H
#define PATTERNMANAGER_H

#include <vector>

#include "patterns.h"
#include "ledgraph.h"
#include "controls.h"
#include "config.h"

template <typename BufferType>
class PatternManager {
  int patternIndex = -1;
  Pattern *activePattern = NULL;

  bool patternAutoRotate = false;
  unsigned long patternTimeout = 40*1000;

  std::vector<Pattern * (*)(void)> patternConstructors;

  BufferType &pixelBuffer;

  HardwareControls controls;

  ChargePattern *chargePattern;

  template<class T>
  static Pattern *construct() {
    return new T();
  }

  // Make testIdlePattern in this constructor instead of at global so the Pattern doesn't get made at launch
  Pattern *TestIdlePattern() {
    static Pattern *testIdlePattern = NULL;
    if (testIdlePattern == NULL) {
      // testIdlePattern = new LitPattern();
      // testIdlePattern = new UpstreamPattern();
      // testIdlePattern = new SoundTest2();
      // testIdlePattern = new DownstreamPattern();
    }
    return testIdlePattern;
  }

  void paletteAutorotateWelcome() {
    DrawModal(120, 100, [this](unsigned long elapsed) {
      this->pixelBuffer.leds.fadeToBlackBy(15);
    });
    DrawModal(120, 1200, [this](unsigned long elapsed) {
      this->pixelBuffer.leds.fadeToBlackBy(15);
      for (unsigned c = 0; c < circleleds.size(); ++c) {
        uint8_t fadeUp = min(0xFFu, 0xFF * elapsed / 250);
        uint8_t fadeDown = 0xFF - 0xFF * max(0, (long)elapsed - 1000) / 200;
        this->pixelBuffer.leds[circleleds[c]] = CHSV(0xFF * (c+elapsed/30) / circleleds.size(), 0xFF, scale8(fadeUp, fadeDown));
      }
    });
  }

  void setupButtons() {
#if EVM_HARDWARE_VERSION > 1
    SPSTButton *buttons[2];
    buttons[0] = controls.addButton(BUTTON_PIN_1);
    buttons[1] = controls.addButton(BUTTON_PIN_2);

    // Patterns Button
    buttons[0]->onSinglePress([this]() {
      this->nextPattern();
    });
    buttons[0]->onDoublePress([this]() {
      this->previousPattern();
    });
    buttons[0]->onLongPress([this]() {
      this->togglePatternAutoRotate();
    });

    // Colors Button
    buttons[1]->onSinglePress([this]() {
      this->nextPalette();
    });
    buttons[1]->onDoublePress([this]() {
      this->previousPalette();
    });
    buttons[1]->onLongPress([this]() {
      this->togglePaletteAutoRotate();
      if (!this->colorManager->pauseRotation) {
        paletteAutorotateWelcome();
      }
    });
    
    // EVM Touch Pads
    TouchButton *touchPads[3];
    touchPads[0] = controls.addTouchButton(TOUCH_PIN_1);
    touchPads[1] = controls.addTouchButton(TOUCH_PIN_2);
    touchPads[2] = controls.addTouchButton(TOUCH_PIN_3);

    for (int b = 0; b < 3; ++b) {
      unsigned long chargeDuration = touchPads[b]->longPressInterval;
      touchPads[b]->onButtonDown([b, this]() {
        logf("touch down: %i", b);
        if (chargePattern) {
          chargePattern->chargeSpoke(b);
        }
        
      });
      touchPads[b]->onButtonUp([b, chargeDuration, this]() {
        logf("touch up: %i", b);
        if (chargePattern) {
          chargePattern->stopChargingSpoke(b, chargeDuration);
        }
      });
      touchPads[b]->onLongPress([b, this]() {
        logf("long press: %i", b);
        if (chargePattern) {
          chargePattern->runSpoke(b);
        }
      });
    }
#else
    SPSTButton *buttons[3];
    buttons[0] = controls.addButton(BUTTON_PIN_1);
    buttons[1] = controls.addButton(BUTTON_PIN_2);
    buttons[2] = controls.addButton(BUTTON_PIN_3);

    buttons[0]->onSinglePress([this]() {
      this->previousPattern();
    });
    buttons[1]->onSinglePress([this]() {
      this->nextPalette();
    });
    buttons[1]->onDoubleLongPress([this]() {
      this->togglePaletteAutoRotate();
      if (!this->colorManager->pauseRotation) {
        paletteAutorotateWelcome();
      }
    });
    buttons[2]->onSinglePress([this]() {
      this->nextPattern();
    });

    for (int b = 0; b < 3; ++b) {
      buttons[b]->onLongPress([b, this]() {
        ChargePattern *charge = new ChargePattern();
        charge->spoke = b;
        this->startPattern(charge);
      });
    }
#endif
    controls.update();
  }

public:
  EVMColorManager *colorManager;

  PatternManager(BufferType &pixelBuffer) : pixelBuffer(pixelBuffer) {
    patternConstructors.push_back(&(construct<DownstreamPattern>));
    patternConstructors.push_back(&(construct<CouplingPattern>));
    patternConstructors.push_back(&(construct<IntersexFlagPattern>));
    patternConstructors.push_back(&(construct<SoundBits>));
    patternConstructors.push_back(&(construct<HeartBeatPattern>));
    
    // patternConstructors.push_back(&(construct<SoundTest>));
  }

  ~PatternManager() {
    delete activePattern;
    delete colorManager;
  }

  void nextPattern() {
    patternAutoRotate = false;
    patternIndex = addmod8(patternIndex, 1, patternConstructors.size());
    if (!startPatternAtIndex(patternIndex)) {
      nextPattern();
    }
  }

  void previousPattern() {
    patternAutoRotate = false;
    patternIndex = mod_wrap(patternIndex - 1, patternConstructors.size());
    if (!startPatternAtIndex(patternIndex)) {
      previousPattern();
    }
  }

  void togglePatternAutoRotate() {
    logf("Toggle pattern autorotate");
    patternAutoRotate = !patternAutoRotate;
  }

  void stopPattern() {
    logf("stopPattern");
    if (activePattern) {
      activePattern->stop();
      delete activePattern;
      activePattern = NULL;
    }
  }
  
  // Palettes

  void nextPalette() {
    colorManager->pauseRotation = true;
    colorManager->nextPalette();
    if (activePattern) {
      activePattern->colorModeChanged();
    }
    if (chargePattern) {
      chargePattern->colorModeChanged();
    }
  }

  void previousPalette() {
    colorManager->pauseRotation = true;
    colorManager->previousPalette();
    if (activePattern) {
      activePattern->colorModeChanged();
    }
    if (chargePattern) {
      chargePattern->colorModeChanged();
    }
  }

  void togglePaletteAutoRotate() {
    colorManager->togglePaletteAutoRotate();
    if (activePattern) {
      activePattern->colorModeChanged();
    }
    chargePattern->colorModeChanged();
  }

private:
  bool startPatternAtIndex(int index) {
    logf("startPatternAtIndex %i", index);
    logf("calling stop pattern first");
    stopPattern();
    auto ctor = patternConstructors[index];
    logf("got ctor");
    Pattern *nextPattern = ctor();
    logf("called ctor");
    if (startPattern(nextPattern)) {
      patternIndex = index;
      return true;
    } else {
      delete nextPattern; // patternConstructors returns retained
      return false;
    }
  }
public:
  bool startPattern(Pattern *pattern) {
    logf("startPattern");
    stopPattern();
    if (pattern->wantsToRun()) {
      pattern->colorManager = colorManager;
      pattern->colorModeChanged();
      pattern->start();
      activePattern = pattern;

      if (chargePattern) {
        chargePattern->stopAllSpokes();
      }

      return true;
    } else {
      return false;
    }
  }

  void setup() {
    colorManager = new EVMColorManager();
    chargePattern = new ChargePattern();
    chargePattern->colorManager = colorManager;
    setupButtons();
  }

  void loop() {
    pixelBuffer.leds.fill_solid(CRGB::Black);

    if (!chargePattern->isRunning()) {
      chargePattern->colorModeChanged();
      chargePattern->start();
    }

    if (activePattern) {
      activePattern->loop(pixelBuffer.ctx);
    }
    if (chargePattern) {
      chargePattern->loop(pixelBuffer.ctx);
    }

    // time out idle patterns
    if (patternAutoRotate && activePattern != NULL && activePattern->isRunning() && activePattern->runTime() > patternTimeout) {
      if (activePattern != TestIdlePattern() && activePattern->wantsToIdleStop()) {
        activePattern->stop();
        delete activePattern;
        activePattern = NULL;
      }
    }

    // start a new random pattern if there is none
    if (activePattern == NULL) {
      Pattern *testPattern = TestIdlePattern();
      if (testPattern) {
        startPattern(testPattern);
      } else {
        int choice = (int)random8(patternConstructors.size());
        startPatternAtIndex(choice);
      }
    }
    controls.update();
  }
};

#endif
