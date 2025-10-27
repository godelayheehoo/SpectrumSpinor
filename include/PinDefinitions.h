#pragma once

// Display pins - SH1106 I2C OLED
#define OLED_SDA   21   // I2C Data
#define OLED_SCL   22   // I2C Clock
#define OLED_RESET -1   // Reset pin not used

// Legacy TFT pins (commented out for reference)
// #define TFT_CS     -1   // CS tied to GND, so not controlled by ESP32
// #define TFT_RST    -1   // Not connected (use -1 to disable)
// #define TFT_DC     2    // DC pin
// #define TFT_SCLK   18   // SCL = SPI Clock
// #define TFT_MOSI   23   // SDA = SPI Data (MOSI)

// Encoder pins
#define ENCODER_A  32
#define ENCODER_B  33
#define ENCODER_BTN 25
#define AUX_BTN    26

// Panic button
#define PANIC_BTN  4

// Color sensor pins (TCS34725)
#define COLOR_SENSOR_SDA 15
#define COLOR_SENSOR_SCL 27

// MIDI pins
#define MIDI_OUT_PIN 17
#define MIDI_IN_PIN -1 // Not used
