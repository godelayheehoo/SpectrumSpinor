#include "ColorEnum.h"

// Color name lookup table - order must match enum values
static const char* colorNames[] = {
    "pink",     // Color::PINK = 0
    "orange",   // Color::ORANGE = 1  
    "blue",     // Color::BLUE = 2
    "yellow",   // Color::YELLOW = 3
    "green",    // Color::GREEN = 4
    "red",      // Color::RED = 5
    "purple",   // Color::PURPLE = 6
    "brown",    // Color::BROWN = 7
    "white",    // Color::WHITE = 8
    "black"     // Color::BLACK = 9
};

const char* colorToString(Color color) {
    int index = colorToIndex(color);
    if (index == -1) {
        return "unknown";
    }
    return colorNames[index];
}