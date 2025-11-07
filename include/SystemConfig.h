//MIDI
#define MIDI_BAUD_RATE 31250

// Display Configuration - SH1106 OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C

#define MARGIN_TOP 2
#define MARGIN_LEFT 2
#define MARGIN_BOTTOM 2
#define USABLE_WIDTH (SCREEN_WIDTH - 2 * MARGIN_LEFT)
#define USABLE_HEIGHT (SCREEN_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM)
#define CENTER_X(width) (MARGIN_LEFT + (USABLE_WIDTH - width) / 2)
#define CENTER_Y(height) (MARGIN_TOP + (USABLE_HEIGHT - height) / 2)x
#define BOTTOM_LINE (SCREEN_HEIGHT - MARGIN_BOTTOM)
#define LARGE_TEXT_SIZE 3
#define SAFE_VISIBLE_OPTIONS 5

// Display Colors (monochrome)
#define OLED_WHITE SH110X_WHITE
#define OLED_BLACK SH110X_BLACK

// Legacy TFT colors (commented out for reference)
// #define TFT_WHITE, TFT_BLACK, TFT_RED, etc.

#define NUM_CALIBRATION_STEPS 20
#define NUM_COLORS 10

// I2C addresses
#define TCA_ADDR 0x70