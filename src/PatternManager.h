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
  uint8_t activePatternBrightness = 0xFF;

  bool patternAutoRotate = false;
  unsigned long patternTimeout = 40*1000;

  std::vector<Pattern * (*)(void)> patternConstructors;

  BufferType &ctx;

  HardwareControls controls;
  TouchButton *touchPads[3] = {0};

  SpokePatternManager *spokeManager;

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
      this->ctx.leds.fadeToBlackBy(15);
    });

    ctx.leds.fill_solid(CRGB::Black);

    DrawModal(120, 1200, [this](unsigned long elapsed) {
      this->ctx.leds.fadeToBlackBy(15);
      for (unsigned c = 0; c < circleleds.size(); ++c) {
        uint8_t fadeUp = min(0xFFu, 0xFF * elapsed / 250);
        uint8_t fadeDown = 0xFF - 0xFF * max(0, (long)elapsed - 1000) / 200;
        this->ctx.leds[circleleds[c]] = CHSV(0xFF * (c+elapsed/30) / circleleds.size(), 0xFF, scale8(fadeUp, fadeDown));
      }
    });
  }

  void patternAutorotateWelcome() {
    DrawModal(120, 100, [this](unsigned long elapsed) {
      this->ctx.leds.fadeToBlackBy(15);
    });

    const int eachFadeUpDuration = 800;
    const int enterDuration = 500;
    
    ctx.leds.fill_solid(CRGB::Black);

    FlagColorManager<CRGBPalette32> flag(4); // pride flag
    const int flagSegments = flag.trackedColorsCount();
    std::vector<CRGB> flagColors;
    for (int s = 0; s < flagSegments; ++s) {
      flagColors.push_back(flag.getTrackedColor(s));
    }

    DrawModal(120, enterDuration + eachFadeUpDuration, [this, flagSegments, flagColors](unsigned elapsed) {
      int maxSegment = min(flagSegments, flagSegments * (int)elapsed / enterDuration);
      for (int segment = 0; segment < maxSegment; ++segment) {
        CRGB color = flagColors[segment];
        unsigned long start = segment * enterDuration / flagSegments;
        if (elapsed > start && elapsed < start + eachFadeUpDuration) {
          // set segment brightness based on sin [0,pi]
          uint8_t brightness = sin16((elapsed - start) * 0x7FFF / eachFadeUpDuration) >> 8;
          color.nscale8(dim8_raw(brightness));
        } else {
          color = CRGB::Black;
        }

        // light the segment
        unsigned segmentStart = segment * circleleds.size() / flagSegments;
        unsigned segmentEnd = (segment+1) * circleleds.size() / flagSegments;
        for (unsigned i = segmentStart; i < segmentEnd; ++i) {
          this->ctx.leds[circleleds[(i + circleIndexOppositeVenus) % circleleds.size()]] = color;
        }
      }
    });
    ctx.leds.fill_solid(CRGB::Black);
    FastLED.show();
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
      this->enablePatternAutoRotate();
    });

    // Colors Button
    buttons[1]->onSinglePress([this]() {
      this->nextPalette();
    });
    buttons[1]->onDoublePress([this]() {
      this->previousPalette();
    });
    buttons[1]->onLongPress([this]() {
      this->enablePaletteAutoRotate();
    });
    
    // EVM Touch Pads
    touchPads[0] = controls.addTouchButton(TOUCH_PIN_1);
    touchPads[1] = controls.addTouchButton(TOUCH_PIN_2);
    touchPads[2] = controls.addTouchButton(TOUCH_PIN_3);

    for (int b = 0; b < 3; ++b) {
      unsigned long chargeDuration = touchPads[b]->longPressInterval;
      touchPads[b]->onButtonDown([b, this]() {
        logf("Touch down: %i", b);
        if (spokeManager) {
          spokeManager->spokeTapDown(b);
        }
        
      });
      touchPads[b]->onButtonUp([b, chargeDuration, this]() {
        logf("Touch up: %i", b);
        if (spokeManager) {
          spokeManager->spokeTapUp(b, chargeDuration);
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
      spokeManager->stopAllSpokes();
    });
    buttons[1]->onSinglePress([this]() {
      this->nextPalette();
    });
    buttons[1]->onDoubleLongPress([this]() {
      this->enablePaletteAutoRotate();
    });
    buttons[2]->onSinglePress([this]() {
      this->nextPattern();
      spokeManager->stopAllSpokes();
    });
    for (int b = 0; b < 3; ++b) {
      buttons[b]->onLongPress([b, this]() {
        spokeManager->runSpoke(b);
      });
    }
#endif
    controls.update();
  }

public:
  EVMColorManager *colorManager;

  PatternManager(BufferType &ctx) : ctx(ctx) {
    patternConstructors.push_back(&(construct<DownstreamPattern>));
    patternConstructors.push_back(&(construct<DownstreamFilledPattern>));
    patternConstructors.push_back(&(construct<CouplingPattern>));
    patternConstructors.push_back(&(construct<IntersexFlagPattern>));
    patternConstructors.push_back(&(construct<SoundBits>));
    patternConstructors.push_back(&(construct<HeartBeatPattern>));
    
    // patternConstructors.push_back(&(construct<SoundTest>));
  }

  ~PatternManager() {
    delete activePattern;
    delete colorManager;
  #if EVM_HARDWARE_VERSION > 1
    delete spokeManager;
  #endif
  }

  void nextPattern() {
    bool spokeChange = false;
    // for touched spokes, change the pattern for those spokes only
    if (spokeManager) {
      for (int s = 0; s < 3; ++s) {
        if (touchPads[s] && touchPads[s]->isButtonPressed()) {
          spokeChange = true;
          spokeManager->nextPattern(s);
        }
      }
    }
    if (!spokeChange) {
      patternAutoRotate = false;
      patternIndex = addmod8(patternIndex, 1, patternConstructors.size());
      if (!startPatternAtIndex(patternIndex)) {
        nextPattern();
      }
    }
  }

  void previousPattern() {
    bool spokeChange = false;
    // for touched spokes, change the pattern for those spokes only
    // FIXME: factor this block out cause I have it repeated 4x
    if (spokeManager) {
      for (int s = 0; s < 3; ++s) {
        if (touchPads[s] && touchPads[s]->isButtonPressed()) {
          spokeChange = true;
          spokeManager->previousPattern(s);
        }
      }
    }
    if (!spokeChange) {
      patternAutoRotate = false;
      patternIndex = mod_wrap(patternIndex - 1, patternConstructors.size());
      if (!startPatternAtIndex(patternIndex)) {
        previousPattern();
      }
    }
  }

  void enablePatternAutoRotate() {
    logf("Enable pattern autorotate");
    patternAutoRotate = true;
    patternAutorotateWelcome();
    stopPattern();
  }

  void stopPattern() {
    if (activePattern) {
      activePattern->stop();
      delete activePattern;
      activePattern = NULL;
    }
  }
  
  // Palettes

  void nextPalette() {
    bool spokeChange = false;
    // for touched spokes, change only the palettes for those spokes
    if (spokeManager) {
      for (int s = 0; s < 3; ++s) {
        if (touchPads[s] && touchPads[s]->isButtonPressed()) {
          spokeChange = true;
          spokeManager->nextPalette(s);
        }
      }
    }

    if (!spokeChange) {
      colorManager->pauseRotation = true;
      colorManager->nextPalette();
      if (activePattern) {
        activePattern->colorModeChanged();
      }
    }
    if (spokeManager) {
      spokeManager->colorModeChanged();
    }
  }

  void previousPalette() {
    bool spokeChange = false;
    // for touched spokes, change only the palettes for those spokes
    if (spokeManager) {
      for (int s = 0; s < 3; ++s) {
        if (touchPads[s] && touchPads[s]->isButtonPressed()) {
          spokeChange = true;
          spokeManager->previousPalette(s);
        }
      }
    }
    
    if (!spokeChange) {
      colorManager->pauseRotation = true;
      colorManager->previousPalette();
      if (activePattern) {
        activePattern->colorModeChanged();
      }
    }
    if (spokeManager) {
      spokeManager->colorModeChanged();
    }
  }

  void enablePaletteAutoRotate() {
    logf("Enable palette autorotate");
    colorManager->pauseRotation = false;
    paletteAutorotateWelcome();
    colorManager->randomizePalette();
    if (activePattern) {
      activePattern->colorModeChanged();
    }
    if (spokeManager) {
      spokeManager->colorModeChanged();
    }
  }

private:
  bool startPatternAtIndex(int index) {
    stopPattern();
    auto ctor = patternConstructors[index];
    Pattern *nextPattern = ctor();
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
    stopPattern();
    if (pattern->wantsToRun()) {
      colorManager->resetFlagColors();
      pattern->colorManager = colorManager;
      pattern->colorModeChanged();
      pattern->start();
      activePattern = pattern;
      return true;
    } else {
      return false;
    }
  }

  void setup() {
    assert(colorManager == NULL, "colorManager is not null");
    colorManager = new EVMColorManager();
    spokeManager = new SpokePatternManager();
#if EVM_HARDWARE_VERSION > 1
    spokeManager->colorManager = colorManager;
#endif
    setupButtons();

    startPatternAtIndex(0);
  }

  void loop() {
    ctx.leds.fill_solid(CRGB::Black);

    if (spokeManager && !spokeManager->isRunning()) {
      spokeManager->colorModeChanged();
      spokeManager->start();
    }

    if (activePattern) {
      // fade out the active pattern somewhat while a spoke is running, or entirely if all three spokes are running
      uint8_t drawingSpokeCount = (spokeManager ? spokeManager->drawingSpokeCount() : 0);
      const uint8_t minPartialBrightness = 0x6F;
      if (drawingSpokeCount == 3) {
        if (activePatternBrightness > 0) {
          --activePatternBrightness;
        }
      } else if (drawingSpokeCount > 0 && activePatternBrightness > minPartialBrightness) {
        activePatternBrightness -= 2;
      } else if (drawingSpokeCount == 0 || activePatternBrightness < minPartialBrightness) {
        activePatternBrightness = qadd8(activePatternBrightness, 4);
      }

      if (activePatternBrightness > 0) {
        activePattern->loop();
        activePattern->ctx.blendIntoContext(ctx, BlendMode::blendBrighten, dim8_raw(activePatternBrightness));
      }
    }

    if (spokeManager) {
      spokeManager->loop();
      spokeManager->ctx.blendIntoContext(ctx, BlendMode::blendBrighten);
      spokeManager->subtractCtx.blendIntoContext(ctx, BlendMode::blendSubtract);
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
