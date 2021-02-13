#ifndef PATTERNMANAGER_H
#define PATTERNMANAGER_H

#include <vector>

#include "patterns.h"
#include "ledgraph.h"

/* ---- Test Options ---- */
const bool kTestPatternTransitions = false;
const long kIdlePatternTimeout = -1;//1000 * (kTestPatternTransitions ? 20 : 60 * 2);

class PatternManager {
  int patternIndex = -1;
  Pattern *activePattern = NULL;

  std::vector<Pattern * (*)(void)> patternConstructors;

  template<class T>
  static Pattern *construct() {
    return new T();
  }

  // Make testIdlePattern in this constructor instead of at global so the Pattern doesn't get made at launch
  Pattern *TestIdlePattern() {
    static Pattern *testIdlePattern = NULL;
    if (testIdlePattern == NULL) {
      testIdlePattern = new UpstreamPattern();
      // testIdlePattern = new SoundTest();
    }
    return testIdlePattern;
  }

public:
  PatternManager() {
    patternConstructors.push_back(&(construct<DownstreamPattern>));
    patternConstructors.push_back(&(construct<CouplingPattern>));
    patternConstructors.push_back(&(construct<IntersexFlagPattern>));
  }

  void nextPattern() {
    patternIndex = addmod8(patternIndex, 1, patternConstructors.size());
    if (!startPatternAtIndex(patternIndex)) {
      nextPattern();
    }
  }

  void previousPattern() {
    int ctorCount = patternConstructors.size();
    patternIndex = addmod8(patternIndex, ctorCount-1, ctorCount);
    if (!startPatternAtIndex(patternIndex)) {
      previousPattern();
    }
  }

  void stopPattern() {
    if (activePattern) {
      activePattern->stop();
      delete activePattern;
      activePattern = NULL;
    }
  }

  void poke() {
    if (activePattern) {
      logf("Poke pattern %s", activePattern->description());
      activePattern->poke();
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
      activePattern->loop(pixelBuffer.ctx);
    }

    // time out idle patterns
    if (activePattern != NULL && kIdlePatternTimeout != -1 && activePattern->isRunning() && activePattern->runTime() > kIdlePatternTimeout) {
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
