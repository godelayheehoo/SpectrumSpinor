#pragma once
#include <Arduino.h>
#include "SystemConfig.h"
/**
 * Enum for efficiently identifying colors without string comparisons
 * Order matches the color database in ColorInfo.h
 */

// ColorCalibration redDefaultCal = ColorCalibration{36600, 11350, 14950};
// ColorCalibration greenDefaultCal = ColorCalibration{12650, 27100,25400};
// ColorCalibration purpleDefaultCal = ColorCalibration{18490, 18100, 32300};
// ColorCalibration blueDefaultCal = ColorCalibration{11800, 21000,38700};
// ColorCalibration orangeDefaultCal = ColorCalibration{33100,14400,13900};
// ColorCalibration yellowDefaultCal = ColorCalibration{24700, 21600, 13500};
// ColorCalibration silverDefaultCal = ColorCalibration{21320, 20500, 22290};
// ColorCalibration whiteDefaultCal = ColorCalibration{20800, 20820, 21900};
enum class Color : uint8_t {
    RED = 0,
    GREEN = 1,
    PURPLE = 2,
    BLUE = 3,
    ORANGE = 4,
    YELLOW = 5,
    SILVER = 6,
    WHITE = 7,      // Special color - ignore this
    UNKNOWN = 255   // Special value for unrecognized colors
};

/**
 * Get the total number of defined colors (excluding UNKNOWN)
 */
constexpr uint8_t getColorCount() {
    return NUM_COLORS;
}

/**
 * Convert Color enum to string name (for debugging/display)
 * @param color The color enum value
 * @return String name of the color
 */
const char* colorToString(Color color);

/**
 * Convert Color enum to array index
 * @param color The color enum value
 * @return Array index (0-9) or -1 for UNKNOWN
 */
inline int colorToIndex(Color color) {
    if (color == Color::UNKNOWN) {
        return -1;
    }
    return static_cast<int>(color);
}

/**
 * Convert array index to Color enum
 * @param index Array index (0-9)
 * @return Color enum value or UNKNOWN if index is invalid
 */
inline Color indexToColor(int index) {
    if (index < 0 || index >= getColorCount()) {
        return Color::UNKNOWN;
    }
    return static_cast<Color>(index);
}