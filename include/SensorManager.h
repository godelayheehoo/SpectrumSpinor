#pragma once
#include <Arduino.h>
#include "ColorHelper.h"
#include "ColorEnum.h"
#include "ColorInfo.h"

class SensorManager {
private:
    static const int MAX_SENSORS = 4;
    ColorHelper* sensors[MAX_SENSORS];
    bool sensorEnabled[MAX_SENSORS];
    Color currentColors[MAX_SENSORS];
    unsigned long lastReadTimes[MAX_SENSORS];
    
public:
    SensorManager();
    ~SensorManager();
    
    // Sensor initialization
    bool initializeSensor(int sensorIndex, uint8_t i2cAddress = 0x29);
    bool isSensorEnabled(int sensorIndex) const;
    
    // Color detection
    Color getCurrentColor(int sensorIndex);
    bool hasColorChanged(int sensorIndex);
    
    // Calibration management
    void setColorDatabase(int sensorIndex, const SensorCalibration& calibration);
    const SensorCalibration& getColorDatabase(int sensorIndex) const;
    
    // Utility
    void updateAllSensors();
    int getNumEnabledSensors() const;
    
    // Debug
    void printSensorStatus() const;
    
    // Sensor indices for readability
    static const int SENSOR_A = 0;
    static const int SENSOR_B = 1;
    static const int SENSOR_C = 2;
    static const int SENSOR_D = 3;

private:
    // Helper function for safe initialization
    void initializeCalibrationData();
};