#ifndef PATTERN_H
#define PATTERN_H

#include <FastLED.h>
#include <vector>
#include <deque>
#include <functional>

#include "util.h"
#include "palettes.h"
#include "ledgraph.h"
#include "drawing.h"

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
    friend BitsFiller;
  private:
    unsigned long birthmilli;
  public:
    int px;
    vector<BitDirection> directions;
    unsigned long lifespan;
    CRGB color;

    CRGB initialColor;  // storage only
    uint8_t colorIndex; // storage only

    Bit(int px, vector<BitDirection> directions, unsigned long lifespan) 
      : px(px), directions(directions), lifespan(lifespan) {
      reset();
      // logf("bit constructor for this = %p, px = %i, directions = %i, lifespan = %i", this, px, directions, lifespan);
    }
    // Bit(const Bit &other) {
    //   reset_to(other);
    // }
    void reset() {
      birthmilli = millis();
      color = CHSV(random8(), 0xFF, 0xFF);
    }
    // commented unless we need a custom copy constructor
    // void reset_to(const Bit &other) {
    //   // logf("RESET BIT TO OTHER %p px = %i, directions = %i, lifespan = %i", &other, other.px, other.directions, other.lifespan);
    //   px = other.px;
    //   directions = other.directions;
    //   lifespan = other.lifespan;
    //   birthmilli = other.birthmilli;
    //   color = other.color;
    //   colorIndex = other.colorIndex;
    //   initialColor = other.initialColor;
    // }
    unsigned long age() {
      return millis() - birthmilli;
    }
  };

private:

  vector<Bit> bits;
  unsigned long lastMove = 0;
  unsigned long lastColorChange = 0;
  unsigned long lastBitSpawn = 0;

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
      // TODO: need to store colorIndex/etc differently if we have additional color rules
      return Bit(spawnLocation(), directionsForBit, lifespan);
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

  bool isIndexAllowed(int index) {
    if (allowedPixels) {
      return allowedPixels->end() != allowedPixels->find(index);
    }
    return true;
  }

  vector<int> nextIndexes(int index, const vector<BitDirection> &bitDirections) {
    vector<int> next;
    switch (flowRule) {
      case priority:
        for (BitDirection direction : bitDirections) {
          auto adj = ledgraph.adjacencies(index, direction);
          for (auto edge : adj) {
            if (isIndexAllowed(edge.to)) {
              next.push_back(edge.to);
              break;
            }
          }
          if (!next.empty()) {
            // FIXME: priority should choose randomly between edges of the same priority
            // but the randomness needs to be stable while pathing this bit during fade-up during repeated calls to nextIndexes
            break;
          }
        }
        break;
      case random:
      case split: {
        vector<Edge> nextEdges;
        for (BitDirection direction : bitDirections) {
          auto adj = ledgraph.adjacencies(index, direction);
          for (auto a : adj) {
            if (isIndexAllowed(a.to)) {
              nextEdges.push_back(a);
            }
          }
        }
        if (flowRule == split) {
          if (nextEdges.size() == 1) {
            // flow normally if we're not actually splitting
            next.push_back(nextEdges.front().to);
          } else {
            // split along all allowed split directions, or none if none are allowed
            for (Edge nextEdge : nextEdges) {
              if (splitDirections.size() == 0 || splitDirections.end() != find(splitDirections.begin(), splitDirections.end(), nextEdge.type)) {
                next.push_back(nextEdge.to);
              }
            }
          }
        } else {
          next.push_back(nextEdges.at(random8()%nextEdges.size()).to);
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
  unsigned maxBitsPerSecond = 0; // limit how fast new bits are spawned, 0 = no limit
  unsigned speed; // in pixels/second
  unsigned long lifespan = 0; // in milliseconds, forever if 0
  vector<BitDirections> bitDirections; // vector of bitfield
  FlowRule flowRule = random;
  LeafRule leafRule = kill;
  SpawnRule spawnRule = maintainPopulation;
  unsigned fadeUpDistance = 0; // fade up n pixels ahead of bit motion
  
  vector<BitDirection> splitDirections; // if flowRule is split, which directions are allowed to split (empty for all directions)
  
  const vector<int> *spawnPixels = NULL; // list of pixels to automatically spawn bits on
  set<int> *allowedPixels = NULL; // set of pixels that bits are allowed to travel to

  function<void(Bit &)> handleNewBit = [](Bit &bit){};
  function<void(Bit &)> handleUpdateBit = [](Bit &bit){};

  BitsFiller(unsigned numBits, unsigned speed, unsigned long lifespan, vector<BitDirections> bitDirections)
    : numBits(numBits), speed(speed), lifespan(lifespan), bitDirections(bitDirections) {
    bits.reserve(numBits);
  };

  void fadeUpForBit(Bit &bit, int px, int distanceAway, int distanceRemaining, unsigned long lastMove, CRGBArray<NUM_LEDS> &leds) {
    vector<int> next = nextIndexes(px, bit.directions);

    unsigned long mils = millis();
    unsigned long fadeUpDuration = 1000 * fadeUpDistance / speed;
    for (int n : next) {
      unsigned long fadeTimeSoFar = mils - lastMove + distanceRemaining * 1000/speed;
      uint8_t progress = 0xFF * fadeTimeSoFar / fadeUpDuration;

      CRGB existing = leds[n];
      CRGB blended = blend(existing, bit.color, dim8_raw(progress));
      leds[n] = blended;
      
      if (distanceRemaining > 0) {
        fadeUpForBit(bit, n, distanceAway+1, distanceRemaining-1, lastMove, leds);
      }
    }
  }

  int fadeDown = 60;
  void update(CRGBArray<NUM_LEDS> &leds) {
    // FIXME: global fadedown
    EVERY_N_MILLIS(16) {
      leds.fadeToBlackBy(fadeDown);
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
      handleUpdateBit(bit);
    }

    for (Bit &bit : bits) {
      leds[bit.px] = bit.color;
    }
    
    if (fadeUpDistance > 0) {
      for (Bit &bit : bits) {
        // don't show full fade-up distance right when bit is created
        int bitFadeUpDistance = min((unsigned long)fadeUpDistance, speed * bit.age() / 1000);
        if (bitFadeUpDistance > 0) {
          // TODO: can fade-up take into account color advancement?
          fadeUpForBit(bit, bit.px, 1, bitFadeUpDistance - 1, lastMove, leds);
        }
      }
    }

    if (spawnRule == maintainPopulation) {
      for (unsigned b = bits.size(); b < numBits; ++b) {
        if (maxBitsPerSecond != 0 && mils - lastBitSpawn < 1000 / maxBitsPerSecond) {
          continue;
        }
        addBit();
        lastBitSpawn = mils;
      }
    }
  };

  Bit &addBit() {
    Bit newbit = makeBit();
    handleNewBit(newbit);
    newbit.initialColor = newbit.color;
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
    bitsFiller = new BitsFiller(0, 24, 0, directions);
    bitsFiller->flowRule = BitsFiller::split;
    bitsFiller->fadeUpDistance = 3;
    
    int bitCount = 3;
    CRGBPalette256 palette = Trans_Flag_gp;
    for (int i = 0; i < bitCount; ++i) {
      BitsFiller::Bit &bit = bitsFiller->addBit();
      bit.px = circleleds[i * circleleds.size() / bitCount];
      bit.color = ColorFromPalette(palette, i * 0xFF / 5);
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
    bits = new BitsFiller(16, 50, 4000, {Edge::clockwise | Edge::counterclockwise});
    bits->spawnPixels = &circleleds;
    bits->handleNewBit = [](BitsFiller::Bit &bit) {
      bit.color = ColorFromPalette((CRGBPalette32)Trans_Flag_gp, random8());
    };
  }
  ~CouplingPattern() {
    delete bits;
  }
  void setup() { }

  void update(CRGBArray<NUM_LEDS> &leds) {
    const vector<int> * const planetspokelists[] = {&venusleds, &marsleds};
    const vector<int> * const earthspokelists[] = {&earthleds, &earthasmarsleds, &earthasvenusleds};

    EVERY_N_MILLIS(1000) {
      leds.fill_solid(CRGB::Black);

      // pick two
      const vector<int> *spokes[2] = {0};
      do {
        if (random8(2) == 0) {
          spokes[0] = ARRAY_SAMPLE(earthspokelists);
        } else {
          spokes[0] = ARRAY_SAMPLE(planetspokelists);
        }
        spokes[1] = ARRAY_SAMPLE(planetspokelists);
      } while (spokes[0] == spokes[1]);

      for (int s = 0; s < 2; ++s) {
        const vector<int> *spoke = spokes[s];
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

/* ------------------------------------------------------------------------------- */

class ChargePattern : public Pattern {
  BitsFiller *bitsFiller;
  set<int> allowedPixels;
public:
  ChargePattern() {
    vector<BitsFiller::BitDirections> directions = {EdgeType::outbound, EdgeType::none};
    bitsFiller = new BitsFiller(50, 80, 0, directions);
    bitsFiller->flowRule = BitsFiller::split;
    bitsFiller->splitDirections = {EdgeType::outbound};
    bitsFiller->fadeUpDistance = 0;
    bitsFiller->fadeDown = 0x7F;
    bitsFiller->maxBitsPerSecond = 20;
  }

  ~ChargePattern() {
    delete bitsFiller;
  }

  void setup() {
    allowedPixels.clear();

    int spoke = random8()%3;
    logf("Charge chose %i", spoke);
    CRGB baseColors[] = {transFlagWhite, transFlagPink, transFlagBlue};

    static int cutoffs[] = {circleIndexOppositeEarth, circleIndexOppositeVenus, circleIndexOppositeMars};
    bitsFiller->handleNewBit = [spoke, baseColors](BitsFiller::Bit &bit) {
      // we pick a spawn point, then figure out which direction is the shortest path to the spoke using cutoffs
      // but add some fuzz to cause some bits fo travel around the point opposite the spoke too.
      int cutoff = cutoffs[spoke];
      int circleindex = mod_wrap(cutoff + random8()%6 - 3, circleleds.size());
      int directionFuzz = random8()%8 - 4;
      
      bit.px = circleleds[circleindex];
      
      if ((unsigned)mod_wrap(circleindex - cutoff + directionFuzz, circleleds.size()) > circleleds.size() / 2) {
        bit.directions[1] = EdgeType::counterclockwise;
      } else {
        bit.directions[1] = EdgeType::clockwise;
      }

      // sprinkle bits of other flag colors in there
      uint8_t colorChoice = random8();
      if (colorChoice < 0x20) {
        bit.color = baseColors[addmod8(spoke, 1, ARRAY_SIZE(baseColors))];
      } else if (colorChoice < 0x40) {
        bit.color = baseColors[addmod8(spoke, 2, ARRAY_SIZE(baseColors))];
      } else {
        bit.color = baseColors[spoke];
      };
    };

    CRGB targetColor = baseColors[spoke];
    bitsFiller->handleUpdateBit = [targetColor](BitsFiller::Bit &bit) {
      uint8_t blendAmount = min((unsigned long)0xFF, 0xFF * bit.age() / 500);
      bit.color = blend(bit.initialColor, targetColor, blendAmount);
    };
    
    const vector<int> *spokes[] = {&earthleds, &venusleds, &marsleds};
    allowedPixels.insert(spokes[spoke]->begin(), spokes[spoke]->end());
    allowedPixels.insert(circleleds.begin(), circleleds.end());
    bitsFiller->allowedPixels = &allowedPixels;
  }

  void update(CRGBArray<NUM_LEDS> &leds) {
    bitsFiller->update(leds);    
  }

  const char *description() {
    return "charge";
  }
};

#endif
