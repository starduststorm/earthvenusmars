#define DEBUG 0

// for memory logging
#ifdef __arm__
extern "C" char* sbrk(int incr);
#else
extern char *__brkval;
#endif

#include <Arduino.h>
// Arduino on samd21 defines min and max differently than the STL so we need to undefine them
#undef min
#undef max

// since we're using the native port of the "arduino zero", not the programming port
#define Serial SerialUSB

#include "config.h"

#include <SPI.h>
#include "wiring_private.h" // pinPeripheral() function
SPIClass ledsSPI (&sercom3, LEDS_MISO, LEDS_SCK, LEDS_MOSI, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_1);

#include <I2S.h>
I2SClass micI2S (0, MIC_CLK_GEN, MIC_SD, MIC_SCK, MIC_FS);

static const uint8_t brightnessDialPort = PORTA;
static const uint8_t brightnessDialPin = 9;

/* --------------------------------- */

#define FASTLED_USE_PROGMEM 1
// #define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

#include "util.h"
#include "drawing.h"
#include "PatternManager.h"
#include "power.h"
#include "ledgraph.h"

#include <functional>

#define WAIT_FOR_SERIAL 0

EVMDrawingContext ctx;

FrameCounter fc;
PatternManager<EVMDrawingContext> patternManager(ctx);
PowerManager powerManager;

static bool serialTimeout = false;
static unsigned long setupDoneTime;

void startupWelcome() {
  int pixelIndices[NUM_LEDS];
  for (int i = 0; i < NUM_LEDS; ++i) {
    pixelIndices[i] = i;
  }
  shuffle<NUM_LEDS>(pixelIndices);

  const uint8_t colorShift = random8(2, 0xFF); // exclude 0,1 to avoid low color distribution
  const int sprinkleDuration = 500;
  const int eachPxDuration = 800;

  ctx.leds.fill_solid(CRGB::Black);

  DrawModal(120, sprinkleDuration + eachPxDuration, [pixelIndices, colorShift](unsigned long elapsed) {
    int maxPx = min(NUM_LEDS, NUM_LEDS * (int)elapsed / sprinkleDuration);
    
    for (int i = 0; i < maxPx; ++i) {
      int px = pixelIndices[i];
      CRGB color = ColorFromPalette((CRGBPalette32)Trans_Flag_gp, i * colorShift);
      unsigned long start = i * sprinkleDuration / NUM_LEDS;
      if (elapsed > start && elapsed < start + eachPxDuration) {
        // set segment brightness based on sin [0,pi]
        uint8_t brightness = sin16((elapsed - start) * 0x7FFF / eachPxDuration) >> 8;
        color.nscale8(dim8_raw(brightness));
      } else {
        color = CRGB::Black;
      }
      
      ctx.leds[px] = color;
      powerManager.loop(ctx);
    }
  });
  ctx.leds.fill_solid(CRGB::Black);
  FastLED.show();
}

void setup() {
  Serial.begin(57600);
  
#if WAIT_FOR_SERIAL
  long setupStart = millis();
  while (!Serial) {
    if (millis() - setupStart > 10000) {
      serialTimeout = true;
      break;
    }
  }
  logf("begin - waited %ims for Serial", millis() - setupStart);
#elif DEBUG
  delay(2000);
  Serial.println("Done waiting at boot.");
#endif
  randomSeed(lsb_noise(UNCONNECTED_PIN_1, 8 * sizeof(uint32_t)));
  random16_add_entropy(lsb_noise(UNCONNECTED_PIN_2, 8 * sizeof(uint16_t)));
  
  powerManager.setup_adc(brightnessDialPort, brightnessDialPin);

  // use the alternate sercoms each of these SPI ports (SERCOM3)
  pinPeripheral(LEDS_MISO, PIO_SERCOM_ALT);
  pinPeripheral(LEDS_SCK,  PIO_SERCOM_ALT);
  pinPeripheral(LEDS_MOSI, PIO_SERCOM_ALT);

  // Make sure we're not running the SPI while in standby
  SERCOM3->SPI.CTRLA.bit.RUNSTDBY = 0;

  FastLED.addLeds<APA102, LEDS_MOSI, LEDS_SCK, BGR>(ctx.leds, ctx.leds.size());
  FastLED.setBrightness(0);

  fc.tick();

  patternManager.setup();

  initLEDGraph();
  assert(ledgraph.adjList.size() == NUM_LEDS, "adjlist size should match num_leds");

  setupDoneTime = millis();
}

void serialTimeoutIndicator() {
  FastLED.setBrightness(50);
  ctx.leds.fill_solid(CRGB::Black);
  if ((millis() - setupDoneTime) % 250 < 100) {
    ctx.leds.fill_solid(CRGB::Red);
  }
  FastLED.show();
  delay(20);
}

void testPalette(CRGBPalette256 &palette) {
  for (unsigned i = 0; i < circleleds.size(); ++i) {
    ctx.leds[circleleds[i]] = ColorFromPalette(palette, 0xFF * i / circleleds.size());
  }
}

void loop() {
  if (serialTimeout && millis() - setupDoneTime < 1000) {
    serialTimeoutIndicator();
    return;
  }

  powerManager.loop(ctx);

  static bool firstLoop = true;
  if (firstLoop) {
    startupWelcome();
    firstLoop = false;
  }

  patternManager.loop();
  
  FastLED.show();

  fc.tick();
  fc.clampToFramerate(120);
}
