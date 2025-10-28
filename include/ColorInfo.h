#pragma once
#include <Arduino.h>

struct ColorCenter{
  uint16_t avgR;
  uint16_t avgG;
  uint16_t avgB;
  const char* name;
};

// Per-sensor calibration data structure
struct SensorCalibration {
    ColorCenter colorDatabase[9]; // Each sensor gets its own calibration for 9 colors
    int numColors;
    bool isCalibrated;
    const char* sensorName; // "A", "B", "C", "D"
};

// Default color definitions (used as template for new sensor calibrations)
extern ColorCenter defaultColors[9];

// Global calibration storage for all four sensors
extern SensorCalibration sensorCalibrations[4];

// Legacy support - points to sensor A's calibration
extern ColorCenter* colorDatabase;
extern int numColorDatabase;
