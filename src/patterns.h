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


// a lil patternlet that can be instantiated to run basic bits
// can this be enhanced to cover flowing bits as well?
// can bit direction be 'towards' a spoke or the center? 
class BitsFiller {
  struct Bit {
    int px;
    int direction;
    int life;
    CRGB color;
    Bit(int px, int direction, int lifespan, CRGB color) : px(px), direction(direction), life(lifespan), color(color) {};
  };
public:
  enum BitDirection {
    forward,
    backward,
    bidirectional,
    towards, // towards what, extra argument?
  };

  vector<int> *pixels;
  vector<Bit> bits;
  CRGBPalette256 palette;
  int lifespan;
  BitsFiller(int numBits, int speed /*(units?)*/, int lifespan, BitDirection direction, CRGBPalette256 palette, vector<int> *pixels)
    : pixels(pixels), palette(palette), lifespan(lifespan) {
    bits.reserve(numBits);
    for (int b = 0; b < numBits; ++b) {
      addBit();
    }
  };
  void update(CRGBArray<NUM_LEDS> &leds) {
    // FIXME: implement speed & direction logic
    EVERY_N_MILLIS(16) {
      for (int b = bits.size() - 1; b >= 0; --b) {
        Bit &bit = bits[b];
         bit.px = mod_wrap(bit.px + bit.direction, pixels->size());
         // FIXME: life should be time, not ticks
         bit.life -= 1;
         if (bit.life == 0) {
           bits.erase(bits.begin() + b);
           addBit();
         }
      }
    }
    for (Bit &bit : bits) {
      leds[pixels->at(bit.px)] = bit.color;
    }
  };

  void addBit(int index=-1) {
    if (index < 0) {
      index = random16(pixels->size());
    }
    int direction = random8()&1 ? 1 : -1;
    Bit newbit(index, direction, lifespan, ColorFromPalette(palette, random8()));
    bits.push_back(newbit);
  }
};

// FIXME: does not wake from sleep properly
class DownstreamPattern : public Pattern {
  int direction;
  FlowingBit flowing[NUM_LEDS];
public:

  void setup() {
    memset(flowing, 0, NUM_LEDS * sizeof(FlowingBit));

    direction = random8() & 1 ? 1 : -1;

    for (int i = 0; i < 2; ++i) {
      uint8_t hue = random8();
      int start = (i == 0 ? 0 : circleleds.size() / 2);
      flowing[circleleds[start]].setStatus(shrinking);
      flowing[circleleds[start]].hue = hue;

      int next = mod_wrap(start + direction, circleleds.size());
      flowing[circleleds[next]].setStatus(growing);
      flowing[circleleds[next]].hue = hue + 0x7F;
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
      // CRGB color = ColorFromPalette((CRGBPalette256)Trans_Flag_gp, bit->hue);
      leds[i] = color.nscale8(dim8_raw(bit->progress));
    }
  }

  const char *description() {
    return "downstream";
  }
};

class CouplingPattern : public Pattern {
  BitsFiller *bits;
public:
  CouplingPattern() {
    bits = new BitsFiller(16, 1, 60, BitsFiller::bidirectional, Trans_Flag_gp, &circleleds);
  }
  ~CouplingPattern() {
    delete bits;
  }
  void setup() { }

  void update(CRGBArray<NUM_LEDS> &leds) {
    vector<int> *planetspokelists[] = {&venusleds, &marsleds};
    vector<int> *earthspokelists[] = {&earthleds, &earthasmarsleds, &earthasvenusleds};

    EVERY_N_MILLIS(1000) {
      leds.fill_solid(CRGB::Black);

      // pick two
      vector<int> *spokes[2] = {0};
      do {
        if (random8(2) == 0) {
          spokes[0] = ARRAY_SAMPLE(earthspokelists);
        } else {
          spokes[0] = ARRAY_SAMPLE(planetspokelists);
        }
        spokes[1] = ARRAY_SAMPLE(planetspokelists);
      } while (spokes[0] == spokes[1]);

      for (int s = 0; s < 2; ++s) {
        vector<int> *spoke = spokes[s];
        // pick a color/palette for each
        if (true || random8(2) == 0) {
          CRGB color = ColorFromPalette((CRGBPalette256)Trans_Flag_gp, random8());
          for (int i : *spoke) {
            leds[i] = color;
          }
        } else {
          CRGBPalette256 palette = Trans_Flag_gp;//ARRAY_SAMPLE(flag_palettes);
          for (unsigned i = 0; i < spoke->size(); ++i) {
            leds[spoke->at(i)] = ColorFromPalette(palette, 0xFF * i / spoke->size());
          }
        }
      }
    };
    EVERY_N_MILLIS(8) {
      for (int i : circleleds) {
        leds[i].fadeToBlackBy(0xFF);
      }
    }
    bits->update(leds);
  }

  const char *description() {
    return "coupling";
  }
};

#endif

