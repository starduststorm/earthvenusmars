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
#include "AudioManager.h"

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
    bool firstFrame = true;
  public:
    int px;
    vector<BitDirection> directions; // FIXME: maybe not the right choice after all because this eats minimum 12 bytes per instance. 
    unsigned long lifespan;
    CRGB color;
    uint8_t brightness = 0xFF;

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
          // FIXME: EdgeType::random behavior also doesn't work right with the way fadeUp is implemented
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
  unsigned maxSpawnBits;
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

  BitsFiller(unsigned maxSpawnBits, unsigned speed, unsigned long lifespan, vector<BitDirections> bitDirections)
    : maxSpawnBits(maxSpawnBits), speed(speed), lifespan(lifespan), bitDirections(bitDirections) {
    bits.reserve(maxSpawnBits);
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
      blended.nscale8(bit.brightness);
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
    
    if (spawnRule == maintainPopulation) {
      for (unsigned b = bits.size(); b < maxSpawnBits; ++b) {
        if (maxBitsPerSecond != 0 && mils - lastBitSpawn < 1000 / maxBitsPerSecond) {
          continue;
        }
        addBit();
        lastBitSpawn = mils;
      }
    }

    if (mils - lastMove > 1000/speed) {
      for (int i = bits.size() - 1; i >= 0; --i) {
        if (bits[i].firstFrame) {
          // don't flow bits on the first frame. this allows pattern code to make their own bits that are displayed before being flowed
          continue;
        }
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
      CRGB color = bit.color;
      color.nscale8(bit.brightness);
      buffer.leds[bit.px] = color;
    }
    
    if (fadeUpDistance > 0) {
      for (Bit &bit : bits) {
        if (bit.firstFrame) continue;
        // don't show full fade-up distance right when bit is created
        int bitFadeUpDistance = min((unsigned long)fadeUpDistance, speed * bit.age() / 1000);
        if (bitFadeUpDistance > 0) {
          // TODO: can fade-up take into account color advancement?
          fadeUpForBit(bit, bit.px, 1, bitFadeUpDistance - 1, lastMove);
        }
      }
    }

    buffer.ctx.blendIntoContext(ctx, BlendMode::blendBrighten);
    lastTick = mils;

    for (Bit &bit : bits) {
      bit.firstFrame = false;
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
  typedef enum {trans, bi, rainbow, modeCount} ColorMode;
  ColorMode colorMode;
  const int kDefaultSpeed = 24;
public:
  DownstreamPattern() {
    BitsFiller::BitDirection circledirection = random8()%2 ? EdgeType::clockwise : EdgeType::counterclockwise;
    vector<BitsFiller::BitDirections> directions = {circledirection, EdgeType::outbound};
    bitsFiller = new BitsFiller(0, kDefaultSpeed, 0, directions);
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
    
    // TODO: play with speed variation. I'm not sure a simple oscillator is right, needs diff acceleration curve

    //bitsFiller->speed = 24 + beatsin16(6, 0, 48);
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
      bitsFiller->handleUpdateBit = [](BitsFiller::Bit &bit) {

      };
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

// FIXME: WIP
class UpstreamPattern : public Pattern {
  BitsFiller bitsFiller;
  // typedef enum {trans, bi, rainbow, modeCount} ColorMode;
  // ColorMode colorMode;
public:
  UpstreamPattern() : bitsFiller(100, 40, 600, {EdgeType::inbound, EdgeType::clockwise | EdgeType::counterclockwise}) {
    bitsFiller.flowRule = BitsFiller::priority;
    bitsFiller.fadeUpDistance = 3;
    bitsFiller.spawnPixels = &leafleds;
    bitsFiller.handleNewBit = [](BitsFiller::Bit &bit) {
      // bit.color = CHSV(millis() / 4, 0xFF, 0xFF);
      bit.color = ARRAY_SAMPLE(transFlagColors);
    };
    bitsFiller.handleUpdateBit = [](BitsFiller::Bit &bit) {
      int raw = min(0xFF, max(0, (int)(0xFF - 0xFF * bit.age() / bit.lifespan)));
      bit.brightness = raw;
    };
  }

  void update(EVMDrawingContext &ctx) {
    bitsFiller.maxBitsPerSecond = beatsin8(2, 16-4, 16+4);
    ctx.leds.fill_solid(CRGB::Black);
    bitsFiller.update(ctx);
  }

  const char *description() {
    return "upstream";
  }
};

/* ------------------------------------------------------------------------------- */

class LitPattern : public Pattern, public PaletteRotation<CRGBPalette256> {
  void update(EVMDrawingContext &ctx) {
    for (int i = 0; i < ctx.leds.size(); ++i) {
      uint8_t hue=0, brightness=0;

      hue = beatsin8(i/20 + 10, 0, 0xFF, 0, 6*i); 
      brightness = beatsin8(13+i/3, 0x0F, 0xFF, 0, 3*i);

      ctx.leds[i] = getPaletteColor(hue, brightness);
    }
  }

  const char *description() {
    return "lit";
  };
};

/* ------------------------------------------------------------------------------- */

class HeartBeatPattern : public Pattern, public FFTProcessing {
  const uint8_t basebpm = 60;
  uint8_t bpm = 40;
  unsigned long lastSystole = 0;
  unsigned long diastoleAt = 0;
  const CRGB bloodColor = CRGB(0xFF, 0, 0x15);
  
  unsigned long lastTick = 0;
  int fadeDown = 3;
  float avgAmp = 0;

  BitsFiller pumpFiller;
public:
  HeartBeatPattern() : pumpFiller(0, 30, 1200, {EdgeType::outbound}) {
    pumpFiller.flowRule = BitsFiller::split;
    pumpFiller.fadeDown = fadeDown;
    pumpFiller.splitDirections = {EdgeType::outbound};
  }

  void beat(EVMDrawingContext &ctx, bool parity, uint8_t intensity) {
    CRGB scaledColor = bloodColor;
    scaledColor.nscale8(intensity);
    
    if (parity) {
      // systole
      // ok I'm cheating here - the systole/diastole timing will actually be swapped and some of the diastole bits will priority-turn outbound to form the systole
      // it just looks better

      // static int systole_spawnpixels[] = {earthleds[0], venusleds[0], marsleds[0]};
      // for (unsigned i = 0; i < ARRAY_SIZE(systole_spawnpixels); ++i) {
      //   BitsFiller::Bit &bit = pumpFiller.addBit();
      //   bit.px = systole_spawnpixels[i];
      //   bit.color = scaledColor;
      // }
    } else {
      // diastole
      for (unsigned i = 0; i < spoke_tip_leds.size(); ++i) {
        BitsFiller::Bit &bit = pumpFiller.addBit();
        bit.px = spoke_tip_leds[i];
        bit.directions = {EdgeType::inbound};
        bit.color = scaledColor;
      }
      
      for (unsigned i = 0; i < 6; ++i) {
        const int circleSixth = circleleds.size() / 6;
        BitsFiller::Bit &bit = pumpFiller.addBit();
        bit.px = circleleds[(i>>1) * circleleds.size() / 3 + circleSixth + i%2];
        bit.directions = {EdgeType::outbound, (i%2 == 0 ? EdgeType::counterclockwise : EdgeType::clockwise)};
        bit.color = scaledColor;
      }
    }
  }

  void update(EVMDrawingContext &ctx) {
    unsigned long mils = millis();

    if (lastTick != 0) {
      ctx.leds.fadeToBlackBy(fadeDown * (mils - lastTick));
    }
    lastTick = mils;

    unsigned milsPerBeat = 1000 * 60 / bpm;
    if (mils - lastSystole > milsPerBeat) {
      beat(ctx, true, 0xFF);
      lastSystole = mils;
      diastoleAt = lastSystole + milsPerBeat * 0.24;
    }
    if (diastoleAt != 0 && mils > diastoleAt) {
      beat(ctx, false, 0x8F);
      diastoleAt = 0;
    }
    pumpFiller.update(ctx);

    // louder -> get yo blood pumpin
    const unsigned ampSamples = 1200;
    const unsigned int ampBaseline = 494;
    unsigned long amplitude = min(1000u, fftUpdate().amplitude);
    amplitude = max(0, (long)amplitude - (long)ampBaseline);
    avgAmp = (avgAmp * (ampSamples-1) + amplitude) / ampSamples;
    bpm = min(150, basebpm + 3 * avgAmp);
  }

  const char *description() {
    return "heartbeat";
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

  typedef enum {flag0, flag1, flag2, pride, trans, bi, lesbian, modeCount} ColorMode;
  ColorMode colorMode = flag0;
public:
  int spoke; 
  ChargePattern() {
    vector<BitsFiller::BitDirections> directions = {EdgeType::outbound, EdgeType::none};
    bitsFiller = new BitsFiller(30, 50, 0, directions);
    bitsFiller->flowRule = BitsFiller::split;
    bitsFiller->splitDirections = {EdgeType::outbound};
    bitsFiller->fadeUpDistance = 2;
    bitsFiller->fadeDown = 5;
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

      switch(colorMode) {
        case flag0:
        case flag1:
        case flag2: {
          CRGB targetColor = baseColors[addmod8(spoke, 0 + colorMode, ARRAY_SIZE(baseColors))];
          // sprinkle bits of other flag colors in there
          uint8_t colorChoice = random8();
          if (colorChoice < 0x20) {
            bit.color = baseColors[addmod8(spoke, 1 + colorMode, ARRAY_SIZE(baseColors))];
            bit.lifespan = 320;
          } else if (colorChoice < 0x40) {
            bit.color = baseColors[addmod8(spoke, 2 + colorMode, ARRAY_SIZE(baseColors))];
            bit.lifespan = 320;
          } else {
            bit.color = baseColors[addmod8(spoke, 0 + colorMode, ARRAY_SIZE(baseColors))];
          }
          
          bitsFiller->handleUpdateBit = [targetColor](BitsFiller::Bit &bit) {
            if (bit.lifespan != 0) {
              uint8_t blendAmount = min((unsigned long)0xFF, 0xFF * bit.age() / 500);
              bit.color = blend(bit.initialColor, targetColor, blendAmount);
            }
          };
          break;
        }
        case trans:
          bit.color = ColorFromPalette((CRGBPalette256)Trans_Flag_gp, random8()); break;
        case bi:
          bit.color = ColorFromPalette((CRGBPalette256)Bi_Flag_gp, random8()); break;
        case lesbian:
          bit.color = ColorFromPalette((CRGBPalette256)Lesbian_Flag_gp, millis() / 5); break;          
        case pride:
          bit.color = ColorFromPalette((CRGBPalette256)Pride_Flag_gp, millis() / 8); break;
        default: break;
      }
    };
  }

  void poke() {
    colorMode = (ColorMode)addmod8(colorMode, 1, modeCount);
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

/* ------------------------------------------------------------------------------- */

class SoundBits : public Pattern, public FFTProcessing, public PaletteRotation<CRGBPalette32> {
  BitsFiller bitsFillerOut;
  BitsFiller bitsFillerIn;

    typedef enum {hue, trans, bi, lesbian, rotation, modeCount} ColorMode;
  ColorMode colorMode = hue;
public:
  SoundBits() : bitsFillerOut(0, 60, 1200, {EdgeType::outbound, EdgeType::clockwise | EdgeType::counterclockwise}),
                bitsFillerIn(0, 60, 1200, {EdgeType::inbound, EdgeType::clockwise | EdgeType::counterclockwise}) {
    bitsFillerOut.flowRule = BitsFiller::random;
    bitsFillerOut.fadeUpDistance = 3;
    bitsFillerOut.spawnPixels = &circleleds;
    bitsFillerOut.fadeDown = 6;
    bitsFillerOut.handleUpdateBit = [](BitsFiller::Bit &bit) {
      int raw = min(0xFF, max(0, (int)(0xFF - 0xFF * bit.age() / bit.lifespan)));
      bit.brightness = raw;
    };

    bitsFillerIn.flowRule = BitsFiller::random;
    bitsFillerIn.fadeUpDistance = 3;
    bitsFillerIn.spawnPixels = &leafleds;
    bitsFillerOut.fadeDown = 6;
    bitsFillerIn.handleUpdateBit = [](BitsFiller::Bit &bit) {
      int raw = min(0xFF, max(0, (int)(0xFF - 0xFF * bit.age() / bit.lifespan)));
      bit.brightness = raw;
    };
  }

  void update(EVMDrawingContext &ctx) {
    vector<int> spectrum = fftUpdate().spectrum;
    // fftLog(spectrum);

    for (unsigned b = 0; b < spectrum.size(); ++b) {
      int thresh = 3;
      if (spectrum[b] > thresh) {
        unsigned maxbits = 50;
        if (bitsFillerOut.bits.size() + bitsFillerIn.bits.size() < maxbits) {
          // loglf("levels[%i]: %i; making a bit; out bits = %u, in bits = %u...", b, spectrum[b], bitsFillerOut.bits.size(), bitsFillerIn.bits.size());
          bool spawnoutbound = b < spectrum.size() / 5;
          unsigned maxlifespan = spawnoutbound ? 2000 : 1000;
          BitsFiller::Bit &bit = (spawnoutbound ? bitsFillerOut : bitsFillerIn).addBit();
          bit.lifespan = min(maxlifespan, maxlifespan * (spectrum[b]-thresh)/20);
          // logf("done");

          uint8_t colorIndex = millis() / 100 + 0xFF * b / 13;
          CRGB color;
          switch (colorMode) {
            case hue:
              color = CHSV(colorIndex, 0xFF, 0xFF); break;
            default:
              color = getPaletteColor(colorIndex);
              break;
          }
          
          color.nscale8(min(0xFF, 0xFF * (spectrum[b]-thresh)/10));
          bit.color = color;
        }
      }
    }

    ctx.leds.fill_solid(CRGB::Black);

    bitsFillerOut.update(ctx);
    bitsFillerIn.update(ctx);
  }

  void poke() {
    colorMode = (ColorMode)addmod8(colorMode, 1, modeCount);
    logf("  colormode -> %i", colorMode);
    pauseRotation = (colorMode != rotation);

    switch (colorMode) {
      case trans:
        setPalette(Trans_Flag_gp); break;
      case bi:
        setPalette(Bi_Flag_gp); break;
      case lesbian:
        setPalette(Lesbian_Flag_gp); break;
      case rotation:
        randomizePalette(); break;
      default: break;
    }
  }

  const char *description() {
    return "SoundBits";
  }
};

class SoundTest : public Pattern, public FFTProcessing {
  BitsFiller bitsFiller;
public:
  SoundTest() : bitsFiller(0, 60, 1200, {EdgeType::outbound, EdgeType::clockwise | EdgeType::counterclockwise}) {
    bitsFiller.flowRule = BitsFiller::random;
    bitsFiller.fadeUpDistance = 3;
    bitsFiller.spawnPixels = &circleleds;
    bitsFiller.handleUpdateBit = [](BitsFiller::Bit &bit) {
      int raw = min(0xFF, max(0, (int)(0xFF - 0xFF * bit.age() / bit.lifespan)));
      bit.brightness = raw;
    };
  }

  void update(EVMDrawingContext &ctx) {
    vector<int> spectrum = fftUpdate().spectrum;
    fftLog(spectrum);
    
    bitsFiller.update(ctx);
    ctx.leds.fadeToBlackBy(15);
    for (unsigned b = 0; b < spectrum.size(); ++b) {
      int thresh = 5;
      if (spectrum[b] > thresh) {
        for (unsigned i = 0; i < circleleds.size(); ++i) {
          ctx.leds[circleleds[i]] += CHSV(0xFF*(b-2)/8, 0xFF, min(0xFF, 0xFF * (spectrum[b]-thresh) / 10));
        }
      }
    }
  }

  const char *description() {
    return "soundtest";
  }
};

#endif
