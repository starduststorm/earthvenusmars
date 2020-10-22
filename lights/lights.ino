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

  FastLED.addLeds<APA102, LEDS_MOSI, LEDS_SCK, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(70);

  pinMode(13 /*PA08*/, INPUT_PULLUP); // Use for top-left spoke
  pinMode(30 /*PA21*/, INPUT_PULLUP); // Use for bottom spoke
  pinMode(47 /*PB02*/, INPUT_PULLUP); // Use for top-right spoke
}

int lead = 0;
void loop() {
  leds.fill_rainbow(lead++);
  lead %= 0xFF;
  FastLED.show();
  delay(16);

  int b1 = digitalRead(13);
  int b2 = digitalRead(30);
  int b3 = digitalRead(47);

  Serial.print("buttons: ");
  Serial.print(b1);
  Serial.print(", ");
  Serial.print(b3);
  Serial.print(", ");
  Serial.print(b2);
  Serial.println();
}
