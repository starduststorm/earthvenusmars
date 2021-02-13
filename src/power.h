#ifndef POWER_H
#define POWER_H

#include <Arduino.h>
#include <FastLED.h>
#include "wiring_private.h" // pinPeripheral() function
#include "util.h"
#include "ledgraph.h"

volatile int handleADC = 0;
volatile uint32_t adcRead = 0;
volatile bool sleepPending = 0;

void ADC_Handler() {
  // unset deep-sleep in case woken by interrupt
  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

  adcRead = ADC->RESULT.reg & 0xFFF; // lower 12 bits
  handleADC = 1;

  // start listening to all adc events again
  ADC->INTFLAG.bit.WINMON = 0x1;
  while (ADC->STATUS.bit.SYNCBUSY);

  // Switch to value<WINUT so we again get interrupts at value=0. See Note on WINMODE.
  ADC->WINCTRL.bit.WINMODE = ADC_WINCTRL_WINMODE_MODE2;
  while (ADC->STATUS.bit.SYNCBUSY);

  ADC->INTENSET.bit.WINMON = 1; // enable window interrupts

  sleepPending = 0;
}

uint32_t getADCRead() {
  return adcRead;
}

void brightnessDialChange(uint32_t value);

class PowerManager {
private:
  HardwareControls controls;
  bool sleeping = 0;
  
  void listen_for_adc_interrupt() {
    logf("Sleeping...");
    Serial.flush();

    // * wiring.c set all Pins as Inputs, changing it to INPUT_PULLUP may reduce sleep current
    // * Need to make sure i2s peripheral sleeps
    // * FIXME: is SERCOM3 sleeping or is RUNSTDBY not enough?
    // * What other peripherals are running by default and can be disabled?

    USBDevice.detach();
    
    // USB->CTRLB.bit.detach
    // I2S->CTRLA.bit.RUNSTDBY = 0;

    sleeping = 1;
    sleepPending = 0;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __WFI(); // wait-for-interrupt
  }

public:
  uint16_t wakeThreshold = 0.06 * 4096; // 12-bit ADC
  uint16_t sleepThreshold = 0.04 * 4096; // 12-bit ADC

  void wakeBlink(CRGBArray<NUM_LEDS> &leds) {
    const int windInFrames = 50;
    for (int i = 0; i < windInFrames; ++i) {
      leds.fill_solid(CRGB::Black);
      uint16_t progress = 0xFFFF * i / windInFrames;
      progress = ease16InOutQuad(progress);
      int maxlit_circle = lerp16by16(0, circleleds.size(), progress);
      for (int c = 0; c < maxlit_circle; ++c) {
        leds[circleleds[c]] = CRGB::Red;
      }
      if (handleADC) {
        // show live brightness
        int pot = adcRead;
        FastLED.setBrightness(0xFF * pot/4096.);
      }
      FastLED.show();
      FastLED.delay(16);
    }
  }

  void sleepBlink(CRGBArray<NUM_LEDS> &leds) {
    const int fadeUpFrames = 20;
    for (int i = 0; i < fadeUpFrames; ++i) {
      leds.fadeToBlackBy(0xFF / fadeUpFrames);
      for (int c : circleleds) {
        leds[c] = CHSV(0, 0xFF, i * 0xFF / fadeUpFrames);
      }
      FastLED.show();
      FastLED.delay(16);
      if (!sleepPending) {
        break;
      }
    }
    if (!sleepPending) {
      return;
    }

    const int windOutFrames = 50;
    for (int i = 0; i < windOutFrames; ++i) {
      leds.fill_solid(CRGB::Black);
      uint16_t progress = 0xFFFF * i / windOutFrames;
      progress = ease16InOutQuad(progress);
      int maxlit_circle = lerp16by16(circleleds.size(), 0, progress);
      for (int c = 0; c < maxlit_circle; ++c) {
        leds[circleleds[c]] = CHSV(0, 0xFF, lerp16by16(0xFF, 80, progress));
      }

      FastLED.show();
      FastLED.delay(16);
      if (!sleepPending) {
        break;
      }
    }
  }

  void setup_adc(int port, int pin) {
    logf("setup adc"); Serial.flush();
    pinPeripheral(pin, PIO_ANALOG);

    // Setup clock GCLK2 with unscaled OSCULP32K
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(0);
    while (GCLK->STATUS.bit.SYNCBUSY);

    // configure the generator of the generic clock
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_GENEN);

    // to output the OSCULP32K clock on PB16:
    // GCLK->GENCTRL.bit.OE = 1;
    // PORT->Group[g_APinDescription[11].ulPort].PINCFG[g_APinDescription[11].ulPin].bit.PMUXEN = 1;
    // PORT->Group[g_APinDescription[11].ulPort].PMUX[g_APinDescription[11].ulPin >> 1].reg |= PORT_PMUX_PMUXE_H;
    while (GCLK->STATUS.bit.SYNCBUSY);

    //confugure and enable the gen clock
    GCLK->CLKCTRL.reg = (uint32_t)(GCLK_CLKCTRL_ID_ADC /* is GCLK_ADC */ | GCLK_CLKCTRL_GEN_GCLK2 | GCLK_CLKCTRL_CLKEN);
    while (GCLK->STATUS.bit.SYNCBUSY);

    ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INTVCC1_Val; // AREF is VDDANA / 2
    
    ADC->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_12BIT_Val; // NOTE: can probably use 8-bit
    while (ADC->STATUS.bit.SYNCBUSY); 

    ADC->CTRLB.bit.PRESCALER = ADC_CTRLB_PRESCALER_DIV32_Val;
    while (ADC->STATUS.bit.SYNCBUSY);

    // set up brightness pin for adc
    PORT->Group[port].PINCFG[pin].bit.PMUXEN = 1;
    PORT->Group[port].PMUX[pin].reg |= PORT_PMUX_PMUXE_B;

    ADC->INPUTCTRL.bit.MUXPOS = 17; // PA09 is AIN[17]
    while (ADC->STATUS.bit.SYNCBUSY); 

    ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val; // Gain / 2 because REFSEL INTVCC1 uses VDDANA / 2

    ADC->CTRLB.bit.DIFFMODE = 0x0; // ADC single-ended mode
    while (ADC->STATUS.bit.SYNCBUSY);

    ADC->WINLT.bit.WINLT = wakeThreshold; // set WINLT to wake threshold so suppress adc updates while sleeping, once we enable the window mode later
    while (ADC->STATUS.bit.SYNCBUSY);

    ADC->WINUT.bit.WINUT = 4095;
    while (ADC->STATUS.bit.SYNCBUSY);

    // Note on WINMODE:
    // here we set the WINMODE to MODE2 to send interrupts for values below threshold WINUT. 
    // this is because we need to use MODE1 and WINLT to ignore interrupts during sleep, but WINLT's minimum value is 0
    // and will only send interrupts when the ADC value is strictly > 0. but when we flip the dial quickly to its lowest position,
    // the ADC converions sometimes skip from >sleepthresh directly to 0, so no interrupt is posted within the sleep thresh and the device stays awake.
    // Disabling WINMODE prevents window interrupts from being posted at all, so instead we'll use MODE2 as a dummy mode and flip to MODE1 prior to sleep.
    ADC->WINCTRL.bit.WINMODE = ADC_WINCTRL_WINMODE_MODE2;
    while (ADC->STATUS.bit.SYNCBUSY);

    ADC->CTRLB.bit.FREERUN = 0x1;
    while (ADC->STATUS.bit.SYNCBUSY); 

    ADC->CTRLA.bit.RUNSTDBY = 0x1; // continue processing ADC interrupts while GCLK is sleeping

    ADC->CTRLA.bit.ENABLE = 0x1;
    while (ADC->STATUS.bit.SYNCBUSY); 

    ADC->INTFLAG.bit.WINMON = 0x1;
    while (ADC->STATUS.bit.SYNCBUSY); 

    ADC->SWTRIG.bit.START = 1;
    while (ADC->STATUS.bit.SYNCBUSY); 
    
    while (!(ADC->INTFLAG.bit.RESRDY));
    ADC->INTFLAG.bit.RESRDY = 0x1;   // discard first reading

    ADC->INTENSET.bit.WINMON = 1; // enable window interrupts

    NVIC_EnableIRQ(ADC_IRQn);
    logf("adc setup done"); Serial.flush();

    // use the AnalogDial from Controls to tap into its value stabilizer
    AnalogDial *dial = controls.addAnalogDial(0);
    dial->readValueFunc = &getADCRead;
    dial->maxValue = 4096; // 12-bit
    dial->updateThreshold = 90;
    
    dial->onChange([this](uint32_t value) {
      this->brightnessUpdate(value);
    });
  }

  template<typename BufferType>
  void loop(BufferType &pixelBuffer) {
    if (sleeping) {
      FastLED.setBrightness(10);
      wakeBlink(pixelBuffer.leds);
      USBDevice.attach();
      logf("Just woke up");
      sleeping = 0;
    } else if (sleepPending) {
      pixelBuffer.leds.fill_solid(CRGB::Black);
      FastLED.setBrightness(10);
      sleepBlink(pixelBuffer.leds);
      if (sleepPending) { // if sleep hasn't been cancelled
        FastLED.setBrightness(0);
        FastLED.show();
        listen_for_adc_interrupt();
      }
    } else if (handleADC) {
      handleADC = 0;
      controls.update();
    }
  }

  void brightnessUpdate(uint32_t value) {
    if (value < sleepThreshold) {
      sleepPending = 1;
      ADC->WINCTRL.bit.WINMODE = ADC_WINCTRL_WINMODE_MODE1; // interrupt when RESULT > WINLT. See Note on WINMODE.
      while (ADC->STATUS.bit.SYNCBUSY);
    } else {
      uint8_t brightness = 0xFF * (value - sleepThreshold)/(4096. - sleepThreshold);
      if (FastLED.getBrightness() != brightness) {
        logf("Brightness -> %u", brightness);
        FastLED.setBrightness(brightness);
      }
    }
  }
};

#endif
