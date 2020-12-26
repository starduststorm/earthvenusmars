#ifndef PATTERN_H
#define PATTERN_H

#include <FastLED.h>

#include "util.h"
#include "palettes.h"
#include "ledgraph.h"

class Pattern {
private:  
  long startTime = -1;
  long stopTime = -1;
  long lastUpdateTime = -1;
public:
  virtual ~Pattern() { }

  void start() {
    logf("Starting %s", description());
    startTime = millis();
    stopTime = -1;
    setup();
  }

  void loop(CRGBArray<NUM_LEDS> &leds) {
    update(leds);
    lastUpdateTime = millis();
  }

  virtual bool wantsToIdleStop() {
    return true;
  }

  virtual bool wantsToRun() {
    // for idle patterns that require microphone input and may opt not to run if there is no sound
    return true;
  }

  virtual void setup() { }

  void stop() {
    logf("Stopping %s", description());
    startTime = -1;
  }

  virtual void update(CRGBArray<NUM_LEDS> &leds) { }
  
  virtual const char *description() = 0;

public:
  bool isRunning() {
    return startTime != -1;
  }

  long runTime() {
    return startTime == -1 ? 0 : millis() - startTime;
  }

  long frameTime() {
    return (lastUpdateTime == -1 ? 0 : millis() - lastUpdateTime);
  }
};

/* ------------------------------------------------------------------------------------------------------ */

typedef enum { idle, birthing, growing, shrinking, } BitStatus;

struct FlowingBit {
private:
  BitStatus status;
public:
  uint8_t progress;
  uint8_t hue;
  unsigned long statusStart;
  void setStatus(BitStatus status) {
    this->status = status;
    statusStart = millis();
    progress = (status == shrinking ? 0xFF : (status == growing ? 0 : progress));
  }
  BitStatus getStatus() {
    return this->status;
  }

  const char *statusString() {
    switch (getStatus()) {
      case idle: return "idle"; break;
      case birthing: return "birthing"; break;
      case growing: return "growing"; break;
      case shrinking: return "shrinking"; break;
    }
  }
};

class DownstreamPattern : public Pattern {
  int direction;
  FlowingBit flowing[NUM_LEDS];
public:

  void setup() {
    memset(flowing, 0, NUM_LEDS * sizeof(FlowingBit));

    direction = random8() & 1 ? 1 : -1;

    for (int i = 0; i < 2; ++i) {
      uint8_t hue = random8();
      //ARRAY_SAMPLE(circleleds);
      int start = (i == 0 ? 0 : ARRAY_SIZE(circleleds) / 2);
      flowing[circleleds[start]].setStatus(shrinking);
      flowing[circleleds[start]].hue = hue;

      int next = mod_wrap(start + direction, ARRAY_SIZE(circleleds));
      flowing[circleleds[next]].setStatus(growing);
      flowing[circleleds[next]].hue = hue;
    }
  }

  void update(CRGBArray<NUM_LEDS> &leds) {
    unsigned long mils = millis();
    
    for (int i = 0; i < NUM_LEDS; ++i) {
      FlowingBit *bit = &flowing[i];
      vector<int> adj = ledgraph.adjacent_to(i);
      
      const int growTime = beatsin8(10, 30, 60);//ms
      const int shrinkTime = beatsin16(12, 300, 500);//ms
      switch (bit->getStatus()) {
        case growing:
          bit->progress = min((long unsigned int)0xFF, (long unsigned)(0xFF * (mils - bit->statusStart) / (float)growTime));
          if (bit->progress == 0xFF) {
            bit->setStatus(idle);
          }
          break;
        case shrinking:
          bit->progress = 0xFF - min((long unsigned int)0xFF, (long unsigned)(0xFF * (mils - bit->statusStart) / (float)shrinkTime));
          if (bit->progress == 0) {
            bit->setStatus(idle);
          }
          break;
        case birthing: break;
        case idle:
          if (bit->progress == 0xFF) {
            for (int nearby : adj) {
              FlowingBit *nearbyBit = &flowing[nearby];
              if (nearbyBit->getStatus() == idle && nearbyBit->progress == 0) {
                nearbyBit->setStatus(birthing);
                nearbyBit->hue = mod_wrap(bit->hue + 5 * direction, 0xFF);
              }
            }
            bit->setStatus(shrinking);
          }
          break;
      }
    }

    for (int i = 0; i < NUM_LEDS; ++i) {
      FlowingBit *bit = &flowing[i];
      if (bit->getStatus() == birthing) {
        bit->setStatus(growing);
      }
      CRGB color = CHSV(bit->hue, 0xFF, 0xFF);
      leds[i] = color.nscale8(dim8_raw(bit->progress));
    }
  }

  const char *description() {
    return "downstream";
  }
};

#endif

