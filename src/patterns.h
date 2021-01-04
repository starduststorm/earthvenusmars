#ifndef PATTERN_H
#define PATTERN_H

#include <FastLED.h>
#include <vector>
#include <deque>

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

// a lil patternlet that can be instantiated to run bits
// TODO: requires universal fade-down. run in a display buffer to avoid that?
class BitsFiller {
public:
  typedef EdgeType BitDirection;
  typedef EdgeTypes BitDirections;
  typedef enum { random, priority, split } FlowRule;
  typedef enum { kill } LeafRule;
  typedef enum { maintainPopulation, manualSpawn } SpawnRule;
 
  struct Bit {
    int px;
    vector<BitDirection> directions;
    unsigned long birthmilli;
    unsigned long lifespan;
    uint8_t colorIndex;
    Bit(int px, vector<BitDirection> directions, unsigned long lifespan, uint8_t colorIndex) 
      : px(px), directions(directions), lifespan(lifespan), colorIndex(colorIndex) {
      reset();
      // logf("bit constructor for this = %p, px = %i, directions = %i, lifespan = %i", this, px, directions, lifespan);
    }
    Bit(const Bit &other) {
      reset_to(other);
    }
    void reset() {
      birthmilli = millis();
    }
    void reset_to(const Bit &other) {
      // logf("RESET BIT TO OTHER %p px = %i, directions = %i, lifespan = %i", &other, other.px, other.directions, other.lifespan);
      px = other.px;
      directions = other.directions;
      lifespan = other.lifespan;
      colorIndex = other.colorIndex;
      birthmilli = other.birthmilli;
    }
    unsigned long age() {
      return millis() - birthmilli;
    }
  };

private:

  vector<Bit> bits;
  unsigned long lastMove = 0;
  unsigned long lastColorChange = 0;

  int spawnLocation() {
    if (spawnPixels) {
      return spawnPixels->at(random8()%spawnPixels->size());
    }
    return random16()%NUM_LEDS;
  }

  Bit makeBit(Bit *fromBit=NULL) {
    // TODO: performance hit of doing this for every new bit? we chould cache the vectorized version of direction or just require a vector at the callsite instead of a bitfield.
    vector<BitDirection> directionsForBit;
    for (unsigned priority = 0; priority < bitDirections.size(); ++priority) {
      BitDirections directions = bitDirections[priority];
      vector<BitDirection> vectorized;
      for (unsigned i = 0; i < EdgeTypeCount; ++i) {
        if (directions & 1 << i) {
          vectorized.push_back((EdgeType)(1 << i));
        }
      }
      BitDirection direction = vectorized.at(random8()%vectorized.size());
      directionsForBit.push_back(direction);
    }

    if (fromBit) {
      return Bit(*fromBit);
    } else {
      return Bit(spawnLocation(), directionsForBit, lifespan, random8());
    }
  }

  void killBit(int bitIndex) {
    bits.erase(bits.begin() + bitIndex);
  }

  void splitBit(Bit &bit, int toIndex) {
    Bit split = makeBit(&bit);
    split.px = toIndex;
    bits.push_back(split);
  }

  vector<int> nextIndexes(int index, const vector<BitDirection> &bitDirections) {
    vector<int> next;
    switch (flowRule) {
      case priority:
        for (BitDirection direction : bitDirections) {
          auto adj = ledgraph.adjacencies(index, direction);
          if (adj.size() > 0) {
            next.push_back(adj.front().to);
            break;
          }
        }
        break;
      case random:
      case split: {
        vector<int> allNext;
        for (BitDirection direction : bitDirections) {
          auto adj = ledgraph.adjacencies(index, direction);
          for (auto a : adj) {
            allNext.push_back(a.to);
          }
        }
        if (flowRule == split) {
          next = allNext;
        } else {
          next.push_back(allNext.at(random8()%allNext.size()));
        }
        break;
      }
    }
    // TODO: does not handle duplicates in the case of the same vertex being reachable via multiple edges
    assert(next.size() <= 4, "no pixel in this design has more than 4 adjacencies but index %i had %u", index, next.size());
    return next;
  }

  bool flowBit(int bitIndex) {
    vector<int> next = nextIndexes(bits[bitIndex].px, bits[bitIndex].directions);
    if (next.size() == 0) {
      // leaf behavior
      switch (leafRule) {
        case kill:
          killBit(bitIndex);
          return false;
          break;
      }
    } else {
      bits[bitIndex].px = next.front();
      for (unsigned i = 1; i < next.size(); ++i) {
        splitBit(bits[bitIndex], next[i]);
      }
    }
    return true;
  }

public:
  void dumpBits() {
    logf("--------");
    logf("There are %i bits", bits.size());
    for (unsigned b = 0; b < bits.size(); ++b) {
      Bit &bit = bits[b];
      logf("Bit %i: px=%i, birthmilli=%lu, colorIndex=%u", b, bit.px, bit.birthmilli, bit.colorIndex);
      Serial.print("  Directions: ");
      for (BitDirection bd : bit.directions) {
        Serial.print((int)bd);
        Serial.print(", ");
      }
      Serial.println();
    }
    logf("--------");
  }

  unsigned numBits;
  unsigned speed; // in pixels/second
  unsigned long lifespan = 0; // in milliseconds, forever if 0
  vector<BitDirections> bitDirections; // vector of bitfield
  FlowRule flowRule = random;
  LeafRule leafRule = kill;
  SpawnRule spawnRule = maintainPopulation;
  unsigned fadeUpDistance = 0; // fade up n pixels ahead of bit motion
  
  CRGBPalette256 palette;
  unsigned long colorCycleDuration = 0; // ms to complete one cycle, 0 == no change
  
  vector<int> *spawnPixels = NULL; // list of pixels to automatically spawn bits on

  BitsFiller(unsigned numBits, unsigned speed, unsigned long lifespan, vector<BitDirections> bitDirections, CRGBPalette256 palette)
    : numBits(numBits), speed(speed), lifespan(lifespan), bitDirections(bitDirections), palette(palette) {
    bits.reserve(numBits);
  };

  void fadeUpForBit(Bit &bit, int px, int distanceAway, int distanceRemaining, unsigned long lastMove, CRGBArray<NUM_LEDS> &leds) {
    vector<int> next = nextIndexes(px, bit.directions);
    
    unsigned long mils = millis();
    unsigned long fadeUpDuration = 1000 * fadeUpDistance / speed;
    for (int n : next) {
      unsigned long fadeTimeSoFar = mils - lastMove + distanceRemaining * 1000/speed;
      uint8_t progress = 0xFF * fadeTimeSoFar / fadeUpDuration;

      CRGB color = ColorFromPalette(palette, bit.colorIndex);
      CRGB existing = leds[n];
      CRGB blended = blend(existing, color, dim8_raw(progress));
      leds[n] = blended;
      
      if (distanceRemaining > 0) {
        fadeUpForBit(bit, n, distanceAway+1, distanceRemaining-1, lastMove, leds);
      }
    }
  }

  void update(CRGBArray<NUM_LEDS> &leds) {
    // FIXME: global fadedown
    EVERY_N_MILLIS(16) {
      leds.fadeToBlackBy(60);
    }

    unsigned long mils = millis();
    if (mils - lastMove > 1000/speed) {
      for (int i = bits.size() - 1; i >= 0; --i) {
        bool bitAlive = flowBit(i);
        if (bitAlive && bits[i].lifespan != 0 && bits[i].age() > bits[i].lifespan) {
          killBit(i);
        }
      }
      lastMove = mils;
    }

    for (Bit &bit : bits) {
      leds[bit.px] = ColorFromPalette(palette, bit.colorIndex);
    }
    if (colorCycleDuration != 0 && mils - lastColorChange > colorCycleDuration / 0xFF) {
      for (Bit &bit : bits) {
        bit.colorIndex = addmod8(bit.colorIndex, 1, 0xFF);
      }
      lastColorChange = mils;
    }
    if (fadeUpDistance > 0) {
      for (Bit &bit : bits) {
        // TODO: can fade-up take into account color advancement?
        fadeUpForBit(bit, bit.px, 1, fadeUpDistance-1, lastMove, leds);
      }
    }

    if (spawnRule == maintainPopulation) {
      for (unsigned b = bits.size(); b < numBits; ++b) {
        addBit();
      }
    }
  };

  Bit &addBit() {
    Bit newbit = makeBit();
    bits.push_back(newbit);
    return bits.back();
  }
};

/* ------------------------------------------------------------------------------- */

class DownstreamPattern : public Pattern {
  BitsFiller *bitsFiller;
public:
  DownstreamPattern() {
    BitsFiller::BitDirection circledirection = random8()%2 ? EdgeType::clockwise : EdgeType::counterclockwise;
    vector<BitsFiller::BitDirections> directions = {circledirection, EdgeType::outbound};
    bitsFiller = new BitsFiller(0, 24, 0, directions, (CRGBPalette256)Trans_Flag_gp);
    bitsFiller->flowRule = BitsFiller::split;
    bitsFiller->fadeUpDistance = 3;
    bitsFiller->colorCycleDuration = 0;
    for (int i = 0; i < 3; ++i) {
      BitsFiller::Bit &bit = bitsFiller->addBit();
      bit.px = circleleds[i * circleleds.size() / 3];
      bit.colorIndex = i * 0xFF / 5; // one of each color in 5-color flag
    }
  }
  ~DownstreamPattern() {
    delete bitsFiller;
  }
  void setup() {
  }

  void update(CRGBArray<NUM_LEDS> &leds) {
    bitsFiller->update(leds);    
  }

  const char *description() {
    return "downstream";
  }
};

/* ------------------------------------------------------------------------------- */

class CouplingPattern : public Pattern {
  BitsFiller *bits;
public:
  CouplingPattern() {
    bits = new BitsFiller(16, 50, 4000, {Edge::clockwise | Edge::counterclockwise}, (CRGBPalette256)Trans_Flag_gp);
    bits->spawnPixels = &circleleds;
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

