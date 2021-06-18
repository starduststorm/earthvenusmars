#define DEBUG 1

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

#include <SPI.h>
#include "wiring_private.h" // pinPeripheral() function

/*
 * SAMD21 pins
 * PA17  D13   SERCOM1.1 SERCOM3.1 // MISO
 * PA19  D12   SERCOM1.3 SERCOM3.3 // SCK
 * PA20  D6    SERCOM5.2 SERCOM3.2 // MOSI
 */

#define LEDS_MISO 13u /* unused */
#define LEDS_SCK  12u 
#define LEDS_MOSI 6u
SPIClass ledsSPI (&sercom3, LEDS_MISO, LEDS_SCK, LEDS_MOSI, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_1);

#include <I2S.h>

// Just duplicating what Arduino Zero is doing in case I ever stop using Arduino Zero bootloader
// TODO: test this with a different set of pins
#define MIC_CLK_GEN 3
#define MIC_SD  9u
#define MIC_SCK 1u
#define MIC_FS  0u
I2SClass micI2S (0, MIC_CLK_GEN, MIC_SD, MIC_SCK, MIC_FS);

// pin numbers from arduino zero variant.cpp
#if EVM_HARDWARE_VERSION > 1
#define BUTTON_PIN_1 2   // PA14  Use for patterns switcher
#define BUTTON_PIN_2 26  // PA27  Use for colors switcher

#define TOUCH_PIN_1 8    // PA06  Use for top-left spoke
#define TOUCH_PIN_2 17   // PA04  Use for bottom spoke
#define TOUCH_PIN_3 19   // PB02  Use for top-right spoke
#else
#define BUTTON_PIN_1 4   // PA08  Use for top-left spoke
#define BUTTON_PIN_2 39  // PA21  Use for bottom spoke
#define BUTTON_PIN_3 19  // PB02  Use for top-right spoke
#endif

static const uint8_t brightnessDialPort = PORTA;
static const uint8_t brightnessDialPin = 9;

/* --------------------------------- */

#define FASTLED_USE_PROGMEM 1
// #define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

#include "util.h"
#include "drawing.h"
#include "PatternManager.h"
#include "controls.h"
#include "power.h"
#include "ledgraph.h"

#define WAIT_FOR_SERIAL 1
#define UNCONNECTED_PIN_1 A1  // PB08
#define UNCONNECTED_PIN_2 A0  // PA02

EVMPixelBuffer pixelBuffer;

FrameCounter fc;
HardwareControls controls;
PatternManager patternManager;
PowerManager powerManager;

static bool serialTimeout = false;
static unsigned long setupDoneTime;

void DrawModal(int fps, unsigned long durationMillis, std::function<void(int frame)> tick) {
  int delayMillis = 1000/fps;
  unsigned long start = millis();
  unsigned long elapsed = 0;
  do {
    tick(elapsed);
    FastLED.show();
    FastLED.delay(delayMillis);
    elapsed = millis() - start;
  } while (elapsed < durationMillis);
}

void paletteAutorotateWelcome() {
  DrawModal(120, 100, [](unsigned long elapsed) {
    pixelBuffer.ctx.leds.fadeToBlackBy(15);
  });
  DrawModal(120, 1200, [](unsigned long elapsed) {
    pixelBuffer.ctx.leds.fadeToBlackBy(15);
    for (unsigned c = 0; c < circleleds.size(); ++c) {
      uint8_t fadeUp = min(0xFFu, 0xFF * elapsed / 250);
      uint8_t fadeDown = 0xFF - 0xFF * max(0, (long)elapsed - 1000) / 200;
      pixelBuffer.leds[circleleds[c]] = CHSV(0xFF * (c+elapsed/30) / circleleds.size(), 0xFF, scale8(fadeUp, fadeDown));
    }
  });
}

void setupButtons() {
#if EVM_HARDWARE_VERSION > 1
  SPSTButton *buttons[2];
  buttons[0] = controls.addButton(BUTTON_PIN_1);
  buttons[1] = controls.addButton(BUTTON_PIN_2);

  // Patterns Button
  buttons[0]->onSinglePress([]() {
    patternManager.nextPattern();
  });
  buttons[0]->onDoublePress([]() {
    patternManager.previousPattern();
  });
  buttons[0]->onLongPress([]() {
    patternManager.togglePatternAutoRotate();
  });

  // Colors Button
  buttons[1]->onSinglePress([]() {
    patternManager.nextPalette();
  });
  buttons[1]->onDoublePress([]() {
    patternManager.previousPalette();
  });
  buttons[1]->onLongPress([]() {
    patternManager.togglePaletteAutoRotate();
    if (!patternManager.colorManager->pauseRotation) {
      paletteAutorotateWelcome();
    }
  });
  
  // EVM Touch Pads
  TouchButton *touchPads[3];
  touchPads[0] = controls.addTouchButton(TOUCH_PIN_1);
  touchPads[1] = controls.addTouchButton(TOUCH_PIN_2);
  touchPads[2] = controls.addTouchButton(TOUCH_PIN_3);

  for (int b = 0; b < 3; ++b) {
    touchPads[b]->onLongPress([b]() {
      ChargePattern *charge = new ChargePattern();
      charge->spoke = b;
      patternManager.startPattern(charge);
    });
  }
#else
  SPSTButton *buttons[3];
  buttons[0] = controls.addButton(BUTTON_PIN_1);
  buttons[1] = controls.addButton(BUTTON_PIN_2);
  buttons[2] = controls.addButton(BUTTON_PIN_3);

  buttons[0]->onSinglePress([]() {
    patternManager.previousPattern();
  });
  buttons[1]->onSinglePress([]() {
    patternManager.nextPalette();
  });
  buttons[2]->onSinglePress([]() {
    patternManager.nextPattern();
  });

  for (int b = 0; b < 3; ++b) {
    buttons[b]->onLongPress([b]() {
      ChargePattern *charge = new ChargePattern();
      charge->spoke = b;
      patternManager.startPattern(charge);
    });
  }
#endif
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

  FastLED.addLeds<APA102, LEDS_MOSI, LEDS_SCK, BGR>(pixelBuffer.leds, pixelBuffer.leds.size());
  FastLED.setBrightness(0);

  fc.tick();

  setupButtons();
  controls.update();

  patternManager.setup();

  initLEDGraph();
  assert(ledgraph.adjList.size() == NUM_LEDS, "adjlist size should match num_leds");

  setupDoneTime = millis();
}

void serialTimeoutIndicator() {
  FastLED.setBrightness(50);
  pixelBuffer.leds.fill_solid(CRGB::Black);
  if ((millis() - setupDoneTime) % 250 < 100) {
    pixelBuffer.leds.fill_solid(CRGB::Red);
  }
  FastLED.show();
  delay(20);
}

void testPalette(CRGBPalette256 &palette) {
  for (unsigned i = 0; i < circleleds.size(); ++i) {
    pixelBuffer.leds[circleleds[i]] = ColorFromPalette(palette, 0xFF * i / circleleds.size());
  }
}

void loop() {
  if (serialTimeout && millis() - setupDoneTime < 1000) {
    serialTimeoutIndicator();
    return;
  }

  powerManager.loop(pixelBuffer);

  patternManager.loop(pixelBuffer);

    // unsigned long mils = millis();
    // int mod = mils % 30000;
    // CRGBPalette256 palette;
    // if (mod < 10000) {
    //   palette = Bi_Flag_gp;
    // } else if (mod < 20000) {
    //   palette = Lesbian_Flag_gp;
    // } else {
    //   palette = Pride_Flag_gp;
    // }
    // testPalette(palette);
  
  FastLED.show();

  fc.tick();
  fc.clampToFramerate(120);
  controls.update();
}
