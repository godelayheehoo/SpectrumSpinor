#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "PinDefinitions.h"
#include "ColorEnum.h"
#include "ColorInfo.h"

// Default color calibrations (defined in one .cpp file)
extern ColorCalibration pinkDefaultCal;
extern ColorCalibration orangeDefaultCal;
extern ColorCalibration darkBlueDefaultCal;
extern ColorCalibration yellowDefaultCal;
extern ColorCalibration greenDefaultCal;
extern ColorCalibration redDefaultCal;
extern ColorCalibration lightBlueDefaultCal;
extern ColorCalibration blackDefaultCal;
extern ColorCalibration whiteDefaultCal;

extern ColorCalibration colorCalibrationDefaultDatabase[NUM_COLORS];

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