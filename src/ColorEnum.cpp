#include "ColorEnum.h"

// Color name lookup table - order must match enum values

// enum class Color : uint8_t {
//     RED = 0,
//     GREEN = 1,
//     PURPLE = 2,
//     BLUE = 3,
//     ORANGE = 4,
//     YELLOW = 5,
//     SILVER = 6,
//     WHITE = 7,      // Special color - ignore this
//     UNKNOWN = 255   // Special value for unrecognized colors
// };
static const char* colorNames[] = {
   "Red",
   "Green",
   "Purple",
    "Blue",
    "Orange",
    "Yellow",
    "Silver",
    "White"
};

const char* colorToString(Color color) {
    int index = colorToIndex(color);
    if (index == -1) {
        return "unknown";
    }
    return colorNames[index];
}