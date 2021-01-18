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

  void loop(EVMDrawingContext &ctx) {
    update(ctx);
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

  virtual void update(EVMDrawingContext &ctx) { }
  
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

  virtual void poke() { } // handle user-input
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

  EVMPixelBuffer buffer;

  unsigned long lastTick = 0;
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
        } else if (nextEdges.size() > 0) {
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

  vector<Bit> bits;
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
    buffer.leds.fill_solid(CRGB::Black);
  };

  void fadeUpForBit(Bit &bit, int px, int distanceAway, int distanceRemaining, unsigned long lastMove) {
    vector<int> next = nextIndexes(px, bit.directions);

    unsigned long mils = millis();
    unsigned long fadeUpDuration = 1000 * fadeUpDistance / speed;
    for (int n : next) {
      unsigned long fadeTimeSoFar = mils - lastMove + distanceRemaining * 1000/speed;
      uint8_t progress = 0xFF * fadeTimeSoFar / fadeUpDuration;

      CRGB existing = buffer.leds[n];
      CRGB blended = blend(existing, bit.color, dim8_raw(progress));
      buffer.leds[n] = blended;
      
      if (distanceRemaining > 0) {
        fadeUpForBit(bit, n, distanceAway+1, distanceRemaining-1, lastMove);
      }
    }
  }

  int fadeDown = 4; // fadeToBlackBy units per millisecond
  void update(EVMDrawingContext &ctx) {
    unsigned long mils = millis();

    buffer.leds.fadeToBlackBy(fadeDown * (mils - lastTick));
    
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
      buffer.leds[bit.px] = bit.color;
    }
    
    if (fadeUpDistance > 0) {
      for (Bit &bit : bits) {
        // don't show full fade-up distance right when bit is created
        int bitFadeUpDistance = min((unsigned long)fadeUpDistance, speed * bit.age() / 1000);
        if (bitFadeUpDistance > 0) {
          // TODO: can fade-up take into account color advancement?
          fadeUpForBit(bit, bit.px, 1, bitFadeUpDistance - 1, lastMove);
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
    buffer.ctx.blendIntoContext(ctx, BlendMode::blendBrighten);
    lastTick = mils;
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
  typedef enum {trans, bi, rainbow, modeCount} ColorMode;
  ColorMode colorMode;
public:
  DownstreamPattern() {
    BitsFiller::BitDirection circledirection = random8()%2 ? EdgeType::clockwise : EdgeType::counterclockwise;
    vector<BitsFiller::BitDirections> directions = {circledirection, EdgeType::outbound};
    bitsFiller = new BitsFiller(0, 24, 0, directions);
    bitsFiller->flowRule = BitsFiller::split;
    bitsFiller->fadeUpDistance = 3;
    
    int bitCount = 3;
    
    for (int i = 0; i < bitCount; ++i) {
      BitsFiller::Bit &bit = bitsFiller->addBit();
      bit.px = circleleds[i * circleleds.size() / bitCount];
      bit.colorIndex = i * 0xFF / 8;
    }
    colorMode = (ColorMode)(random8()%modeCount);
    updateColors();
  }
  ~DownstreamPattern() {
    delete bitsFiller;
  }
  
  void update(EVMDrawingContext &ctx) {
    ctx.leds.fill_solid(CRGB::Black);
    bitsFiller->update(ctx);
  }

  void updateColors() {
    for (unsigned b = 0; b < bitsFiller->bits.size(); ++b) {
      if (colorMode == trans) {
        bitsFiller->bits[b].color = ColorFromPalette((CRGBPalette256)Trans_Flag_gp, b * 0xFF / 5);
      } else if (colorMode == bi) {
        bitsFiller->bits[b].color = ColorFromPalette((CRGBPalette256)Bi_Flag_gp, b * 0xFF / 2);
      }
    }
    if (colorMode == rainbow) {
      bitsFiller->handleUpdateBit = [](BitsFiller::Bit &bit) {
        bit.color = CHSV(millis()/20 + bit.colorIndex, 0xFF, 0xFF);
      };
    } else {
      bitsFiller->handleUpdateBit = [](BitsFiller::Bit &bit) {};
    }
  }

  void poke() {
    colorMode = (ColorMode)addmod8(colorMode, 1, modeCount);
    updateColors();
  }

  const char *description() {
    return "downstream";
  }
};

/* ------------------------------------------------------------------------------- */

class CouplingPattern : public Pattern {
  enum { coupling, looking } state = looking;
  BitsFiller *spokesFillers[2];
  set<int> allowedPixels[2];
  unsigned long lastStateChange = 0;
public:
  CouplingPattern() {
    for (int i = 0; i < 2; ++i) {
      spokesFillers[i] = new BitsFiller(8, 50, 3000, {Edge::outbound, Edge::clockwise | Edge::counterclockwise});
      spokesFillers[i]->spawnPixels = &circleleds;
      spokesFillers[i]->allowedPixels = &allowedPixels[i];
      spokesFillers[i]->spawnRule = BitsFiller::maintainPopulation;
      spokesFillers[i]->maxBitsPerSecond = 10;
      spokesFillers[i]->fadeDown = 3;
      spokesFillers[i]->flowRule = BitsFiller::split;
      spokesFillers[i]->splitDirections = {EdgeType::outbound};
    }
  }
  ~CouplingPattern() {
    delete spokesFillers[0];
    delete spokesFillers[1];
  }

  CRGBPalette256 palette = Trans_Flag_gp;
  uint8_t paletteIndex = 0;
  void poke() {
    const int numPalettes = 3;
    paletteIndex = addmod8(paletteIndex, 1, numPalettes);
    switch (paletteIndex) {
      case 0:
        palette = Trans_Flag_gp; break;
      case 1:
        palette = Bi_Flag_gp; break;
      case 2:
        palette = Lesbian_Flag_gp; break;
    }
  }

  void update(EVMDrawingContext &ctx) {
    ctx.leds.fill_solid(CRGB::Black);

    unsigned long mils = millis();
    if (state == looking && mils - lastStateChange > 500) {
      const vector<int> * const planetspokelists[] = {&venusleds, &marsleds};
      const vector<int> * const earthspokelists[] = {&earthleds, &earthasmarsleds, &earthasvenusleds};
      const vector<int> *spokes[2] = {0};
      // omg matchmaking time
      do {
        if (random8(2) == 0) {
          spokes[0] = ARRAY_SAMPLE(earthspokelists);
        } else {
          spokes[0] = ARRAY_SAMPLE(planetspokelists);
        }
        spokes[1] = ARRAY_SAMPLE(planetspokelists);
      } while (spokes[0] == spokes[1]);

      for (int i = 0; i < 2; ++i) {
        allowedPixels[i].clear();
        allowedPixels[i].insert(spokes[i]->begin(), spokes[i]->end());
        allowedPixels[i].insert(circleleds.begin(), circleleds.end());

        // pick a color/palette for each
        if (random8(2) == 0) {
          spokesFillers[i]->handleNewBit = [this](BitsFiller::Bit &bit) {
            bit.color = ColorFromPalette(palette, random8());
          };
        } else {
          CRGB solidColor = ColorFromPalette(palette, random8());
          spokesFillers[i]->handleNewBit = [solidColor](BitsFiller::Bit &bit) {
            bit.color = solidColor;
          };
        }
      }
      // start splitting bits down the chosen spokes
      spokesFillers[0]->splitDirections = {EdgeType::outbound};
      spokesFillers[1]->splitDirections = {EdgeType::outbound};
      state = coupling;
      lastStateChange = mils;

    } else if (state == coupling && mils - lastStateChange > 1200) {
      // breakup fml, stop following the spokes
      spokesFillers[0]->splitDirections = {};
      spokesFillers[1]->splitDirections = {};
      state = looking;
      lastStateChange = mils;
    }
    spokesFillers[0]->update(ctx);
    spokesFillers[1]->update(ctx);
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
  int spoke; 
  ChargePattern() {
    vector<BitsFiller::BitDirections> directions = {EdgeType::outbound, EdgeType::none};
    bitsFiller = new BitsFiller(50, 70, 0, directions);
    bitsFiller->flowRule = BitsFiller::split;
    bitsFiller->splitDirections = {EdgeType::outbound};
    bitsFiller->fadeUpDistance = 0;
    bitsFiller->fadeDown = 6;
    bitsFiller->maxBitsPerSecond = 25;
    
    spoke = random8()%3;
  }

  ~ChargePattern() {
    delete bitsFiller;
  }

  void setup() {
    allowedPixels.clear();

    const vector<int> *spokes[] = {&earthleds, &venusleds, &marsleds};
    allowedPixels.insert(spokes[spoke]->begin(), spokes[spoke]->end());
    allowedPixels.insert(circleleds.begin(), circleleds.end());
    bitsFiller->allowedPixels = &allowedPixels;

    updateMode();
  }

  void updateMode() {
    CRGB baseColors[] = {transFlagWhite, transFlagPink, transFlagBlue};

    bitsFiller->handleNewBit = [=](BitsFiller::Bit &bit) {
      static int cutoffs[] = {circleIndexOppositeEarth, circleIndexOppositeVenus, circleIndexOppositeMars};
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
  }

  void poke() {
#warning unimplemented
    updateMode();
  }

  void update(EVMDrawingContext &ctx) {
    ctx.leds.fill_solid(CRGB::Black);
    bitsFiller->update(ctx);    
  }

  const char *description() {
    return "charge";
  }
};

/* ------------------------------------------------------------------------------- */

class IntersexFlagPattern : public Pattern {
  BitsFiller outerBits;
  BitsFiller innerBits;
  std::set<int> spokePixels;
public:
  IntersexFlagPattern() : outerBits(20, 40, 4000, {EdgeType::inbound}), innerBits(8, 40, 4000, {EdgeType::clockwise | EdgeType::counterclockwise}) {
    spokePixels.insert(earthleds.begin(), earthleds.end());
    spokePixels.insert(venusleds.begin(), venusleds.end());
    spokePixels.insert(marsleds.begin(), marsleds.end());

    outerBits.allowedPixels = &spokePixels; // keeps pixels from following the last inbound edge onto the circle
    outerBits.spawnPixels = &leafleds;
    outerBits.fadeUpDistance = 2;
    outerBits.maxBitsPerSecond = 30;
    outerBits.handleNewBit = [](BitsFiller::Bit &bit) {
      bit.color = CRGB::Yellow;
    };

    innerBits.spawnPixels = &circleleds;
    innerBits.fadeUpDistance = 2;
    innerBits.maxBitsPerSecond = 8;
    innerBits.handleNewBit = [](BitsFiller::Bit &bit) {
      bit.color = CRGB(0x6E, 0x07, 0xD7);
    };
  }

  void update(EVMDrawingContext &ctx) {
    ctx.leds.fill_solid(CRGB::Black);
    outerBits.update(ctx);
    innerBits.update(ctx);
  }

  const char *description() {
    return "intersex";
  }
};

#endif
