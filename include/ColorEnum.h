#pragma once
#include <Arduino.h>
/**
 * Enum for efficiently identifying colors without string comparisons
 * Order matches the color database in ColorInfo.h
 */
enum class Color : uint8_t {
    PINK = 0,
    ORANGE = 1,
    BLUE = 2,
    YELLOW = 3,
    GREEN = 4,
    RED = 5,
    PURPLE = 6,
    BROWN = 7,
    WHITE = 8,      // Special color - turns off notes, doesn't generate MIDI
    UNKNOWN = 255   // Special value for unrecognized colors
};

/**
 * Get the total number of defined colors (excluding UNKNOWN)
 */
constexpr uint8_t getColorCount() {
    return 9;
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
 * @return Array index (0-7) or -1 for UNKNOWN
 */
inline int colorToIndex(Color color) {
    if (color == Color::UNKNOWN) {
        return -1;
    }
    return static_cast<int>(color);
}

/**
 * Convert array index to Color enum
 * @param index Array index (0-7)
 * @return Color enum value or UNKNOWN if index is invalid
 */
inline Color indexToColor(int index) {
    if (index < 0 || index >= getColorCount()) {
        return Color::UNKNOWN;
    }
    return static_cast<Color>(index);
}