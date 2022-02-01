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
    }

    void reset() {
      birthmilli = millis();
      color = CHSV(random8(), 0xFF, 0xFF);
    }

    unsigned long age() {
      return millis() - birthmilli;
    }
  };

private:

  EVMDrawingContext &ctx;

  unsigned long lastTick = 0;
  unsigned long lastMove = 0;
  unsigned long lastBitSpawn = 0;

  uint8_t spawnLocation() {
    if (spawnPixels) {
      return spawnPixels->at(random8()%spawnPixels->size());
    }
    return random16()%NUM_LEDS;
  }

  Bit &makeBit(Bit *fromBit=NULL) {
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
      bits.emplace_back(*fromBit);
    } else {
      bits.emplace_back(spawnLocation(), directionsForBit, lifespan);
    }
    return bits.back();
  }

  void killBit(uint8_t bitIndex) {
    bits.erase(bits.begin() + bitIndex);
  }

  void splitBit(Bit &bit, uint8_t toIndex) {
    Bit &split = makeBit(&bit);
    split.px = toIndex;
  }

  bool isIndexAllowed(uint8_t index) {
    if (allowedPixels) {
      return allowedPixels->end() != allowedPixels->find(index);
    }
    return true;
  }

  vector<uint8_t> nextIndexes(uint8_t index, EdgeTypesPair bitDirections) {
    vector<uint8_t> next;
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

  bool flowBit(uint8_t bitIndex) {
    vector<uint8_t> next = nextIndexes(bits[bitIndex].px, bits[bitIndex].directions);
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
  
  const vector<uint8_t> *spawnPixels = NULL; // list of pixels to automatically spawn bits on
  const set<uint8_t> *allowedPixels = NULL; // set of pixels that bits are allowed to travel to

  function<void(Bit &)> handleNewBit = [](Bit &bit){};
  function<void(Bit &)> handleUpdateBit = [](Bit &bit){};

  BitsFiller(EVMDrawingContext &ctx, uint8_t maxSpawnBits, uint8_t speed, unsigned long lifespan, vector<EdgeTypes> bitDirections)
    : ctx(ctx), maxSpawnBits(maxSpawnBits), speed(speed), lifespan(lifespan) {
      this->bitDirections = MakeEdgeTypesPair(bitDirections);
    bits.reserve(maxSpawnBits);
  };

  void fadeUpForBit(Bit &bit, uint8_t px, int distanceRemaining, unsigned long lastMove) {
    vector<uint8_t> next = nextIndexes(px, bit.directions);

    unsigned long mils = millis();
    unsigned long fadeUpDuration = 1000 * fadeUpDistance / speed;
    for (uint8_t n : next) {
      unsigned long fadeTimeSoFar = mils - lastMove + distanceRemaining * 1000/speed;
      uint8_t progress = 0xFF * fadeTimeSoFar / fadeUpDuration;

      CRGB existing = ctx.leds[n];
      CRGB blended = blend(existing, bit.color, dim8_raw(progress));
      blended.nscale8(bit.brightness);
      ctx.leds[n] = blended;
      
      if (distanceRemaining > 0) {
        fadeUpForBit(bit, n, distanceRemaining-1, lastMove);
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
          fadeUpForBit(bit, bit.px, bitFadeUpDistance - 1, lastMove);
        }
      }
    }

    lastTick = mils;

    for (Bit &bit : bits) {
      bit.firstFrame = false;
    }
  };

  Bit &addBit() {
    Bit &newbit = makeBit();
    handleNewBit(newbit);
    return newbit;
  }

  void removeAllBits() {
    bits.clear();
  }

  void resetBitColors(EVMColorManager *colorManager) {
    for (Bit &bit : bits) {
      bit.color = colorManager->getPaletteColor(bit.colorIndex, bit.color.getAverageLight());
    }
  }
};

/* ------------------------------------------------------------------------------- */

class DownstreamPattern : public Pattern {
protected:
  BitsFiller *bitsFiller;
  unsigned circleBits = 0;
  unsigned numAutoRotateColors = 3;
  unsigned numAutoRotatePaletteCycles = 1;
public:
  DownstreamPattern() {
    EdgeType circledirection = (random8()%2 ? EdgeType::clockwise : EdgeType::counterclockwise);
    vector<EdgeTypes> directions = {circledirection, EdgeType::outbound};
    bitsFiller = new BitsFiller(ctx, 0, 24, 0, directions);
    bitsFiller->flowRule = BitsFiller::split;
  }
  ~DownstreamPattern() {
    delete bitsFiller;
  }

  void update() {
    bitsFiller->update();

    for (int i = 0; i < colorManager->trackedColorsCount(); ++i) {
      bitsFiller->bits[i].color = colorManager->getTrackedColor(i);
    }
    colorManager->paletteRotationTick();
  }

  virtual void colorModeChanged() {
    unsigned oldCircleBits = circleBits;
    if (colorManager->pauseRotation) {
      // color manager will track flag bands
      circleBits = colorManager->trackedColorsCount();
    } else {
      // keep it simple with 3 bits with doing full palette rotation
      colorManager->prepareTrackedColors(numAutoRotateColors, numAutoRotatePaletteCycles);
      circleBits = numAutoRotateColors;
    }
     
    if (circleBits != oldCircleBits) {
      bitsFiller->removeAllBits();
      for (unsigned i = 0; i < circleBits; ++i) {
        BitsFiller::Bit &bit = bitsFiller->addBit();
        bit.px = circleleds[i * circleleds.size() / circleBits];
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
  }
  void colorModeChanged() {
    numAutoRotateColors = 17;
    numAutoRotatePaletteCycles = 3;

    DownstreamPattern::colorModeChanged();

    bitsFiller->fadeDown = 0;
    bitsFiller->fadeUpDistance = 1;
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

// optional feature to beat the heart in response to external timing pulse (RISING pulse on SDA pin)
#define USE_PACEMAKER false

volatile bool pumpPacemaker = false;
#if USE_PACEMAKER
void pacemakerBeat() {
  pumpPacemaker = true;
}
#endif

class HeartBeatPattern : public Pattern, public FFTProcessing {
  const uint8_t basebpm = 60;
  uint8_t bpm = 40;
  unsigned long lastSystole = 0;
  unsigned long diastoleAt = 0;
  const CRGB bloodColor = CRGB(0xFF, 0, 0x15);
  
  int fadeDown = 3;
  float avgAmp = 0;

  bool usingPacemaker = false;

  BitsFiller pumpFiller;
public:
  HeartBeatPattern() : pumpFiller(ctx, 0, 30, 1200, {EdgeType::outbound}) {
    pumpFiller.flowRule = BitsFiller::split;
    pumpFiller.fadeDown = fadeDown;
    pumpFiller.splitDirections = EdgeType::outbound;
#if USE_PACEMAKER
    pinMode(SDA, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(SDA), pacemakerBeat, RISING);
#endif
  }

  ~HeartBeatPattern() {
#if USE_PACEMAKER
    detachInterrupt(SDA);
#endif
  }

  void beat(bool parity, uint8_t intensity) {
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
    if (pumpPacemaker) {
      pumpPacemaker = false;
      usingPacemaker = true;
      beat(false, 0x8F);
    }
    if (!usingPacemaker) {
      unsigned long mils = millis();
      unsigned milsPerBeat = 1000 * 60 / bpm;
      if (mils - lastSystole > milsPerBeat) {
        beat(true, 0xFF);
        lastSystole = mils;
        diastoleAt = lastSystole + milsPerBeat * 0.24;
      }
      if (diastoleAt != 0 && mils > diastoleAt) {
        beat(false, 0x8F);
        diastoleAt = 0;
      }

      // louder -> get yo blood pumpin
      const unsigned ampSamples = 1200;
      const unsigned int ampBaseline = 494;
      unsigned long amplitude = min(1000u, fftUpdate().amplitude);
      amplitude = max(0, (long)amplitude - (long)ampBaseline);
      avgAmp = (avgAmp * (ampSamples-1) + amplitude) / ampSamples;
      bpm = min(150, basebpm + 3 * avgAmp);
    }

    pumpFiller.update();
  }

  const char *description() {
    return "heartbeat";
  }
};

/* ------------------------------------------------------------------------------- */

// global state! these help the Coupling pattern not draw silly things like just the line of a spoke
bool earthVenusSuppressed = false;
bool earthMarsSuppressed = false;

class CouplingPattern : public Pattern {
  enum { coupling, looking } state = looking;
  BitsFiller *spokesFillers[2];
  set<uint8_t> allowedPixels[2];
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
      const vector<uint8_t> * const planetspokelists[] = {&venusleds, &marsleds};
      const vector<uint8_t> * const earthspokelists[] = {&earthleds, &earthasvenusleds, &earthasmarsleds};
      const vector<uint8_t> *spokes[2] = {0};

      // omg matchmaking time
      do {
        if (random8(3) != 0) {
          spokes[0] = ARRAY_SAMPLE(earthspokelists);
        } else {
          spokes[0] = ARRAY_SAMPLE(planetspokelists);
        }
        spokes[1] = ARRAY_SAMPLE(planetspokelists);
      } while (spokes[0] == spokes[1]
              // don't show a single line on the spoke if part of it is suppressed
              || (spokes[0] == &earthasvenusleds && earthVenusSuppressed)
              || (spokes[0] == &earthasmarsleds && earthMarsSuppressed));

      for (int i = 0; i < 2; ++i) {
        // FIXME: replace with the new constants from ledgraph?
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

class SpokePattern {
  bool baseActive;
protected:
  EVMDrawingContext &ctx;
  EVMDrawingContext &subtractCtx;
public:
  FlagPalette<CRGBPalette16> flagPalette;
  EVMColorManager &sharedColorManager;
  uint8_t spoke = spoke;
  bool useSharedPalette = true;

  SpokePattern(EVMDrawingContext &ctx, EVMDrawingContext &subtractCtx, EVMColorManager &sharedColorManager, uint8_t spoke) : ctx(ctx), subtractCtx(subtractCtx), sharedColorManager(sharedColorManager), spoke(spoke) { }
  virtual ~SpokePattern() { }
  void colorModeChanged() { }
  void nextPalette() {
    if (useSharedPalette && sharedColorManager.pauseRotation) {
      // start from the current shared palette
      flagPalette.setFlagIndex(sharedColorManager.getFlagIndex());
    }
    flagPalette.nextPalette();
    useSharedPalette = false;
  }

  void previousPalette() {
    if (useSharedPalette && sharedColorManager.pauseRotation) {
      // start from the current shared palette
      flagPalette.setFlagIndex(sharedColorManager.getFlagIndex());
    }
    flagPalette.previousPalette();
    useSharedPalette = false;
  }

  CRGB getAutoColor(unsigned long msPerCycle=500, uint8_t randomOffset=20) {
    if (useSharedPalette) {
      return sharedColorManager.flagSample(true, NULL, msPerCycle, randomOffset);
    } else {
      return flagPalette.flagSample(true, NULL, msPerCycle, randomOffset);
    }
  }

  virtual void update() = 0;
  
  virtual bool isIdle() {
    return !baseActive;
  }
  virtual void setActive(bool active) {
    baseActive = active;
  }
  // allows the Earth spoke to have more modes; returns false if no mode switch happened
  virtual bool nextMode() { return false; }
  virtual bool previousMode() { return false; }
  virtual int8_t getMode() { return 0; }
  virtual bool setMode(int8_t mode) { return false; };
  // false if this spoke pattern doesn't draw anything and shouldn't dim the background
  virtual bool isDrawing() { return true; }
};

/* -------- */

class ChargeSpokePattern : public SpokePattern {
  BitsFiller *bitsFiller;
  void resetBitHandler() {
    bitsFiller->handleNewBit = [=](BitsFiller::Bit &bit) {
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
      bit.color = getAutoColor(500, 0);
    };
  }
public:
  ChargeSpokePattern(EVMDrawingContext &ctx, EVMDrawingContext &subtractCtx, EVMColorManager &sharedColorManager, uint8_t spoke) : SpokePattern(ctx, subtractCtx, sharedColorManager, spoke) {
    bitsFiller = new BitsFiller(ctx, 30, 50, 0, {EdgeType::outbound});
    bitsFiller->flowRule = BitsFiller::split;
    bitsFiller->splitDirections = EdgeType::outbound;
    bitsFiller->fadeUpDistance = 2;
    bitsFiller->fadeDown = 0;
    bitsFiller->maxBitsPerSecond = 25;
    bitsFiller->spawnRule = BitsFiller::maintainPopulation;
    bitsFiller->allowedPixels = kSpokeCircleLedSets[spoke];

    resetBitHandler();
  }

  ~ChargeSpokePattern() {
    delete bitsFiller;
  }

  void colorModeChanged() {
    resetBitHandler();
  }

  void update() {
    bitsFiller->update();
  }

  void setActive(bool active) {
    bitsFiller->spawnRule = (active ? BitsFiller::maintainPopulation : BitsFiller::manualSpawn);
  }

  bool isIdle() {
    return bitsFiller->bits.size() == 0;
  }
};

class SparkleSpokePattern : public SpokePattern {
  unsigned long lastSpark = 0;
  std::vector<uint8_t> spawnIndexes;
public:
  SparkleSpokePattern(EVMDrawingContext &ctx, EVMDrawingContext &subtractCtx, EVMColorManager &sharedColorManager, uint8_t spoke) : SpokePattern(ctx, subtractCtx, sharedColorManager, spoke) {
    for (auto item : *(kSpokeCircleLedSets[spoke])) {
      spawnIndexes.push_back(item);
    }
  }

  ~SparkleSpokePattern() {
  }

  void colorModeChanged() {
  }

  void update() {
    if (millis() - lastSpark > 10) {
      for (int i = 0; i < 5; ++i) {
        uint8_t index = spawnIndexes[random8(spawnIndexes.size())];
        CRGB color = getAutoColor(1000, 80);
        ctx.leds[index] = color;
        lastSpark = millis();
      }
    }
  }
};

// Used to turn off a spoke entirely
class SuppressSpokePattern : public SpokePattern {
  enum : int8_t { earth = 0, venus = 1, mars = 2, };
  int8_t earthAs = earth;
  unsigned long start = 0;
public:
  SuppressSpokePattern(EVMDrawingContext &ctx, EVMDrawingContext &subtractCtx, EVMColorManager &sharedColorManager, uint8_t spoke) : SpokePattern(ctx, subtractCtx, sharedColorManager, spoke) { }
  ~SuppressSpokePattern() {
    if (spoke == earth) {
      // inform global state that we're no longer hiding these partial spokes
      earthMarsSuppressed = false;
      earthVenusSuppressed = false;
    }
  }

  const vector<uint8_t>& ledList() {
    if (spoke == earth) {
      earthMarsSuppressed = (earthAs == mars);
      earthVenusSuppressed = (earthAs == venus);
      const vector<uint8_t> * const earthspokelists[] = {&earthleds, &earthVenusOnly, &earthMarsOnly};
      return *earthspokelists[earthAs];
    } else {
      return *kSpokeLedLists[spoke];
    }
  }

  void update() {
    const unsigned long kFadeTime = 500; // matching long-press interval for now
    if (start == 0) {
      start = millis();
    }
    const vector<uint8_t>& leds = ledList();
    unsigned long runTime = millis() - start;
    for (uint8_t index : leds) {
       ctx.leds[index] = CRGB::White;
    }
    for (uint8_t index : leds) {
      subtractCtx.leds[index] = CRGB::White;
      subtractCtx.leds[index].nscale8(min(kFadeTime, runTime) * 0xFF / kFadeTime);
    }
  }

  void setActive(bool active) {
    SpokePattern::setActive(active);
    if (!active) {
      for (uint8_t index : ledList()) {
         ctx.leds[index] = CRGB::White;
      }
    }
  }

  bool nextMode() {
    if (spoke == earth && earthAs < mars) {
      ++earthAs;
      start = millis();
      return true;
    }
    return false;
  }

  bool previousMode() {
    if (spoke == earth && earthAs > earth) {
      --earthAs;
      start = millis();
      return true;
    }
    return false;
  }

  int8_t getMode() {
    if (spoke == earth) {
      return earthAs;
    }
    return 0;
  }

  bool setMode(int8_t mode) {
    if (spoke == earth) {
      earthAs = mode;
      return true;
    }
    return false;
  }

  bool isDrawing() { return false; }
};

/* ----------------------------------------- */

class SpokePatternManager : public Pattern {
  static const uint32_t SpokeInactive = UINT32_MAX;
  uint32_t spokeActivation[3] = {SpokeInactive, SpokeInactive, SpokeInactive};

  int spokePatternIndex[3] = {0};
  SpokePattern *spokePatterns[3] = {0};
  int8_t spokeMode[3] = {0};

  // slight redundancy - these values are also stored in the SpokePattern for convenience, but need to be persistantly stored here
  uint8_t spokeFlagIndexes[3] = {0};
  bool useSharedPalettes[3] = {true, true, true};

  std::vector<SpokePattern * (*)(EVMDrawingContext&, EVMDrawingContext&, EVMColorManager&, uint8_t)> patternConstructors;
  template<class T>
  static SpokePattern *construct(EVMDrawingContext &ctx, EVMDrawingContext &subtractCtx, EVMColorManager &colorManager, uint8_t spoke) {
    return new T(ctx, subtractCtx, colorManager, spoke);
  }

private:
  void initSpoke(uint8_t spoke) {
    spokeActivation[spoke] = millis();
    if (!spokePatterns[spoke]) {
      auto ctor = patternConstructors[spokePatternIndex[spoke]];
      spokePatterns[spoke] = ctor(this->ctx, this->subtractCtx, *colorManager, spoke);
      spokePatterns[spoke]->setMode(spokeMode[spoke]);
      spokePatterns[spoke]->setActive(true);

      spokePatterns[spoke]->useSharedPalette = useSharedPalettes[spoke];
      spokePatterns[spoke]->flagPalette.setFlagIndex(spokeFlagIndexes[spoke]);
    }
  }
public:

  EVMDrawingContext subtractCtx;

  SpokePatternManager() {
    patternConstructors.push_back(&(construct<ChargeSpokePattern>));
    patternConstructors.push_back(&(construct<SparkleSpokePattern>));
    patternConstructors.push_back(&(construct<SuppressSpokePattern>));
  }

  void stopSpoke(uint8_t spoke) {
    spokePatterns[spoke]->setActive(false);
    if (spokePatterns[spoke]->isIdle()) {
      // some spokes may be immediately idle and some may want to spin down
      teardownSpoke(spoke);
    }
  }

  // live responsiveness
  void spokeTapDown(uint8_t spoke) {
    initSpoke(spoke);
  }

  void spokeTapUp(uint8_t spoke, unsigned long chargeDuration) {
    if (millis() - spokeActivation[spoke] < chargeDuration) {
      assert(spokePatterns[spoke] != NULL, "stop charging a non-existent spoke?");
      if (spokePatterns[spoke]) {
        stopSpoke(spoke);
      }
    }
  }

#if EVM_HARDWARE_VERSION == 1
  // legacy
  void runSpoke(uint8_t spoke) {
    initSpoke(spoke);
  }
  void stopAllSpokes() {
    for (int spoke = 0; spoke < 3; ++spoke) {
      if (spokePatterns[spoke]) {
        stopSpoke(spoke);
      }
    }
  }
#endif

  void teardownSpoke(uint8_t spoke) {
    spokeMode[spoke] = spokePatterns[spoke]->getMode();
    spokeActivation[spoke] = SpokeInactive;
    delete spokePatterns[spoke];
    spokePatterns[spoke] = NULL;
  }

  uint8_t drawingSpokeCount() {
    uint8_t count = 0;
    for (int spoke = 0; spoke < 3; ++spoke) {
      if (spokePatterns[spoke] && spokePatterns[spoke]->isDrawing()) {
        ++count;
      }
    }
    return count;
  }

  void colorModeChanged() {
    for (int spoke = 0; spoke < 3; ++spoke) {
      if (spokePatterns[spoke]) {
        spokePatterns[spoke]->colorModeChanged();
      }
    }
  }

  void nextPalette(uint8_t spoke) {
    spokePatterns[spoke]->nextPalette();
    useSharedPalettes[spoke] = spokePatterns[spoke]->useSharedPalette;
    spokeFlagIndexes[spoke] = spokePatterns[spoke]->flagPalette.getFlagIndex();
  }

  void previousPalette(uint8_t spoke) {
    spokePatterns[spoke]->previousPalette();
    useSharedPalettes[spoke] = spokePatterns[spoke]->useSharedPalette;
    spokeFlagIndexes[spoke] = spokePatterns[spoke]->flagPalette.getFlagIndex();
  }

  void nextPattern(uint8_t spoke) {
    if (!spokePatterns[spoke]->nextMode()) {
      teardownSpoke(spoke);
      spokePatternIndex[spoke] = addmod8(spokePatternIndex[spoke], 1, patternConstructors.size());
      initSpoke(spoke);
    }
  }

  void previousPattern(uint8_t spoke) {
    if (!spokePatterns[spoke]->previousMode()) {
      teardownSpoke(spoke);
      spokePatternIndex[spoke] = mod_wrap(spokePatternIndex[spoke] - 1, patternConstructors.size());
      initSpoke(spoke);
    }
  }

  void update() {
    ctx.leds.fadeToBlackBy(5 * frameTime());
    subtractCtx.leds.fadeToBlackBy(5 * frameTime());

    for (int spoke = 0; spoke < 3; ++spoke) {
      if (spokePatterns[spoke]) {
        spokePatterns[spoke]->update();
        if (spokePatterns[spoke]->isIdle()) {
          teardownSpoke(spoke);
        }
      }
    }
  }

  const char *description() {
    return "SpokeManager";
  }
};

/* ------------------------------------------------------------------------------- */

class IntersexFlagPattern : public Pattern {
  BitsFiller outerBits;
  BitsFiller innerBits;
  std::set<uint8_t> spokePixels;
public:
  IntersexFlagPattern() : outerBits(ctx, 20, 40, 4000, {EdgeType::inbound}), 
                          innerBits(ctx, 8, 40, 4000, {EdgeType::clockwise | EdgeType::counterclockwise}) {
    spokePixels.insert(earthleds.begin(), earthleds.end());
    spokePixels.insert(venusleds.begin(), venusleds.end());
    spokePixels.insert(marsleds.begin(), marsleds.end());

    outerBits.allowedPixels = &spokePixels; // keeps pixels from following the last inbound edge onto the circle
    outerBits.spawnPixels = &leafleds;
    outerBits.fadeUpDistance = 2;
    outerBits.fadeDown = 0;
    outerBits.maxBitsPerSecond = 30;

    innerBits.spawnPixels = &circleleds;
    innerBits.fadeUpDistance = 2;
    innerBits.fadeDown = 0;
    innerBits.maxBitsPerSecond = 8;
  }

  unsigned long lastColorShift = 0;

  void update() {
    ctx.leds.fadeToBlackBy(4 * frameTime());

    outerBits.update();
    innerBits.update();
    
    if ((!colorManager->pauseRotation || colorManager->getFlagIndex() != 3) && millis() - lastColorShift > 40) {
      colorManager->shiftTrackedColors(1);
      lastColorShift = millis();
    }
  }

  CRGB colorForBit(BitsFiller::Bit &bit, BitsFiller *filler) {
    uint8_t colorCount = colorManager->trackedColorsCount();
    assert(colorCount > 1, "not tracking colors?");
    uint8_t tracked = 0;
    if (colorManager->pauseRotation && colorManager->getFlagIndex() == 3) {
      // hack for intersex flag palette which is what this pattern was originally written for
      tracked = (filler == &outerBits ? 1 : 0);
    } else {
      if (colorCount > 3) {
        if (onEarth(bit.px)) tracked = 1;
        else if (onVenus(bit.px)) tracked = 2;
        else if (onMars(bit.px)) tracked = 3;
      } else {
        bool onCircle = (filler == &innerBits);
        tracked = (onCircle ? 0 : random8(colorCount-1) + 1);
      }
    }
    return this->colorManager->getTrackedColor(tracked);
  }

  void colorModeChanged() {
    if (!colorManager->pauseRotation) {
      // 4 lets us track the ring & spokes separately
      colorManager->prepareTrackedColors(4);
    }
    innerBits.handleNewBit = [this](BitsFiller::Bit &bit) {
      bit.color = this->colorForBit(bit, &innerBits);
    };
    outerBits.handleNewBit = [this](BitsFiller::Bit &bit) {
      bit.color = this->colorForBit(bit, &outerBits);
    };

    // change bit colors for the new palette immediately for better feedback
    for (BitsFiller::Bit &bit : outerBits.bits) {
      bit.color = colorForBit(bit, &outerBits);
    }
    for (BitsFiller::Bit &bit : innerBits.bits) {
      bit.color = colorForBit(bit, &innerBits);
    }
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

    FFTFrame fftFrame = fftUpdate();
    // fftLog(spectrum);

    for (unsigned freqBucket = 0; freqBucket < fftFrame.size; ++freqBucket) {
      if (fftFrame.spectrum[freqBucket] > soundThreshold) {
        
        if (bitsFillerOut.bits.size() + bitsFillerIn.bits.size() < maxbits) {
          // loglf("levels[%i]: %i; making a bit; out bits = %u, in bits = %u...", b, spectrum[b], bitsFillerOut.bits.size(), bitsFillerIn.bits.size());
          bool spawnoutbound = freqBucket < fftFrame.size / 5;
          unsigned maxlifespan = spawnoutbound ? 2000 : 1000;
          BitsFiller::Bit &bit = (spawnoutbound ? bitsFillerOut : bitsFillerIn).addBit();
          bit.lifespan = min(maxlifespan, maxlifespan * (fftFrame.spectrum[freqBucket]-soundThreshold)/20);
          // logf("done");                                                  

          uint8_t colorIndex = millis() / 100 + 0xFF * freqBucket / 13;
          CRGB color = colorManager->getPaletteColor(colorIndex);
          color.nscale8(min(0xFF, 0xFF * (fftFrame.spectrum[freqBucket]-soundThreshold)/10));
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
    FFTFrame fftFrame = fftUpdate();
    fftLog();
    
    bitsFiller.update();
    ctx.leds.fadeToBlackBy(15);
    for (unsigned b = 0; b < fftFrame.size; ++b) {
      int thresh = 5;
      if (fftFrame.spectrum[b] > thresh) {
        for (unsigned i = 0; i < circleleds.size(); ++i) {
          ctx.leds[circleleds[i]] += CHSV(0xFF*(b-2)/8, 0xFF, min(0xFF, 0xFF * (fftFrame.spectrum[b]-thresh) / 10));
        }
      }
    }
  }

  const char *description() {
    return "soundtest";
  }
};

#endif
