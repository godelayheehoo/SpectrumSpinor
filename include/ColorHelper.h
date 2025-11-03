#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "PinDefinitions.h"
#include "ColorEnum.h"
#include "ColorInfo.h"

struct ColorCalibration{
    uint red;
    uint green;
    uint blue;
    char name[12];
};

class ColorHelper {
public:
    ColorHelper(bool normalizeReadings = true);
    // Initialize the color sensor
    bool begin();
    // Get the currently detected color enum (EFFICIENT!)
    Color getCurrentColorEnum();
    // Get the currently detected color name (for backwards compatibility)
    const char* getCurrentColor();
    // Get raw color readings
    void getRawData(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* c);
    // Get normalized color readings (0.0 - 1.0)
    void getNormalizedData(float* r, float* g, float* b);

    // Get data using new calibration scheme
    void getCalibratedData(float* r, float* g, float* b);

    // Check if sensor is available
    bool isAvailable() const;

    // Set or update the color database (copy up to NUM_COLORS entries)
    void setColorDatabase(const ColorCalibration db[], int numColors);

    void getSamplesAverage(uint16_t* avgR, uint16_t* avgG, uint16_t* avgB);

    void calibrateWhite(); // Calibration function prototype
    
    void calibrateColor(Color color);

    // ColorCenter* colorDatabase = nullptr;
    ColorCalibration calibrationDatabase[NUM_COLORS];

  //default values gotten from sensor A -- actually got all zeroes
    uint rDark = 0;
    uint gDark = 0;
    uint bDark = 0;

    //default values, from sensor A
    uint rW = 24519;
    uint gW = 24150;
    uint bW = 14495;

    
    uint avgW = (rW + gW + bW) / 3.0;
    uint rGain = avgW / rW;
    uint gGain = avgW / gW;
    uint bGain = avgW / bW;

    ColorCalibration pinkCal = ColorCalibration{26588, 15769, 20923, "Pink"};
    ColorCalibration orangeCal = ColorCalibration{33937, 14220, 10201, "Orange"};
    ColorCalibration darkBlueCal = ColorCalibration{6808, 17916, 56515, "DarkBlue"};
    ColorCalibration yellowCal = ColorCalibration{27469, 20655, 9679, "Yellow"};
    ColorCalibration greenCal = ColorCalibration{9516, 32877, 23031, "Green"};
    ColorCalibration redCal = ColorCalibration{45099, 8233, 13700, "Red"};
    ColorCalibration lightBlueCal = ColorCalibration{11127, 18437, 46838, "LightBlue"};
    ColorCalibration blackCal = ColorCalibration{8374, 22792, 22171, "Black"};
    ColorCalibration whiteCal = ColorCalibration{21200, 21179, 22199, "White"}; //this is the background, not the white disk

    ColorCalibration colorCalibrationDefaultDatabase [NUM_COLORS] = {
    pinkCal,
    orangeCal,
    darkBlueCal,
    yellowCal,
    greenCal,
    redCal,
    blackCal,
    lightBlueCal,
    whiteCal
    };

    
private:
    Adafruit_TCS34725 tcs;
    bool normalize;
    bool sensorAvailable;

    // Internal color database access
    void* getColorDatabase(int& numColors);
    // Find nearest color match (returns enum - EFFICIENT!)
    Color findNearestColorEnum(float r, float g, float b);
    // Find nearest color match (returns string - for backwards compatibility)
    const char* findNearestColor(float r, float g, float b);
    // Calculate Euclidean distance between colors
    float calculateColorDistance(float r1, float g1, float b1,
                               float r2, float g2, float b2);

    // Add these members to hold the color database
   
    int numColorDatabase = 0;
};