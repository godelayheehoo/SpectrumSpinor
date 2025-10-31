#include "ColorHelper.h"
#include "ColorInfo.h"
#include "SystemConfig.h"

//todo: maybe do sample counts in the menu as well.

ColorHelper::ColorHelper(bool normalizeReadings) 
    : tcs(TCS34725_INTEGRATIONTIME_24MS, TCS34725_GAIN_4X), 
      normalize(normalizeReadings), 
      sensorAvailable(false) {
}

// Set or update the color database
void ColorHelper::setColorDatabase(ColorCenter* db, int numColors) {
    colorDatabase = db;
    numColorDatabase = numColors;
}

bool ColorHelper::begin() {
    // Don't re-initialize Wire - assume it's already been set up by main.cpp
    // The color sensor will use the same I2C bus as the OLED display
    
    if (tcs.begin()) {
        sensorAvailable = true;
        Serial.println("TCS34725 color sensor initialized successfully");
        return true;
    } else {
        sensorAvailable = false;
        Serial.println("ERROR: TCS34725 color sensor not found - check wiring!");
        Serial.println("Make sure sensor is connected to SDA=21, SCL=22");
        return false;
    }
}

bool ColorHelper::isAvailable() const {
    return sensorAvailable;
}

void ColorHelper::getRawData(uint16_t* r, uint16_t* g, uint16_t* b, uint16_t* c) {
    if (!sensorAvailable) {
        *r = *g = *b = *c = 0;
        return;
    }
    
    tcs.getRawData(r, g, b, c);
}

void ColorHelper::getNormalizedData(float* r, float* g, float* b) {
    uint16_t rawR, rawG, rawB, rawC;
    getRawData(&rawR, &rawG, &rawB, &rawC);
    
    if (normalize && rawC != 0) {
        // Normalize by clear channel to reduce brightness effects
        *r = rawR / (float)rawC;
        *g = rawG / (float)rawC;
        *b = rawB / (float)rawC;
    } else {
        // Convert to 0.0-1.0 range without clear channel normalization
        *r = rawR / 65535.0f;
        *g = rawG / 65535.0f;
        *b = rawB / 65535.0f;
    }
}

Color ColorHelper::getCurrentColorEnum() {
    if (!sensorAvailable) {
        return Color::UNKNOWN;
    }
    
    float r, g, b;
    // Serial.println("DEBUG:  about to call getNormalizedData [getCurrentColorEnum]");
    getNormalizedData(&r, &g, &b);
    // Serial.println("DEBUG: About to call findNearestColorEnum [getCurrentColorEnum]");
    
    return findNearestColorEnum(r, g, b);
}

const char* ColorHelper::getCurrentColor() {
    if (!sensorAvailable) {
        return "no sensor";
    }
    
    Color color = getCurrentColorEnum();
    if (color == Color::UNKNOWN) {
        return "unknown";
    }
    return colorToString(color);
}

void* ColorHelper::getColorDatabase(int& numColors) {
    numColors = numColorDatabase;
    return colorDatabase;
}

Color ColorHelper::findNearestColorEnum(float r, float g, float b) {

    // Serial.println("DEBUG: inside findNearestColorEnum");
    Color nearestColor = Color::UNKNOWN;
    float minDistance = 1e9f;
    // Serial.print("[DEBUG]: numColorDatabase is: ");
    // Serial.println(numColorDatabase);
    for (int i = 0; i < numColorDatabase; i++) {
        // Serial.print("[DEBUG]: checking #");
        // Serial.println(i);
        // Convert stored color values to normalized range
        float storedR = colorDatabase[i].avgR / 65535.0f;
        float storedG = colorDatabase[i].avgG / 65535.0f;
        float storedB = colorDatabase[i].avgB / 65535.0f;
        
        float distance = calculateColorDistance(r, g, b, storedR, storedG, storedB);
        
        if (distance < minDistance) {
            minDistance = distance;
            nearestColor = indexToColor(i);
        }
    }
    
    return nearestColor;
}

const char* ColorHelper::findNearestColor(float r, float g, float b) {
    Color color = findNearestColorEnum(r, g, b);
    return colorToString(color);
}

float ColorHelper::calculateColorDistance(float r1, float g1, float b1, 
                                        float r2, float g2, float b2) {
    float dr = r1 - r2;
    float dg = g1 - g2;
    float db = b1 - b2;
    
    // Return Euclidean distance squared (no need to sqrt for comparison)
    return dr*dr + dg*dg + db*db;
}

void ColorHelper::getSamplesAverage(uint16_t* avgR, uint16_t* avgG, uint16_t* avgB){
    uint16_t sumR, sumG, sumB;
    sumR = 0;
    sumG = 0;
    sumB = 0;
    delay(50);
    for(int i = 0; i < NUM_CALIBRATION_STEPS; i++){
        Serial.print("Sample # ");
        Serial.println(i);
        uint16_t r, g, b, c;
        getRawData(&r, &g, &b, &c);

        // I think maybe I shouldn't be normalizing here.

        if (this->normalize && c != 0) {  // avoid divide-by-zero
            sumR += (uint32_t)((float)r / c * 65535);
            sumG += (uint32_t)((float)g / c * 65535);
            sumB += (uint32_t)((float)b / c * 65535);
        } else {
            sumR += r;
            sumG += g;
            sumB += b;
        }
        delay(100);
    }
    // Compute averages
    *avgR = sumR / NUM_CALIBRATION_STEPS;
    *avgG = sumG / NUM_CALIBRATION_STEPS;
    *avgB = sumB / NUM_CALIBRATION_STEPS;
}

//white is separate from the colors because it might be treated differently soon.
void ColorHelper::calibrateWhite(){
 
  uint16_t avgR, avgG, avgB;
  getSamplesAverage(&avgR, &avgG, &avgB);

  Serial.println("Calibration complete!");
  Serial.print("Average R: "); Serial.println(avgR);
  Serial.print("Average G: "); Serial.println(avgG);
  Serial.print("Average B: "); Serial.println(avgB);

  byte whiteIndex = colorToIndex(Color::WHITE);
  this->colorDatabase[whiteIndex].avgR = avgR;
  this->colorDatabase[whiteIndex].avgG = avgG;
  this->colorDatabase[whiteIndex].avgB = avgB;
}

void ColorHelper::calibrateColor(Color color){
      uint16_t avgR, avgG, avgB;
  getSamplesAverage(&avgR, &avgG, &avgB);

  Serial.println("Calibration complete!");
  Serial.print("Average R: "); Serial.println(avgR);
  Serial.print("Average G: "); Serial.println(avgG);
  Serial.print("Average B: "); Serial.println(avgB);

  byte colorIndex = colorToIndex(color);
  this->colorDatabase[colorIndex].avgR = avgR;
  this->colorDatabase[colorIndex].avgG = avgG;
  this->colorDatabase[colorIndex].avgB = avgB;
}
    
