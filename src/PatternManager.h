#ifndef PATTERNMANAGER_H
#define PATTERNMANAGER_H

#include <vector>

#include "patterns.h"
#include "ledgraph.h"

class PatternManager {
  int patternIndex = -1;
  Pattern *activePattern = NULL;

  bool patternAutoRotate = false;
  unsigned long patternTimeout = 40*1000;

  std::vector<Pattern * (*)(void)> patternConstructors;

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

public:
  EVMColorManager *colorManager;

  PatternManager() {
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
  }

  void previousPalette() {
    colorManager->pauseRotation = true;
    colorManager->previousPalette();
    if (activePattern) {
      activePattern->colorModeChanged();
    }
  }

  void togglePaletteAutoRotate() {
    colorManager->togglePaletteAutoRotate();
    if (activePattern) {
      activePattern->colorModeChanged();
    }
  }

private:
  bool startPatternAtIndex(int index) {
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
    colorManager = new EVMColorManager();
  }

  template<typename BufferType>
  void loop(BufferType &pixelBuffer) {
    if (activePattern) {
      activePattern->loop(pixelBuffer.ctx);
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
  }
};

#endif
