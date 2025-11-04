#include "ColorHelper.h"
#include "ColorInfo.h"
#include "SystemConfig.h"
#include <EEPROM.h>
#include "EEPROMAddresses.h"
#include "MenuManager.h"

// Default color calibration definitions (define once in this translation unit)
ColorCalibration pinkDefaultCal = ColorCalibration{26588, 15769, 20923};
ColorCalibration orangeDefaultCal = ColorCalibration{33937, 14220, 10201};
ColorCalibration darkBlueDefaultCal = ColorCalibration{6808, 17916, 56515};
ColorCalibration yellowDefaultCal = ColorCalibration{27469, 20655, 9679};
ColorCalibration greenDefaultCal = ColorCalibration{9516, 32877, 23031};
ColorCalibration redDefaultCal = ColorCalibration{45099, 8233, 13700};
ColorCalibration lightBlueDefaultCal = ColorCalibration{11127, 18437, 46838};
ColorCalibration blackDefaultCal = ColorCalibration{8374, 22792, 22171};
ColorCalibration whiteDefaultCal = ColorCalibration{21200, 21179, 22199}; //this is the background, not the white disk
ColorCalibration purpleDefaultCal = ColorCalibration{21200, 21179, 22199}; // need to update this

ColorCalibration colorCalibrationDefaultDatabase[NUM_COLORS] = {
    pinkDefaultCal,
    orangeDefaultCal,
    darkBlueDefaultCal,
    yellowDefaultCal,
    greenDefaultCal,
    redDefaultCal,
    blackDefaultCal,
    lightBlueDefaultCal,
    whiteDefaultCal,
    purpleDefaultCal
};

//todo: maybe do sample counts in the menu as well.

ColorHelper::ColorHelper(bool normalizeReadings, MenuManager* menuPtr) 
    : tcs(TCS34725_INTEGRATIONTIME_24MS, TCS34725_GAIN_4X), 
      normalize(normalizeReadings), 
      sensorAvailable(false),
      menu(menuPtr) {
}

// Set or update the color database by copying entries into the internal array
// Copies at most NUM_COLORS entries from the provided db array.
void ColorHelper::setColorDatabase(const ColorCalibration db[], int numColors) {
    int toCopy = numColors;
    if (toCopy > NUM_COLORS) toCopy = NUM_COLORS;
    for (int i = 0; i < toCopy; ++i) {
        calibrationDatabase[i] = db[i];
    }
    // If fewer entries provided than NUM_COLORS, zero out remaining entries
    for (int i = toCopy; i < NUM_COLORS; ++i) {
        calibrationDatabase[i].red = 0;
        calibrationDatabase[i].green = 0;
        calibrationDatabase[i].blue = 0;
    }
    numColorDatabase = toCopy;
}

void ColorHelper::setMenu(MenuManager* menuPtr) {
    menu = menuPtr;
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

// void ColorHelper::getNormalizedData(float* r, float* g, float* b) {
//     uint16_t rawR, rawG, rawB, rawC;
//     getRawData(&rawR, &rawG, &rawB, &rawC);
    
//     if (normalize && rawC != 0) {
//         // Normalize by clear channel to reduce brightness effects
//         *r = rawR / (float)rawC;
//         *g = rawG / (float)rawC;
//         *b = rawB / (float)rawC;
//     } else {
//         // Convert to 0.0-1.0 range without clear channel normalization
//         *r = rawR / 65535.0f;
//         *g = rawG / 65535.0f;
//         *b = rawB / 65535.0f;
//     }
// }

void ColorHelper::getCalibratedData(float* r, float* g, float* b) {
    uint16_t rawR, rawG, rawB, rawC;
    getRawData(&rawR, &rawG, &rawB, &rawC);
    // Serial.print("Raw R: ");
    // Serial.println(rawR);
    // Serial.print("rDark");
    // Serial.println(rDark);
   uint32_t rAdj = max(0, (int)rawR-(int)rDark);
   uint32_t bAdj = max(0, (int)rawB-(int)bDark);
   uint32_t gAdj = max(0, (int)rawG-(int)gDark);

//    Serial.print("Adj1: ");
//    Serial.println(rAdj);

//    Serial.print("rGain: ");
//    Serial.println(rGain);
  rAdj = (uint32_t)(rAdj * rGain);
  gAdj = (uint32_t)(gAdj * gGain);
  bAdj = (uint32_t)(bAdj * bGain);

//   Serial.print("rAdj2: ");
//   Serial.println(rAdj);

  if (normalize && rawC != 0) {
    rAdj = (uint32_t)((float)rAdj / rawC * 65535);
    // Serial.print("rAdj3: ");
    // Serial.println(rAdj);
    gAdj = (uint32_t)((float)gAdj / rawC * 65535);
    bAdj = (uint32_t)((float)bAdj / rawC * 65535);
}

*r = rAdj;
*g = gAdj;
*b = bAdj;
}

Color ColorHelper::getCurrentColorEnum() {
    if (!sensorAvailable) {
        return Color::UNKNOWN;
    }
    
    float r, g, b;
    // Serial.println("DEBUG:  about to call getNormalizedData [getCurrentColorEnum]");
    getCalibratedData(&r, &g, &b);
    // Serial.println("DEBUG: About to call findNearestColorEnum [getCurrentColorEnum]");
    // Serial.print("calibrated R: ");
    // Serial.println(r);
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
    return calibrationDatabase;
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
    float storedR = calibrationDatabase[i].red; /// 65535.0f;
    float storedG = calibrationDatabase[i].green;// / 65535.0f;
    float storedB = calibrationDatabase[i].blue;// / 65535.0f;

    // Serial.print("comparing r: ");
    // Serial.print(r);
    // Serial.print(" and storedR: ");
    // Serial.println(storedR);

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
    float sumR, sumG, sumB;
    sumR = 0;
    sumG = 0;
    sumB = 0;
    delay(50);
    for(int i = 0; i < NUM_CALIBRATION_STEPS; i++){
        Serial.print("Sample # ");
        Serial.println(i);
        float r, g, b;
        getCalibratedData(&r, &g, &b);

        // I think maybe I shouldn't be normalizing here-- I already normalize when I get calibrated data.

        sumR += r;
        sumG += g;
        sumB += b;
        // 
        delay(100);
        Serial.print("sum R:");
        Serial.println(sumR);
    }
    // Compute averages
    *avgR = (uint16_t)(sumR / NUM_CALIBRATION_STEPS);
    *avgG = (uint16_t)(sumG / NUM_CALIBRATION_STEPS);
    *avgB = (uint16_t)(sumB / NUM_CALIBRATION_STEPS);
}

void ColorHelper::calibrateDark(){
    Serial.println("Not currently working, need to install LED off pins");
    return;
    /*
    rDark = 0;
    gDark = 0;
    bDark = 0;

    // save to EEPROM
    switch(SensorNum){
        case 0:
            EEPROM.put(SENSOR_A_RDARK_ADDR, rDark);
            EEPROM.put(SENSOR_A_GDARK_ADDR, gDark);
            EEPROM.put(SENSOR_A_BDARK_ADDR, bDark);
            break;
        case 1:
            EEPROM.put(SENSOR_B_RDARK_ADDR, rDark);
            EEPROM.put(SENSOR_B_GDARK_ADDR, gDark);
            EEPROM.put(SENSOR_B_BDARK_ADDR, bDark);
            break;
        case 2:
            EEPROM.put(SENSOR_C_RDARK_ADDR, rDark);
            EEPROM.put(SENSOR_C_GDARK_ADDR, gDark);
            EEPROM.put(SENSOR_C_BDARK_ADDR, bDark);
            break;
        case 3:
            EEPROM.put(SENSOR_D_RDARK_ADDR, rDark);
            EEPROM.put(SENSOR_D_GDARK_ADDR, gDark);
            EEPROM.put(SENSOR_D_BDARK_ADDR, bDark);
            break;
        default:
            Serial.println("ERROR: Invalid sensor number for dark calibration");
    }
    EEPROM.commit();
    */
}

//white is separate from the colors because it might be treated differently soon.
void ColorHelper::calibrateWhiteGains(){

    //todo: menu should tell you what to do and that this should be done AFTER dark offset
 
    Serial.println("WARNING: NEEDS TO BE UPDATED");

    Serial.print("Pre Cal Wvals: ");
    Serial.print(rW);
    Serial.print(", ");
    Serial.print(gW);
    Serial.print(", ");
    Serial.println(bW);
  uint16_t avgRw, avgGw, avgBw;
  uint16_t sumRw, sumGw, sumBw;
    sumRw = 0;
    sumGw = 0;
    sumBw = 0;
    delay(50);
    menu->calibrationStartProgressBar();
    for(int i = 0; i < NUM_CALIBRATION_STEPS; i++){
        Serial.print("Sample # ");
        Serial.println(i);
        uint16_t r, g, b, c;
        getRawData(&r, &g, &b, &c);

        // I think maybe I shouldn't be normalizing here.

        if (this->normalize && c != 0) {  // avoid divide-by-zero
            sumRw += (uint32_t)((float)r / c * 65535);
            sumGw += (uint32_t)((float)g / c * 65535);
            sumBw += (uint32_t)((float)b / c * 65535);
        } else {
            sumRw += r;
            sumGw += g;
            sumBw += b;
        }
        menu->calibrationIncrementProgressBar(i);
        delay(100);
    }
    // Compute averages
    rW = sumRw / NUM_CALIBRATION_STEPS;
    gW = sumGw / NUM_CALIBRATION_STEPS;
    bW = sumBw / NUM_CALIBRATION_STEPS;

    Serial.print("Post Cal wVals: ");
    Serial.print(rW);
    Serial.print(", ");
    Serial.print(gW);
    Serial.print(", ");
    Serial.println(bW);


    float avg = (rW + gW + bW) / 3.0f;
    rGain = avg / rW;
    gGain = avg / gW;
    bGain = avg / bW;

    //save W values
    switch(SensorNum){
        case 0:
            EEPROM.put(SENSOR_A_RW_ADDR, rW);
            EEPROM.put(SENSOR_A_GW_ADDR, gW);
            EEPROM.put(SENSOR_A_BW_ADDR, bW);
            break;
        case 1:
            EEPROM.put(SENSOR_B_RW_ADDR, rW);
            EEPROM.put(SENSOR_B_GW_ADDR, gW);
            EEPROM.put(SENSOR_B_BW_ADDR, bW);
            break;
        case 2:
            EEPROM.put(SENSOR_C_RW_ADDR, rW);
            EEPROM.put(SENSOR_C_GW_ADDR, gW);
            EEPROM.put(SENSOR_C_BW_ADDR, bW);
            break;
        case 3:
            EEPROM.put(SENSOR_D_RW_ADDR, rW);
            EEPROM.put(SENSOR_D_GW_ADDR, gW);
            EEPROM.put(SENSOR_D_BW_ADDR, bW);
            break;
        default:
            Serial.println("ERROR: Invalid sensor number for white calibration");
    }
    EEPROM.commit();
    Serial.println("White calibration complete!");
}

void ColorHelper::calibrateColor(Color color){

    byte colorIndex = colorToIndex(color);
  
    Serial.print("Initial vals: r: ");
    Serial.print(this->calibrationDatabase[colorIndex].red);
    Serial.print(", g: ");
    Serial.print(this->calibrationDatabase[colorIndex].green);
    Serial.print(", b: ");
    Serial.println(this->calibrationDatabase[colorIndex].blue);
    Serial.println("Starting color calibration...");
  uint16_t avgR, avgG, avgB;
  getSamplesAverage(&avgR, &avgG, &avgB);

  Serial.println("Calibration complete!");
  Serial.print("Average R: "); Serial.println(avgR);
  Serial.print("Average G: "); Serial.println(avgG);
  Serial.print("Average B: "); Serial.println(avgB);

  
  ColorCalibration newCal{avgR, avgG, avgB};
  this->calibrationDatabase[colorIndex] = newCal;

    Serial.print("new vals: r: ");
    Serial.print(this->calibrationDatabase[colorIndex].red);
    Serial.print(", g: ");
    Serial.print(this->calibrationDatabase[colorIndex].green);
    Serial.print(", b: ");
    Serial.println(this->calibrationDatabase[colorIndex].blue);

//save result

// #define SENSOR_A_LIGHT_BLUE_CAL_ADDR 98
// #define SENSOR_A_ORANGE_CAL_ADDR 110
// #define SENSOR_A_PINK_CAL_ADDR 122
// #define SENSOR_A_YELLOW_CAL_ADDR 134
// #define SENSOR_A_GREEN_CAL_ADDR 146
// #define SENSOR_A_RED_CAL_ADDR 158
// #define SENSOR_A_BLACK_CAL_ADDR 170
// #define SENSOR_A_DARK_BLUE_CAL_ADDR 182
// #define SENSOR_A_PURPLE_CAL_ADDR 194
#define SENSOR_A_WHITE_CAL_ADDR 206
uint lightBlueAddr, orangeAddr, pinkAddr, yellowAddr, greenAddr, redAddr, blackAddr, darkBlueAddr, purpleAddr;
switch(SensorNum){
    case 0:
        lightBlueAddr = SENSOR_A_LIGHT_BLUE_CAL_ADDR;
        orangeAddr = SENSOR_A_ORANGE_CAL_ADDR;
        pinkAddr = SENSOR_A_PINK_CAL_ADDR;
        yellowAddr = SENSOR_A_YELLOW_CAL_ADDR;
        greenAddr = SENSOR_A_GREEN_CAL_ADDR;
        redAddr = SENSOR_A_RED_CAL_ADDR;
        blackAddr = SENSOR_A_BLACK_CAL_ADDR;
        darkBlueAddr = SENSOR_A_DARK_BLUE_CAL_ADDR;
        purpleAddr = SENSOR_A_PURPLE_CAL_ADDR;
        break;
    case 1:
        lightBlueAddr = SENSOR_B_LIGHT_BLUE_CAL_ADDR;
        orangeAddr = SENSOR_B_ORANGE_CAL_ADDR;
        pinkAddr = SENSOR_B_PINK_CAL_ADDR;
        yellowAddr = SENSOR_B_YELLOW_CAL_ADDR;
        greenAddr = SENSOR_B_GREEN_CAL_ADDR;
        redAddr = SENSOR_B_RED_CAL_ADDR;
        blackAddr = SENSOR_B_BLACK_CAL_ADDR;
        darkBlueAddr = SENSOR_B_DARK_BLUE_CAL_ADDR;
        purpleAddr = SENSOR_B_PURPLE_CAL_ADDR;
        break;
    case 2:
        lightBlueAddr = SENSOR_C_LIGHT_BLUE_CAL_ADDR;
        orangeAddr = SENSOR_C_ORANGE_CAL_ADDR;
        pinkAddr = SENSOR_C_PINK_CAL_ADDR;
        yellowAddr = SENSOR_C_YELLOW_CAL_ADDR;
        greenAddr = SENSOR_C_GREEN_CAL_ADDR;
        redAddr = SENSOR_C_RED_CAL_ADDR;
        blackAddr = SENSOR_C_BLACK_CAL_ADDR;
        darkBlueAddr = SENSOR_C_DARK_BLUE_CAL_ADDR;
        purpleAddr = SENSOR_C_PURPLE_CAL_ADDR;
        break;
    case 3:
        lightBlueAddr = SENSOR_D_LIGHT_BLUE_CAL_ADDR;
        orangeAddr = SENSOR_D_ORANGE_CAL_ADDR;
        pinkAddr = SENSOR_D_PINK_CAL_ADDR;
        yellowAddr = SENSOR_D_YELLOW_CAL_ADDR;
        greenAddr = SENSOR_D_GREEN_CAL_ADDR;
        redAddr = SENSOR_D_RED_CAL_ADDR;
        blackAddr = SENSOR_D_BLACK_CAL_ADDR;
        darkBlueAddr = SENSOR_D_DARK_BLUE_CAL_ADDR;
        purpleAddr = SENSOR_D_PURPLE_CAL_ADDR;
        break;
    default:
        Serial.println("ERROR: Invalid sensor number for color calibration");
        return;
}
    switch(color){
        case Color::LIGHT_BLUE:
            EEPROM.put(lightBlueAddr, newCal);
            break;
        case Color::ORANGE:
            EEPROM.put(orangeAddr, newCal);
            break;
        case Color::PINK:
            EEPROM.put(pinkAddr, newCal);
            break;
        case Color::YELLOW:
            EEPROM.put(yellowAddr, newCal);
            break;
        case Color::GREEN:
            EEPROM.put(greenAddr, newCal);
            break;
        case Color::RED:
            EEPROM.put(redAddr, newCal);
            break;
        case Color::BLACK:
            EEPROM.put(blackAddr, newCal);
            break;
        case Color::DARK_BLUE:
            EEPROM.put(darkBlueAddr, newCal);
            break;
        case Color::PURPLE:
            EEPROM.put(purpleAddr, newCal);
            break; 
        default:
            Serial.println("ERROR: Invalid color for calibration");
            return;
    }
    EEPROM.commit();
}

    
