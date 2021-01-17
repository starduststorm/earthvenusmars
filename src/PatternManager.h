#ifndef PATTERNMANAGER_H
#define PATTERNMANAGER_H

#include <vector>

#include "patterns.h"
#include "ledgraph.h"

/* ---- Test Options ---- */
const bool kTestPatternTransitions = false;
const long kIdlePatternTimeout = -1;//1000 * (kTestPatternTransitions ? 20 : 60 * 2);

Pattern *testIdlePattern = NULL;//new CouplingPattern();

class PatternManager {
  int patternIndex = -1;
  Pattern *activePattern = NULL;

  std::vector<Pattern * (*)(void)> patternConstructors;

  template<class T>
  static Pattern *construct() {
    return new T();
  }

public:
  PatternManager() {
    patternConstructors.push_back(&(construct<DownstreamPattern>));
    patternConstructors.push_back(&(construct<CouplingPattern>));
    patternConstructors.push_back(&(construct<ChargePattern>));
  }

  void nextPattern() {
    stopPattern();

    patternIndex = (patternIndex + 1) % patternConstructors.size();
    if (!startPatternAtIndex(patternIndex)) {
      nextPattern();
    }
  }

  void stopPattern() {
    if (activePattern) {
      activePattern->stop();
      delete activePattern;
      activePattern = NULL;
    }
  }

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

  bool startPattern(Pattern *pattern) {
    if (pattern->wantsToRun()) {
      pattern->start();
      activePattern = pattern;
      return true;
    } else {
      return false;
    }
  }

  template<typename BufferType>
  void loop(BufferType &pixelBuffer) {
    if (activePattern) {
      activePattern->loop(pixelBuffer.leds);
    }

    // time out idle patterns
    if (activePattern != NULL && kIdlePatternTimeout != -1 && activePattern->isRunning() && activePattern->runTime() > kIdlePatternTimeout) {
      if (activePattern != testIdlePattern && activePattern->wantsToIdleStop()) {
        activePattern->stop();
        delete activePattern;
        activePattern = NULL;
      }
    }

    // start a new random pattern if there is none
    if (activePattern == NULL) {
      if (testIdlePattern) {
        startPattern(testIdlePattern);
      } else {
        int choice = (int)random8(patternConstructors.size());
        startPatternAtIndex(choice);
      }
    }
  }
};

#endif
