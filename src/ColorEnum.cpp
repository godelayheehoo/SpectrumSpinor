#include "ColorEnum.h"

// Color name lookup table - order must match enum values
static const char* colorNames[] = {
   "Pink",     // Color::PINK = 0
    "Orange",   // Color::ORANGE = 1  
    "DarkBlue",     // Color::DARK_BLUE = 2
    "Yellow",   // Color::YELLOW = 3
    "Green",    // Color::GREEN = 4
    "Red",      // Color::RED = 5
    "Black",    // Color::BLACK = 6
    "LightBlue",// Color::LIGHT_BLUE = 7
    "White"     // Color::WHITE = 8
};

const char* colorToString(Color color) {
    int index = colorToIndex(color);
    if (index == -1) {
        return "unknown";
    }
    return colorNames[index];
}