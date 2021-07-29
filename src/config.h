#ifndef CONFIG_H
#define CONFIG_H

/*
 * SAMD21 pins
 * PA17  D13   SERCOM1.1 SERCOM3.1 // MISO
 * PA19  D12   SERCOM1.3 SERCOM3.3 // SCK
 * PA20  D6    SERCOM5.2 SERCOM3.2 // MOSI
 */
#define LEDS_MISO 13u /* unused */
#define LEDS_SCK  12u 
#define LEDS_MOSI 6u

// Just duplicating what Arduino Zero is doing in case I ever stop using Arduino Zero bootloader
// TODO: test this with a different set of pins
#define MIC_CLK_GEN 3
#define MIC_SD  9u
#define MIC_SCK 1u
#define MIC_FS  0u

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

#define UNCONNECTED_PIN_1 A1  // PB08
#define UNCONNECTED_PIN_2 A0  // PA02

#endif
