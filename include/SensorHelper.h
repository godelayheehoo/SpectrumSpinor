//none of this is used

#pragma once
#include <Arduino.h>
#include "ColorEnum.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "PinDefinitions.h"

struct ColorCalibration{
    uint red;
    uint green;
    uint blue;
};

class SensorHelper {
    public:
    SensorHelper(byte sensorIndexValue) : sensorIndex(sensorIndexValue) {};

    byte sensorIndex;

    //default values gotten from sensor A -- actually got all zeroes
    uint rDark = 0;
    uint gDark = 0;
    uint bDark = 0;

    uint rGain;
    uint gGain;
    uint bGain;

    ColorCalibration pinkCal;
    ColorCalibration orangeCal;
    ColorCalibration blueCal;
    ColorCalibration yellowCal;
    ColorCalibration greenCal;
    ColorCalibration redCal;
    ColorCalibration purpleCal;
    ColorCalibration brownCal;
    ColorCalibration whiteCal;
    ColorCalibration blackCal;

    void calibrateDark();

    void calibrateGains();

    void calibrateColor(Color color);

    Color calculateColor(uint rRaw, uint gRaw, uint bRaw, uint cRaw);

    Adafruit_TCS34725 tcs;

    bool sensorBegin();


    private:
    bool sensorAvailable;
    
};

