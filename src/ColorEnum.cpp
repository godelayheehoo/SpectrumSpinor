#include "ColorEnum.h"

// Color name lookup table - order must match enum values

//  LIGHT_BLUE  = 0,
    // ORANGE = 1,
    // PINK = 2,
    // YELLOW = 3,
    // GREEN = 4,
    // RED = 5,
    // BLACK = 6,
    // DARK_BLUE = 7,
    // PURPLE = 8,
    // WHITE = 9,      // Special color - ignore this
    // UNKNOWN = 255  
static const char* colorNames[] = {
   "LightBlue",     // Color::LIGHT_BLUE = 0
    "Orange",   // Color::ORANGE = 1
    "Pink",     // Color::PINK = 2
    "Yellow",   // Color::YELLOW = 3
    "Green",    // Color::GREEN = 4
    "Red",      // Color::RED = 5
    "Black",    // Color::BLACK = 6
    "DarkBlue",// Color::DARK_BLUE = 7
    "Purple",   // Color::PURPLE = 8
    "White"     // Color::WHITE = 9
};

const char* colorToString(Color color) {
    int index = colorToIndex(color);
    if (index == -1) {
        return "unknown";
    }
    return colorNames[index];
}