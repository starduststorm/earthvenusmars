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

#define LEDS_MISO 13 /* unused */
#define LEDS_SCK  12 
#define LEDS_MOSI 6
SPIClass mySPI (&sercom3, LEDS_MISO, LEDS_SCK, LEDS_MOSI, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_1);

#include <FastLED.h>

#define SERIAL_LOGGING 1
#define NUM_LEDS (48)
#define UNCONNECTED_PIN 14

CRGBArray<NUM_LEDS> leds;

void setup() {

  pinPeripheral(LEDS_MISO, PIO_SERCOM_ALT);
  pinPeripheral(LEDS_SCK,  PIO_SERCOM_ALT);
  pinPeripheral(LEDS_MOSI, PIO_SERCOM_ALT);

  Serial.begin(57600);
  Serial.println("begin");
  delay(200);

//  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
  FastLED.addLeds<APA102, LEDS_MOSI, LEDS_SCK, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(70);
}

int lead = 0;
void loop() {
  Serial.print("Lead: ");
  Serial.println(lead);
  
  leds.fill_rainbow(lead++);
  lead %= 0xFF;
  FastLED.show();
  delay(16); 
}