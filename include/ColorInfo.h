#pragma once
#include <Arduino.h>

struct ColorCenter{
  uint16_t avgR;
  uint16_t avgG;
  uint16_t avgB;
  const char* name;
};

// Color definitions
ColorCenter pink = {29661, 14660, 20216, "pink"};
ColorCenter orange = {38714, 7259, 17264, "orange"};
ColorCenter blue = {15596, 26298, 23352, "blue"};
ColorCenter yellow = {28083, 26337, 8616, "yellow"};
ColorCenter green = {15202, 35000, 13116, "green"};
ColorCenter red = {47706, 11395, 9411, "red"};
ColorCenter purple = {26269, 19172, 20256, "purple"};
ColorCenter brown = {34588, 20049, 9354, "brown"};
ColorCenter white = {24491, 24725, 13963, "white"};

// Static color database
static ColorCenter colorDatabase[] = {
    pink, orange, blue, yellow, green, red, purple, brown, white
};
static const int numColorDatabase = sizeof(colorDatabase) / sizeof(colorDatabase[0]);
