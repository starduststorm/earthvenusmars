#ifndef PATTERN_H
#define PATTERN_H

#include <FastLED.h>
#include <vector>
#include <functional>

#include "util.h"
#include "palettes.h"
#include "ledgraph.h"
#include "drawing.h"
#include "AudioManager.h"

typedef FlagColorManager<CRGBPalette32> EVMColorManager;

class Pattern {
private:  
  long startTime = -1;
  long stopTime = -1;
  long lastUpdateTime = -1;
public:
  EVMColorManager *colorManager;
  EVMDrawingContext ctx;
  virtual ~Pattern() { }

  void start() {
    logf("Starting %s", description());
    startTime = millis();
    stopTime = -1;
    setup();
  }

  void loop() {
    update();
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

  virtual void update() { }
  
  virtual const char *description() = 0;

public:
  bool isRunning() {
    return startTime != -1;
  }

  unsigned long runTime() {
    return startTime == -1 ? 0 : millis() - startTime;
  }

  unsigned long frameTime() {
    return (lastUpdateTime == -1 ? 0 : millis() - lastUpdateTime);
  }

  virtual void colorModeChanged() { }
};

/* ------------------------------------------------------------------------------------------------------ */

// a lil patternlet that can be instantiated to run bits
class BitsFiller {
public:
  typedef enum : uint8_t { random, priority, split } FlowRule;
  typedef enum : uint8_t { maintainPopulation, manualSpawn } SpawnRule;
 
  struct Bit {
    friend BitsFiller;
  private:
    unsigned long birthmilli;
    bool firstFrame = true;
  public:
    uint8_t colorIndex; // storage only
    
    uint8_t px;
    EdgeTypesPair directions;
    
    unsigned long lifespan;

    CRGB color;
    uint8_t brightness = 0xFF;

    Bit(int px, EdgeTypesPair directions, unsigned long lifespan) 
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
    // }
    unsigned long age() {
      return millis() - birthmilli;
    }
  };

private:

  EVMDrawingContext &ctx;

  unsigned long lastTick = 0;
  unsigned long lastMove = 0;
  unsigned long lastBitSpawn = 0;

  int spawnLocation() {
    if (spawnPixels) {
      return spawnPixels->at(random8()%spawnPixels->size());
    }
    return random16()%NUM_LEDS;
  }

  Bit makeBit(Bit *fromBit=NULL) {
    // the bit directions at the BitsFiller level may contain multiple options, choose one at random for this bit
    EdgeTypesPair directionsForBit = {0};

    for (int n = 0; n < 2; ++n) {
      uint8_t bit[EdgeTypesCount] = {0};
      uint8_t bitcount = 0;
      for (int i = 0; i < EdgeTypesCount; ++i) {
        uint8_t nybble = bitDirections.pair >> (n * EdgeTypesCount);
        if (nybble & 1 << i) {
          bit[bitcount++] = i;
        }
      }
      if (bitcount) {
        directionsForBit.pair |= 1 << (bit[random8()%bitcount] + n * EdgeTypesCount);
      }
    }

    if (fromBit) {
      return Bit(*fromBit);
    } else {
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

  vector<int> nextIndexes(int index, EdgeTypesPair bitDirections) {
    vector<int> next;
    switch (flowRule) {
      case priority: {
        auto adj = ledgraph.adjacencies(index, bitDirections);
        for (auto edge : adj) {
          if (isIndexAllowed(edge.to)) {
            next.push_back(edge.to);
            break;
          }
        }
        break;
      }
      case random:
      case split: {
        vector<Edge> nextEdges;
        auto adj = ledgraph.adjacencies(index, bitDirections);
        for (auto a : adj) {
          if (isIndexAllowed(a.to)) {
            nextEdges.push_back(a);
          }
        }
        if (flowRule == split) {
          if (nextEdges.size() == 1) {
            // flow normally if we're not actually splitting
            next.push_back(nextEdges.front().to);
          } else {
            // split along all allowed split directions, or none if none are allowed
            for (Edge nextEdge : nextEdges) {
              if (splitDirections & nextEdge.type) {
                next.push_back(nextEdge.to);
              }
            }
          }
        } else if (nextEdges.size() > 0) {
          // FIXME: EdgeType::random behavior doesn't work right with the way fadeUp is implemented
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
      killBit(bitIndex);
      return false;
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
      Serial.print("  Directions: 0b");
      for (int i = 2*EdgeTypesCount - 1; i >= 0; --i) {
        Serial.print(bit.directions.pair & (1 << i));
      }
      Serial.println();
    }
    logf("--------");
  }

  vector<Bit> bits;
  uint8_t maxSpawnBits;
  uint8_t maxBitsPerSecond = 0; // limit how fast new bits are spawned, 0 = no limit
  uint8_t speed; // in pixels/second
  EdgeTypesPair bitDirections;

  unsigned long lifespan = 0; // in milliseconds, forever if 0

  FlowRule flowRule = random;
  SpawnRule spawnRule = maintainPopulation;
  uint8_t fadeUpDistance = 0; // fade up n pixels ahead of bit motion
  EdgeTypes splitDirections = EdgeType::all; // if flowRule is split, which directions are allowed to split
  
  const vector<int> *spawnPixels = NULL; // list of pixels to automatically spawn bits on
  const set<int> *allowedPixels = NULL; // set of pixels that bits are allowed to travel to

  function<void(Bit &)> handleNewBit = [](Bit &bit){};
  function<void(Bit &)> handleUpdateBit = [](Bit &bit){};

  BitsFiller(EVMDrawingContext &ctx, uint8_t maxSpawnBits, uint8_t speed, unsigned long lifespan, vector<EdgeTypes> bitDirections)
    : ctx(ctx), maxSpawnBits(maxSpawnBits), speed(speed), lifespan(lifespan) {
      this->bitDirections = MakeEdgeTypesPair(bitDirections);
    bits.reserve(maxSpawnBits);
  };

  void fadeUpForBit(Bit &bit, int px, int distanceAway, int distanceRemaining, unsigned long lastMove) {
    vector<int> next = nextIndexes(px, bit.directions);

    unsigned long mils = millis();
    unsigned long fadeUpDuration = 1000 * fadeUpDistance / speed;
    for (int n : next) {
      unsigned long fadeTimeSoFar = mils - lastMove + distanceRemaining * 1000/speed;
      uint8_t progress = 0xFF * fadeTimeSoFar / fadeUpDuration;

      CRGB existing = ctx.leds[n];
      CRGB blended = blend(existing, bit.color, dim8_raw(progress));
      blended.nscale8(bit.brightness);
      ctx.leds[n] = blended;
      
      if (distanceRemaining > 0) {
        fadeUpForBit(bit, n, distanceAway+1, distanceRemaining-1, lastMove);
      }
    }
  }

  int fadeDown = 4; // fadeToBlackBy units per millisecond
  void update() {
    unsigned long mils = millis();

    ctx.leds.fadeToBlackBy(fadeDown * (mils - lastTick));
    
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
      if (mils - lastMove > 2000/speed) {
        lastMove = mils;
      } else {
        // This helps avoid time drift, which for some reason can make one device run consistently faster than another
        lastMove += 1000/speed;
      }
    }
    for (Bit &bit : bits) {
      handleUpdateBit(bit);
    }

    for (Bit &bit : bits) {
      CRGB color = bit.color;
      color.nscale8(bit.brightness);
      ctx.leds[bit.px] = color;
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

    lastTick = mils;

    for (Bit &bit : bits) {
      bit.firstFrame = false;
    }
  };

  Bit &addBit() {
    Bit newbit = makeBit();
    handleNewBit(newbit);
    bits.push_back(newbit);
    return bits.back();
  }

  void removeAllBits() {
    bits.clear();
  }

  void resetBitColors(EVMColorManager *colorManager) {
    for (Bit &bit : bits) {
      CRGB origColor = colorManager->getPaletteColor(bit.colorIndex);
      bit.color = origColor.scale8(min(0xFF, 0xFF * bit.color.getAverageLight() / origColor.getAverageLight()));
    }
  }
};

/* ------------------------------------------------------------------------------- */

class DownstreamPattern : public Pattern {
protected:
  BitsFiller *bitsFiller;
  unsigned circleBits = 0;
public:
  DownstreamPattern() {
    EdgeType circledirection = (random8()%2 ? EdgeType::clockwise : EdgeType::counterclockwise);
    vector<EdgeTypes> directions = {circledirection, EdgeType::outbound};
    bitsFiller = new BitsFiller(ctx, 0, 24, 0, directions);
    bitsFiller->flowRule = BitsFiller::split;
  }
  ~DownstreamPattern() {
    colorManager->releaseTrackedColors();
    delete bitsFiller;
  }

  void update() {
    bitsFiller->update();

    // shiftTrackedColors
    if (!colorManager->pauseRotation) {
      for (int i = 0; i < colorManager->trackedColorsCount(); ++i) {
        bitsFiller->bits[i].color = colorManager->getTrackedColor(i);
      }
      colorManager->paletteRotationTick();
    }
  }

  void colorModeChanged() {
    unsigned oldCircleBits = circleBits;
    if (colorManager->pauseRotation) {
      circleBits = colorManager->getNumFlagBands();
      colorManager->releaseTrackedColors();
    } else {
      // keep it simple with 3 bits with doing full palette rotation
      colorManager->prepareTrackedColors(3);
      circleBits = 3;
    }

    if (circleBits != oldCircleBits) {
      bitsFiller->removeAllBits();
      for (unsigned i = 0; i < circleBits; ++i) {
        BitsFiller::Bit &bit = bitsFiller->addBit();
        bit.px = circleleds[i * circleleds.size() / circleBits];
      }
    }
    if (colorManager->pauseRotation) {
      for (unsigned i = 0; i < circleBits; ++i) {
        bitsFiller->bits[i].color = colorManager->getFlagBand(i);
      }
    }

    bitsFiller->fadeDown = circleBits+1;
    bitsFiller->fadeUpDistance = max(2, 6-(int)circleBits);
  }

  const char *description() {
    return "downstream";
  }
};

class DownstreamFilledPattern : public DownstreamPattern {
public:
  DownstreamFilledPattern() {
    bitsFiller->fadeDown = 0;
  }
  void colorModeChanged() {
    DownstreamPattern::colorModeChanged();
    bitsFiller->fadeDown = 0;
  }
  const char *description() {
    return "downstream-filled";
  }
};

/* ------------------------------------------------------------------------------- */

// FIXME: WIP
class UpstreamPattern : public Pattern {
  BitsFiller bitsFiller;
  // typedef enum {trans, bi, rainbow, modeCount} ColorMode;
  // ColorMode colorMode;
public:
  UpstreamPattern() : bitsFiller(ctx, 100, 40, 600, {EdgeType::inbound, EdgeType::clockwise | EdgeType::counterclockwise}) {
    bitsFiller.flowRule = BitsFiller::priority;
    bitsFiller.fadeUpDistance = 3;
    bitsFiller.spawnPixels = &leafleds;
    bitsFiller.handleNewBit = [](BitsFiller::Bit &bit) {
      // bit.color = CHSV(millis() / 4, 0xFF, 0xFF);
      CRGBPalette32 palette = Trans_Flag_gp;
      bit.color = ColorFromPalette(palette, random8());
    };
    bitsFiller.handleUpdateBit = [](BitsFiller::Bit &bit) {
      int raw = min(0xFF, max(0, (int)(0xFF - 0xFF * bit.age() / bit.lifespan)));
      bit.brightness = raw;
    };
  }

  void update() {
    bitsFiller.maxBitsPerSecond = beatsin8(2, 16-4, 16+4);
    bitsFiller.update();
  }

  const char *description() {
    return "upstream";
  }
};

/* ------------------------------------------------------------------------------- */

class LitPattern : public Pattern, public PaletteRotation<CRGBPalette256> {
  void update() {
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
  
  int fadeDown = 3;
  float avgAmp = 0;

  BitsFiller pumpFiller;
public:
  HeartBeatPattern() : pumpFiller(ctx, 0, 30, 1200, {EdgeType::outbound}) {
    pumpFiller.flowRule = BitsFiller::split;
    pumpFiller.fadeDown = fadeDown;
    pumpFiller.splitDirections = EdgeType::outbound;
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
        bit.directions.edgeTypes.first = EdgeType::inbound;
        bit.color = scaledColor;
      }
      
      for (unsigned i = 0; i < 6; ++i) {
        const int circleSixth = circleleds.size() / 6;
        BitsFiller::Bit &bit = pumpFiller.addBit();
        bit.px = circleleds[(i>>1) * circleleds.size() / 3 + circleSixth + i%2];
        bit.directions.edgeTypes.first = EdgeType::outbound;
        bit.directions.edgeTypes.second = (i%2 == 0 ? EdgeType::counterclockwise : EdgeType::clockwise);
        bit.color = scaledColor;
      }
    }
  }

  void update() {
    unsigned long mils = millis();

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
    pumpFiller.update();

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
      spokesFillers[i] = new BitsFiller(ctx, 8, 50, 3000, {Edge::outbound, Edge::clockwise | Edge::counterclockwise});
      spokesFillers[i]->spawnPixels = &circleleds;
      spokesFillers[i]->allowedPixels = &allowedPixels[i];
      spokesFillers[i]->spawnRule = BitsFiller::maintainPopulation;
      spokesFillers[i]->maxBitsPerSecond = 10;
      spokesFillers[i]->fadeDown = 0;
      spokesFillers[i]->flowRule = BitsFiller::split;
      spokesFillers[i]->splitDirections = EdgeType::outbound;
    }
  }
  ~CouplingPattern() {
    delete spokesFillers[0];
    delete spokesFillers[1];
  }

  void colorModeChanged() {
    // change bit colors for the new palette immediately for better feedback
    for (int i = 0; i < 2; ++i) {
      spokesFillers[i]->resetBitColors(colorManager);
    }
  }

  void update() {
    static const unsigned relationshipDuration = 1400; // maybe consider therapy if your relationships only last 1400ms
    static const unsigned lookingDuration = 150; // dang, standards++

    unsigned long mils = millis();
    ctx.leds.fadeToBlackBy(3 * frameTime());

    if (state == looking && mils - lastStateChange > lookingDuration) {
      const vector<int> * const planetspokelists[] = {&venusleds, &marsleds};
      const vector<int> * const earthspokelists[] = {&earthleds, &earthasmarsleds, &earthasvenusleds};
      const vector<int> *spokes[2] = {0};

      // omg matchmaking time
      do {
        if (random8(3) != 0) {
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
            uint8_t colorIndex = 0;
            bit.color = colorManager->flagSample(false, &colorIndex);
            bit.colorIndex = colorIndex;
          };
        } else {
          uint8_t colorIndex = 0;
          CRGB solidColor = colorManager->flagSample(false, &colorIndex);
          spokesFillers[i]->handleNewBit = [solidColor, colorIndex](BitsFiller::Bit &bit) {
            bit.color = solidColor;
            bit.colorIndex = colorIndex;
          };
        }
      }
      // start splitting bits down the chosen spokes
      spokesFillers[0]->splitDirections = EdgeType::outbound;
      spokesFillers[1]->splitDirections = EdgeType::outbound;
      state = coupling;
      lastStateChange = mils;

    } else if (state == coupling && mils - lastStateChange > relationshipDuration) {
      // breakup fml
      // stop following the spokes before we change allowedPixels, this helps prevent pixels from stopping mid-spoke during breakups. closure? this feature is called closure.
      for (int base : spoke_base_leds) {
        allowedPixels->erase(base);
      }
      // and allow the bits to flow around in the circle in the meantime
      spokesFillers[0]->splitDirections = EdgeType::all;
      spokesFillers[1]->splitDirections = EdgeType::all;
      state = looking;
      lastStateChange = mils;
    }
    spokesFillers[0]->update();
    spokesFillers[1]->update();
  }

  const char *description() {
    return "coupling";
  }
};

/* ------------------------------------------------------------------------------- */

class ChargePattern : public Pattern {
  BitsFiller *bitsFillers[3] = {0};
  uint32_t spokeActivation[3] = {0};

  void initSpoke(int spoke) {
    if (bitsFillers[spoke] == NULL) {
      bitsFillers[spoke] = new BitsFiller(ctx, 30, 50, 0, {EdgeType::outbound});
      bitsFillers[spoke]->flowRule = BitsFiller::split;
      bitsFillers[spoke]->splitDirections = EdgeType::outbound;
      bitsFillers[spoke]->fadeUpDistance = 2;
      bitsFillers[spoke]->fadeDown = 0;
      bitsFillers[spoke]->maxBitsPerSecond = 25;
      
      static const set<int> *const allowedSets[] = {&circleEarthLeds, &circleVenusLeds, &circleMarsLeds};
      bitsFillers[spoke]->allowedPixels = allowedSets[spoke];

      resetBitHandlers();
    }
    bitsFillers[spoke]->spawnRule = BitsFiller::maintainPopulation;
  }

  void teardownSpoke(int spoke) {
    if (bitsFillers[spoke] != NULL) {
      delete bitsFillers[spoke];
      bitsFillers[spoke] = NULL;
    }
  }
  
  void resetBitHandlers() {
    for (int spoke = 0; spoke < 3; ++spoke) {
      if (bitsFillers[spoke]) {
        bitsFillers[spoke]->handleNewBit = [=](BitsFiller::Bit &bit) {
          static const int cutoffs[] = {circleIndexOppositeEarth, circleIndexOppositeVenus, circleIndexOppositeMars};
          // we pick a spawn point, then figure out which direction is the shortest path to the spoke using cutoffs
          // but add some fuzz to cause some bits fo travel around the point opposite the spoke too.
          int cutoff = cutoffs[spoke];
          int circleindex = mod_wrap(cutoff + random8()%6 - 3, circleleds.size());
          int directionFuzz = random8()%8 - 4;
          
          bit.px = circleleds[circleindex];
          
          if ((unsigned)mod_wrap(circleindex - cutoff + directionFuzz, circleleds.size()) > circleleds.size() / 2) {
            bit.directions.edgeTypes.second = EdgeType::counterclockwise;
          } else {
            bit.directions.edgeTypes.second = EdgeType::clockwise;
          }
          bit.color = colorManager->flagSample(true);
        };
      }
    }
  }

public:
  ~ChargePattern() {
    for (int i = 0; i < 3; ++i) {
      if (bitsFillers[i]) {
        delete bitsFillers[i];
      }
    }
  }

  void colorModeChanged() {
    resetBitHandlers();
  }

  void update() {
    ctx.leds.fadeToBlackBy(5 * frameTime());

    for (int spoke = 0; spoke < 3; ++spoke) {
      if (bitsFillers[spoke]) {
        bitsFillers[spoke]->update();
        if (bitsFillers[spoke]->bits.size() == 0) {
          teardownSpoke(spoke);
        }
      }
    }
  }

  // live responsiveness
  void chargeSpoke(int spoke) {
    initSpoke(spoke);
    spokeActivation[spoke] = millis();
  }

  void runSpoke(int spoke) {
    initSpoke(spoke);
    spokeActivation[spoke] = UINT32_MAX;
  }

  void stopChargingSpoke(int spoke, unsigned long chargeDuration) {
    if (millis() - spokeActivation[spoke] < chargeDuration) {
      if (bitsFillers[spoke]) {
        bitsFillers[spoke]->spawnRule = BitsFiller::manualSpawn;
      }
    }
  }

  void stopAllSpokes() {
    for (int i = 0; i < 3; ++i) {
      teardownSpoke(i);
    }
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
  IntersexFlagPattern() : outerBits(ctx, 20, 40, 4000, {EdgeType::inbound}), 
                          innerBits(ctx, 8, 40, 4000, {EdgeType::clockwise | EdgeType::counterclockwise}) {
    logf("intersex flag init");
    spokePixels.insert(earthleds.begin(), earthleds.end());
    spokePixels.insert(venusleds.begin(), venusleds.end());
    spokePixels.insert(marsleds.begin(), marsleds.end());

    outerBits.allowedPixels = &spokePixels; // keeps pixels from following the last inbound edge onto the circle
    outerBits.spawnPixels = &leafleds;
    outerBits.fadeUpDistance = 2;
    outerBits.fadeDown = 0;
    outerBits.maxBitsPerSecond = 30;
    outerBits.handleNewBit = [](BitsFiller::Bit &bit) {
      bit.color = CRGB::Yellow;
    };

    innerBits.spawnPixels = &circleleds;
    innerBits.fadeUpDistance = 2;
    innerBits.fadeDown = 0;
    innerBits.maxBitsPerSecond = 8;
    innerBits.handleNewBit = [](BitsFiller::Bit &bit) {
      bit.color = CRGB(0x6E, 0x07, 0xD7);
    };
  }

  void update() {
    ctx.leds.fadeToBlackBy(4 * frameTime());

    outerBits.update();
    innerBits.update();
  }

  const char *description() {
    return "intersex";
  }
};

/* ------------------------------------------------------------------------------- */

class SoundBits : public Pattern, public FFTProcessing {
  BitsFiller bitsFillerOut;
  BitsFiller bitsFillerIn;
public:
  SoundBits() : bitsFillerOut(ctx, 0, 60, 1200, {EdgeType::outbound, EdgeType::clockwise | EdgeType::counterclockwise}),
                bitsFillerIn(ctx, 0, 60, 1200, {EdgeType::inbound, EdgeType::clockwise | EdgeType::counterclockwise}) {
    bitsFillerOut.flowRule = BitsFiller::random;
    bitsFillerOut.fadeUpDistance = 3;
    bitsFillerOut.spawnPixels = &circleleds;
    bitsFillerOut.fadeDown = 0;
    bitsFillerOut.handleUpdateBit = [](BitsFiller::Bit &bit) {
      int raw = min(0xFF, max(0, (int)(0xFF - 0xFF * bit.age() / bit.lifespan)));
      bit.brightness = raw;
    };

    bitsFillerIn.flowRule = BitsFiller::random;
    bitsFillerIn.fadeUpDistance = 3;
    bitsFillerIn.spawnPixels = &leafleds;
    bitsFillerOut.fadeDown = 0;
    bitsFillerIn.handleUpdateBit = [](BitsFiller::Bit &bit) {
      int raw = min(0xFF, max(0, (int)(0xFF - 0xFF * bit.age() / bit.lifespan)));
      bit.brightness = raw;
    };
  }

  void colorModeChanged() {
    // change bit colors for the new palette immediately for better feedback
    bitsFillerOut.resetBitColors(colorManager);
    bitsFillerIn.resetBitColors(colorManager);
  }

  const unsigned maxbits = 50;
  const int soundMinThreshold = 3;
  int soundThreshold = soundMinThreshold;
  unsigned long lastThreshAdjust = 0;

  void update() {
    ctx.leds.fadeToBlackBy(4 * frameTime());

    vector<int> spectrum = fftUpdate().spectrum;
    // fftLog(spectrum);

    for (unsigned freqBucket = 0; freqBucket < spectrum.size(); ++freqBucket) {
      if (spectrum[freqBucket] > soundThreshold) {
        
        if (bitsFillerOut.bits.size() + bitsFillerIn.bits.size() < maxbits) {
          // loglf("levels[%i]: %i; making a bit; out bits = %u, in bits = %u...", b, spectrum[b], bitsFillerOut.bits.size(), bitsFillerIn.bits.size());
          bool spawnoutbound = freqBucket < spectrum.size() / 5;
          unsigned maxlifespan = spawnoutbound ? 2000 : 1000;
          BitsFiller::Bit &bit = (spawnoutbound ? bitsFillerOut : bitsFillerIn).addBit();
          bit.lifespan = min(maxlifespan, maxlifespan * (spectrum[freqBucket]-soundThreshold)/20);
          // logf("done");                                                  

          uint8_t colorIndex = millis() / 100 + 0xFF * freqBucket / 13;
          CRGB color = colorManager->getPaletteColor(colorIndex);
          color.nscale8(min(0xFF, 0xFF * (spectrum[freqBucket]-soundThreshold)/10));
          bit.color = color;
          bit.colorIndex = colorIndex;
        }
      }
    }
    // last effort dirty gain management
    unsigned extantBits = bitsFillerOut.bits.size() + bitsFillerIn.bits.size();
    if (millis() - lastThreshAdjust > 1000) {
      if (extantBits >= (maxbits >> 1)) {
        soundThreshold++;
        logf("Is Loud. bit thresh to %i", soundThreshold);
        lastThreshAdjust = millis();
      } else if (extantBits < 5 && soundThreshold > soundMinThreshold) {
        soundThreshold--;
        logf("Is Quiet. bit thresh to %i", soundThreshold);
        lastThreshAdjust = millis();
      }
    }

    bitsFillerOut.update();
    bitsFillerIn.update();
  }

  const char *description() {
    return "SoundBits";
  }
};

class SoundTest : public Pattern, public FFTProcessing {
  BitsFiller bitsFiller;
public:
  SoundTest() : bitsFiller(ctx, 0, 60, 1200, {EdgeType::outbound, EdgeType::clockwise | EdgeType::counterclockwise}) {
    bitsFiller.flowRule = BitsFiller::random;
    bitsFiller.fadeUpDistance = 3;
    bitsFiller.spawnPixels = &circleleds;
    bitsFiller.handleUpdateBit = [](BitsFiller::Bit &bit) {
      int raw = min(0xFF, max(0, (int)(0xFF - 0xFF * bit.age() / bit.lifespan)));
      bit.brightness = raw;
    };
  }

  void update() {
    vector<int> spectrum = fftUpdate().spectrum;
    fftLog(spectrum);
    
    bitsFiller.update();
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
