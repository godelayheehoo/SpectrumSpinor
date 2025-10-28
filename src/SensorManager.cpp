#include "SensorManager.h"
#include "ColorInfo.h"

// Define the default colors (moved from header to avoid multiple definition)
ColorCenter defaultColors[9] = {
    {29661, 14660, 20216, "pink"},
    {38714, 7259, 17264, "orange"}, 
    {15596, 26298, 23352, "blue"},
    {28083, 26337, 8616, "yellow"},
    {15202, 35000, 13116, "green"},
    {47706, 11395, 9411, "red"},
    {26269, 19172, 20256, "purple"},
    {34588, 20049, 9354, "brown"},
    {24491, 24725, 13963, "white"}
};

// Initialize global calibration storage
SensorCalibration sensorCalibrations[4];

// Legacy support - points to sensor A's calibration
ColorCenter* colorDatabase = nullptr;
int numColorDatabase = 9;

SensorManager::SensorManager() {
    Serial.println("SensorManager constructor starting...");
    
    // Initialize arrays safely
    for (int i = 0; i < MAX_SENSORS; i++) {
        sensors[i] = nullptr;
        sensorEnabled[i] = false;
        currentColors[i] = Color::UNKNOWN;
        lastReadTimes[i] = 0;
    }
    
    Serial.println("Basic arrays initialized");
    
    // Initialize calibration data with defaults
    initializeCalibrationData();
    
    Serial.println("SensorManager initialized with 4 sensor slots");
}

SensorManager::~SensorManager() {
    // Clean up any allocated sensors
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (sensors[i] != nullptr) {
            delete sensors[i];
            sensors[i] = nullptr;
        }
    }
}

bool SensorManager::initializeSensor(int sensorIndex, uint8_t i2cAddress) {
    if (sensorIndex < 0 || sensorIndex >= MAX_SENSORS) {
        Serial.print("Invalid sensor index: ");
        Serial.println(sensorIndex);
        return false;
    }
    
    // Clean up existing sensor if any
    if (sensors[sensorIndex] != nullptr) {
        delete sensors[sensorIndex];
    }
    
    // Create new sensor
    sensors[sensorIndex] = new ColorHelper(true); // Enable normalization
    
    // Initialize the sensor (you might need to modify ColorHelper to accept I2C address)
    if (sensors[sensorIndex]->begin()) {
        sensorEnabled[sensorIndex] = true;
        Serial.print("Sensor ");
        Serial.print(sensorCalibrations[sensorIndex].sensorName);
        Serial.print(" initialized successfully at address 0x");
        Serial.println(i2cAddress, HEX);
        return true;
    } else {
        delete sensors[sensorIndex];
        sensors[sensorIndex] = nullptr;
        sensorEnabled[sensorIndex] = false;
        Serial.print("Failed to initialize sensor ");
        Serial.println(sensorCalibrations[sensorIndex].sensorName);
        return false;
    }
}

bool SensorManager::isSensorEnabled(int sensorIndex) const {
    if (sensorIndex < 0 || sensorIndex >= MAX_SENSORS) {
        return false;
    }
    return sensorEnabled[sensorIndex];
}

Color SensorManager::getCurrentColor(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= MAX_SENSORS || !sensorEnabled[sensorIndex]) {
        return Color::UNKNOWN;
    }
    
    if (sensors[sensorIndex] != nullptr && sensors[sensorIndex]->isAvailable()) {
        Color newColor = sensors[sensorIndex]->getCurrentColorEnum();
        currentColors[sensorIndex] = newColor;
        lastReadTimes[sensorIndex] = millis();
        return newColor;
    }
    
    return currentColors[sensorIndex]; // Return last known color
}

bool SensorManager::hasColorChanged(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= MAX_SENSORS || !sensorEnabled[sensorIndex]) {
        return false;
    }
    
    Color previousColor = currentColors[sensorIndex];
    Color newColor = getCurrentColor(sensorIndex);
    
    return (newColor != Color::UNKNOWN && newColor != previousColor);
}

void SensorManager::setColorDatabase(int sensorIndex, const SensorCalibration& calibration) {
    if (sensorIndex < 0 || sensorIndex >= MAX_SENSORS) {
        return;
    }
    
    sensorCalibrations[sensorIndex] = calibration;
    Serial.print("Updated calibration for sensor ");
    Serial.println(sensorCalibrations[sensorIndex].sensorName);
}

const SensorCalibration& SensorManager::getColorDatabase(int sensorIndex) const {
    if (sensorIndex < 0 || sensorIndex >= MAX_SENSORS) {
        return sensorCalibrations[0]; // Return sensor A as fallback
    }
    
    return sensorCalibrations[sensorIndex];
}

void SensorManager::updateAllSensors() {
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (sensorEnabled[i]) {
            getCurrentColor(i); // This updates the color and timestamp
        }
    }
}

int SensorManager::getNumEnabledSensors() const {
    int count = 0;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (sensorEnabled[i]) {
            count++;
        }
    }
    return count;
}

void SensorManager::printSensorStatus() const {
    Serial.println("=== Sensor Manager Status ===");
    for (int i = 0; i < MAX_SENSORS; i++) {
        Serial.print("Sensor ");
        Serial.print(sensorCalibrations[i].sensorName);
        Serial.print(": ");
        if (sensorEnabled[i]) {
            Serial.print("ENABLED - Current color: ");
            Serial.print(colorToString(currentColors[i]));
            Serial.print(" (last read: ");
            Serial.print(millis() - lastReadTimes[i]);
            Serial.println("ms ago)");
        } else {
            Serial.println("DISABLED");
        }
    }
    Serial.print("Total enabled sensors: ");
    Serial.println(getNumEnabledSensors());
}