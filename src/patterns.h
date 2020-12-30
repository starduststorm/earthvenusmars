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
  typedef enum { end, bounce } LeafRule;
  typedef enum { maintainPopulation, manualSpawn } SpawnRule;
private:
  
  struct Bit {
    int px;
    BitDirections directions;
    unsigned long birthmilli;
    unsigned long lifespan;
    CRGB color;
    Bit(int px, BitDirections directions, unsigned long lifespan, CRGB color) : px(px), directions(directions), lifespan(lifespan), color(color) {
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
      color = other.color;
      birthmilli = other.birthmilli;
    }
    unsigned long age() {
      return millis() - birthmilli;
    }
  };

  vector<Bit> bits;
  deque<Bit> deadBits;
  unsigned long lastMove = 0; // FIXME: implement fadeup?

  int spawnLocation() {
    if (spawnPixels) {
      return spawnPixels->at(random8()%spawnPixels->size());
    }
    return random16()%NUM_LEDS;
  }

  Bit makeBit(Bit *fromBit=NULL) {
    BitDirections bitDirections = directions;
    if (bitDirections & EdgeType::clockwise && bitDirections & EdgeType::counterclockwise) {
      if (random8()%2) {
        bitDirections -= EdgeType::clockwise;
      } else {
        bitDirections -= EdgeType::counterclockwise;
      }
    }

    if (bitDirections & EdgeType::inbound && bitDirections & EdgeType::outbound) {
      if (random8()%2) {
        bitDirections -= EdgeType::inbound;
      } else {
        bitDirections -= EdgeType::outbound;
      }
    }

    if (deadBits.empty()) {
      if (fromBit) {
        return Bit(*fromBit);
      } else {
        return Bit(spawnLocation(), bitDirections, lifespan, ColorFromPalette(palette, random8()));
      }
    } else {
      assert(0, "fixme: dead bits");
      Bit bit = deadBits.front();
      deadBits.pop_front();
      if (fromBit) {
        bit.reset_to(*fromBit);
      } else {
        bit.px = spawnLocation();
        bit.directions = bitDirections;
        bit.lifespan = lifespan;
        bit.birthmilli = millis();
        bit.color = ColorFromPalette(palette, random8());
      }
      return bit;
    }
  }

  void killBit(int bitIndex) {
    // deadBits.push_back(*it);
    bits.erase(bits.begin() + bitIndex);
  }

  void splitBit(Bit &bit, Edge edge) {
    Bit split = makeBit(&bit);
    split.px = edge.to;
    bits.push_back(split);
  }

  bool flowBit(int bitIndex) {
    Bit &bit = bits[bitIndex];
    vector<Edge> adj = ledgraph.adjacencies(bit.px, bit.directions);
    if (adj.size() == 0) {
      // leaf behavior
      switch (leafRule) {
        case end:
          killBit(bitIndex);
          return false;
          break;
        case bounce: 
          // FIXME: will I ever use this
          logf("bounce unimplemented");
          delay(100);
          break;
      }
    } else if (adj.size() == 1) {
      Edge follow = adj.front();
      bit.px = follow.to;
    } else {
      // flow behavior
      switch (flowRule) {
        case random: 
          bit.px = adj[random8()%adj.size()].to;
          break;
        case priority:
          // for (BitDirection direction : it->directions) {
          //   for (Edge edge : adj) {
          //     if (edge.type == direction) {
          //       it->px = edge.to;
          //       return;
          //     }
          //   }
          // }
          // FIXME: implement
          break;
        case split: {
          bool first = true;
          int i = 0;
          for (Edge edge : adj) {
            if (first) {
              bit.px = edge.to;
              first = false;
            } else {
              splitBit(bit, edge);
            }
          }
          break;
        }
      }
    }
    return true;
  }

public:
  unsigned numBits;
  unsigned speed; // in pixels/second
  unsigned long lifespan = 0; // in milliseconds, forever if 0
  BitDirections directions; // may contain contradictory directions e.g. inbound and outbound and will choose randomly when spawning a new bit
  FlowRule flowRule = random;
  LeafRule leafRule = end;
  SpawnRule spawnRule = maintainPopulation;
  
  CRGBPalette256 palette;
  
  vector<int> *spawnPixels;

  BitsFiller(unsigned numBits, unsigned speed, unsigned long lifespan, BitDirections directions, CRGBPalette256 palette)
    : numBits(numBits), speed(speed), lifespan(lifespan), directions(directions), palette(palette) {
    bits.reserve(numBits);
  };

  void update(CRGBArray<NUM_LEDS> &leds) {
    // FIXME: global fadedown
    EVERY_N_MILLIS(16) {
      leds.fadeToBlackBy(40);
    }

    unsigned long mils = millis();
    if (mils - lastMove > 1000/speed) {
      for (int i = bits.size() - 1; i >= 0; --i) {
        Bit bit = bits[i];
        bool bitAlive = flowBit(i);
        if (bitAlive && bit.lifespan != 0 && bit.age() > bit.lifespan) {
          killBit(i);
        }
      }
      lastMove = mils;
    }
    for (Bit &bit : bits) {
      leds[bit.px] = bit.color;
    }

    if (spawnRule == maintainPopulation) {
      for (unsigned b = bits.size(); b < numBits; ++b) {
        addBit();
      }
    }
  };

  void addBit() {
    Bit newbit = makeBit();
    bits.push_back(newbit);
  }
};


class DownstreamPattern : public Pattern {
  BitsFiller *bitsFiller;
public:
  DownstreamPattern() {
    EdgeTypes direction = random8()%2 ? EdgeType::clockwise : EdgeType::counterclockwise;
    direction |= EdgeType::outbound;
    bitsFiller = new BitsFiller(2, 24, 0, direction, (CRGBPalette256)Trans_Flag_gp);
    bitsFiller->flowRule = BitsFiller::split;
    bitsFiller->spawnPixels = &circleleds;
    
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

class CouplingPattern : public Pattern {
  BitsFiller *bits;
public:
  CouplingPattern() {
    bits = new BitsFiller(16, 50, 4000, Edge::clockwise | Edge::counterclockwise, (CRGBPalette256)Trans_Flag_gp);
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

