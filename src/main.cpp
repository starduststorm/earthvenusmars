#include <Arduino.h>
/* 

HARDWARE NOTES:

// FIXME: how do I need to wire the reset pin or other pins to more easily trigger a reset in case of on-launch software crash?

*/

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


#include <FastLED.h>

#include "util.h"

#define SERIAL_LOGGING 1
#define NUM_LEDS (48)
#define UNCONNECTED_PIN 14

CRGBArray<NUM_LEDS> leds;

// pin numbers from arduino zero variant.cpp
#define BUTTON_PIN_1 4   // PA08  Use for top-left spoke
#define BUTTON_PIN_2 39  // PA21  Use for bottom spoke
#define BUTTON_PIN_3 19  // PB02  Use for top-right spoke

static const uint8_t brightnessDialPort = PORTA;
static const uint8_t brightnessDialPin = 9;
static const uint16_t brightness_wake_thresh = 0.05 * 4096; // 12-bit ADC
static const uint16_t brightness_sleep_thresh = 0.03 * 4096; // 12-bit ADC

void setup_adc() {
  logf("setup adc"); Serial.flush();
  pinPeripheral(brightnessDialPin, PIO_ANALOG);

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

  logf("set up pin"); Serial.flush();

  // set up brightness pin for adc
  PORT->Group[brightnessDialPort].PINCFG[brightnessDialPin].bit.PMUXEN = 1;
  PORT->Group[brightnessDialPort].PMUX[brightnessDialPin].reg |= PORT_PMUX_PMUXE_B;

  ADC->INPUTCTRL.bit.MUXPOS = 17; // PA09 is AIN[17]
  while (ADC->STATUS.bit.SYNCBUSY); 

  logf("set up rest of adc"); Serial.flush();

  ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val; // Gain / 2 because REFSEL INTVCC1 uses VDDANA / 2

  ADC->CTRLB.bit.DIFFMODE = 0x0; // ADC single-ended mode
  while (ADC->STATUS.bit.SYNCBUSY);

  ADC->WINLT.bit.WINLT = 0;
  while (ADC->STATUS.bit.SYNCBUSY);
  
  ADC->WINCTRL.bit.WINMODE = 0x1; // interrupt when RESULT > WINLT
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
  ADC->INTFLAG.bit.RESRDY = 0x1;     // discard first reading

  ADC->INTENSET.bit.WINMON = 1; // enable window interrupts
  NVIC_EnableIRQ(ADC_IRQn);
  logf("adc setup done"); Serial.flush();
}

void wakeBlink() {
  for (int i = 0; i <= 0xFF; ++i) {
    analogWrite(A2, i);
    delay(2);
  }
  delay(100);
  analogWrite(A2, 0xFF);
  delay(50);
  analogWrite(A2, 0);
  delay(100);
  analogWrite(A2, 0xFF);
  delay(50);
  analogWrite(A2, 0);
  
}

void sleepBlink() {
  analogWrite(A2, 0xFF);
  delay(50);
  analogWrite(A2, 0);
  delay(100);
  analogWrite(A2, 0xFF);
  delay(50);
  analogWrite(A2, 0);
  delay(100);
  for (int i = 0xFF; i >=0; --i) {
    analogWrite(A2, i);
    delay(2);
  }
}

bool sleeping = 0;
void listen_for_adc_interrupt() {
  ADC->WINLT.bit.WINLT = brightness_wake_thresh;
  while (ADC->STATUS.bit.SYNCBUSY);

  FastLED.setBrightness(0);
  FastLED.show();
  sleepBlink();
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
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  __WFI(); // wait-for-interrupt
}

volatile int handleADC = 0;
volatile uint32_t adcRead = 0;
void ADC_Handler() {
  // unset deep-sleep in case woken by interrupt
  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

  adcRead = ADC->RESULT.reg & 0xFFF; // lower 12 bits
  handleADC = 1;

// start listening to all adc events again
  ADC->INTFLAG.bit.WINMON = 0x1;
  while (ADC->STATUS.bit.SYNCBUSY);
  ADC->WINLT.bit.WINLT = 0; 
  while (ADC->STATUS.bit.SYNCBUSY);

  ADC->INTENSET.bit.WINMON = 1; // enable window interrupts
}

void setup() {
  Serial.begin(57600);
  Serial.println("begin");
  Serial.flush();
  
  pinMode(A2, OUTPUT);
  analogWrite(A2, 0xFF);

  delay(5000);
  Serial.println("Done waiting at boot.");
  setup_adc();

  // use the alternate sercoms each of these SPI ports (SERCOM3)
  pinPeripheral(LEDS_MISO, PIO_SERCOM_ALT);
  pinPeripheral(LEDS_SCK,  PIO_SERCOM_ALT);
  pinPeripheral(LEDS_MOSI, PIO_SERCOM_ALT);

  // Make sure we're not running the SPI while in standby
  SERCOM3->SPI.CTRLA.bit.RUNSTDBY = 0;

  FastLED.addLeds<APA102, LEDS_MOSI, LEDS_SCK, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(70);
  
  // pinMode(BUTTON_PIN_1, INPUT_PULLUP);
  // pinMode(BUTTON_PIN_2, INPUT_PULLUP);
  // pinMode(BUTTON_PIN_3, INPUT_PULLUP);
  
}

int lead = 0;
void loop() {
  if (sleeping) {
    wakeBlink();
    USBDevice.attach();
    logf("Just woke up");
    sleeping = 0;
  }

  if (handleADC) {
    handleADC = 0;

    int pot = adcRead;
    FastLED.setBrightness(0xFF * pot/4096.);
    logf("adcRead: %i", pot);
    if (pot < brightness_sleep_thresh) {
      listen_for_adc_interrupt();
    }
  }

  leds.fill_rainbow(lead++);
  lead %= 0xFF;
  FastLED.show();
  delay(16);

  int b1 = digitalRead(BUTTON_PIN_1);
  int b2 = digitalRead(BUTTON_PIN_2);
  int b3 = digitalRead(BUTTON_PIN_3);
  
  unsigned long m = (millis() % 5000);
  analogWrite(A2, (m < 50 ? 0xFF : 0));
  
//
// #define UP_DOWN(button) ((button) ? "up" : "down")
  // logf("Buttons: button1: %s, button2: %s, button3: %s", UP_DOWN(b1), UP_DOWN(b2), UP_DOWN(b3));

}
