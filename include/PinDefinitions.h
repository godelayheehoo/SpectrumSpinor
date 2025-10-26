#pragma once

// Display pins
// Note: SCL=18 (SCLK), SDA=23 (MOSI), CS tied to GND
#define TFT_CS     -1   // CS tied to GND, so not controlled by ESP32
#define TFT_RST    -1   // Not connected (use -1 to disable)
#define TFT_DC     2    // DC pin
#define TFT_SCLK   18   // SCL = SPI Clock
#define TFT_MOSI   23   // SDA = SPI Data (MOSI)

// Encoder pins
#define ENCODER_A  32
#define ENCODER_B  33
#define ENCODER_BTN 25
#define AUX_BTN    26

// Color sensor pins (TCS34725)
#define COLOR_SENSOR_SDA 15
#define COLOR_SENSOR_SCL 27