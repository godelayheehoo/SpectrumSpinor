/*
TODO:
- when we have multiple rings and sensors, we'll replace each number in the menu grid with the assigned letter.
The current working letter will be in the top left color, aux button will cycle through them.

- Way down the road, we need to work in a way to auto-calibrate, or at least speed up the process.
Maybe a menu called "calibrate colors" and then have calibrate orange, calibrate blue, etc.
Also will need a menu functionality that just displays the current color seen.

- Improve encoder turns, make it match detents. Naive dividing by two does not work.. 

- Forcing sensor update for troubleshoot menu 2 on startup and switch doesn't work i think.

- remove display test stuff from setup

- need to work in the manual calibration, which is going to suck.

- need to add in dark to colorhelper, colordatabase, etc.

- calibration is still really screwy

- add "restore defaults" option for color cal menu

- pretty sure I don't need the SensorCalibration objects

- move defaults into SystemConfig.h (for menu stuff)

- make the first calibration option a noop because of how often I double click by accident.s


*/
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "MenuManager.h"
#include "PinDefinitions.h"
#include "ColorHelper.h"
#include "ColorEnum.h"
#include <MIDI.h>
#include "SystemConfig.h"
#include <ESP32Encoder.h>
#include <EEPROM.h>
#include "EEPROMAddresses.h"
#include "ColorInfo.h"

//checks
// static_assert(sizeof(ColorHelper) == 124, "ColorHelper struct size must be 124 bytes for EEPROM layout!");


// TCA9548A I2C Multiplexer setup
#define TCA_ADDR 0x70

// MIDI setup
HardwareSerial MIDIserial(1);
MIDI_CREATE_INSTANCE(HardwareSerial, MIDIserial, MIDI);

// Button debouncing helper
struct ButtonHelper {
  int pin;
  bool lastState;
  bool currentState;
  bool initialState;
  unsigned long lastChangeTime;
  unsigned long debounceDelay;
  
  ButtonHelper(int buttonPin, unsigned long debounceMs = 50) 
    : pin(buttonPin), debounceDelay(debounceMs), lastChangeTime(0) {
    pinMode(pin, INPUT_PULLUP);
    initialState = digitalRead(pin);  // Assume this is the unpressed state
    lastState = initialState;
    currentState = initialState;
  }
  
  bool isPressed() {
    bool reading = digitalRead(pin);
    unsigned long currentTime = millis();
    
    if (reading != lastState) {
      lastChangeTime = currentTime;
    }
    
    if ((currentTime - lastChangeTime) > debounceDelay) {
      if (reading != currentState) {
        currentState = reading;
        lastState = reading;
        // Return true when state changes from initial (unpressed) to opposite (pressed)
        return (currentState != initialState);
      }
    }
    
    lastState = reading;
    return false;
  }
};

// Display setup - SH1106 OLED
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MenuManager menu(display);

// Color sensor setup
ColorHelper colorHelperA(true, &menu); // Enable normalization
ColorHelper colorHelperB(true, &menu);
ColorHelper colorHelperC(true, &menu);
ColorHelper colorHelperD(true, &menu);

ColorHelper* colorHelpers[4]{&colorHelperA, &colorHelperB, &colorHelperC, &colorHelperD};
ColorHelper* activeColorSensor = nullptr;

// extern SensorCalibration sensorCalibrations[4];

// Scale manager setup

// Encoder setup

ESP32Encoder enc;
long lastEncoderPos = 0;

// Button helpers
ButtonHelper encoderBtn(ENCODER_BTN);
ButtonHelper conBtn(CON_BTN);
ButtonHelper bakBtn(BAK_BTN);
ButtonHelper panicBtn(PANIC_BTN);

// Script States
Color currentColorA = Color::UNKNOWN;
Color currentColorB = Color::UNKNOWN;
Color currentColorC = Color::UNKNOWN;
Color currentColorD = Color::UNKNOWN;

byte oldMidiNote = 0;
byte lastNoteA = 0;
byte lastNoteB = 0;
byte lastNoteC = 0;
byte lastNoteD = 0;

// Interrupt flags
bool encoderButtonFlag = false;
bool conButtonFlag = false;
bool backButtonFlag = false;


//function prototypes  

void IRAM_ATTR encoderButtonISR() {
  encoderButtonFlag = true;
}

void IRAM_ATTR conButtonISR() {
  conButtonFlag = true;
}

void IRAM_ATTR backButtonISR() {
  backButtonFlag = true;
}

// TCA9548A I2C Multiplexer functions
void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

// Optional: disable all channels
void tcaDisableAll() {
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(0); // no bits set
  Wire.endTransmission();
}

//helper functions
void midiPanic(){
  // Send All Notes Off message on all channels
  for (uint8_t channel = 1; channel <= 16; channel++) {
    MIDI.sendControlChange(123, 0, channel); // 123 = All Notes Off
  }
}

void sendAllNotesOff(uint8_t channel) {
  // Send All Notes Off message to specific channel
  MIDI.sendControlChange(123, 0, channel); // 123 = All Notes Off
  Serial.print("Sent ALL NOTES OFF to channel ");
  Serial.println(channel);
}

void resetOLED() {
  Serial.println("Starting OLED reset...");
  display.clearDisplay();      // Clear the display buffer
  display.display();           // Send clear buffer to display
  delay(50);                   // Minimal delay
  menu.render();               // Redraw the UI
  Serial.println("OLED reset complete");
}

//////////////////////////////////
/////////   SETUP ///////////////
////////////////////////////////

void setup() {
  Serial.begin(115200);
  Serial.print("Size of colorhelper is: ");
Serial.println(sizeof(ColorHelper));
  Serial.print("Sanity check magic number is: ");
  Serial.println(EEPROM_MAGIC_VALUE);
  Serial.println("Starting up...");

  Serial.println("Setting up MIDI...");
  MIDIserial.begin(MIDI_BAUD_RATE, SERIAL_8N1, MIDI_IN_PIN, MIDI_OUT_PIN);
  
  // Initialize I2C for OLED display
  Serial.println("Initializing I2C for display...");
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println("I2C initialized");

  EEPROM.begin(1024); // Initialize EEPROM with 1KB size
  //DEBUG: immediately check midi channel
  byte check;
  EEPROM.get(ACTIVE_MIDI_CHANNEL_A_ADDR, check);
  Serial.print("Read back from EEPROM for A at startup, immediately after begin(): ");
  Serial.println(check);

  if (!display.begin(OLED_I2C_ADDRESS)) {
    Serial.println("SH1106G allocation failed");
    while (1);
  }

  // OLED Test Functions - comment out when not needed
  Serial.println("Running OLED tests...");
  
  // Test 1: Fill screen with white/black
  display.clearDisplay();
  display.fillScreen(OLED_WHITE);
  display.display();
  delay(50);
  display.clearDisplay();
  display.display();
  delay(50);
  
  // Test 2: Draw some shapes
  display.clearDisplay();
  display.drawRect(10, 10, 50, 25, OLED_WHITE);
  display.fillCircle(80, 32, 15, OLED_WHITE);
  display.drawLine(0, 0, 127, 63, OLED_WHITE);
  display.display();
  delay(100);
  
  // Test 3: Text rendering
  display.clearDisplay();
  display.setTextColor(OLED_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OLED Test");
  display.setTextSize(1);
  display.println("Small text");
  display.println("Line 3");
  display.display();
  delay(200);
  
  // Clear screen for normal operation
  display.clearDisplay();
  display.display();
  Serial.println("OLED tests complete");

  //encoder startup
  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  //putting these backwards to get the correct direction
  enc.attachHalfQuad(ENCODER_B, ENCODER_A);
  enc.setCount(0);
  
  // Initialize encoder pins
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  
  // Attach interrupts for buttons (falling edge only - button press)
  attachInterrupt(digitalPinToInterrupt(ENCODER_BTN), encoderButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(CON_BTN), conButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BAK_BTN), backButtonISR, FALLING);
  
  Serial.println("Display and encoder initialized");
  
  // Initialize color sensors through I2C multiplexer
  Serial.println("Initializing color sensors on I2C multiplexer...");
  
  //load from EEPROM or initialize if not available
  byte storedMagicValue = EEPROM.read(EEPROM_MAGIC_ADDRESS);
  bool calibrationValid = storedMagicValue==EEPROM_MAGIC_VALUE;
  Serial.print("Stored magic value was: ");
  Serial.print(storedMagicValue);
  Serial.print(" set magic val: ");
  Serial.println(EEPROM_MAGIC_VALUE);
  
  if(calibrationValid){
    Serial.println("Stored values found!");
  }
  else{Serial.println("Stored values not found!");
  }

  for (int i = 0; i < 4; i++) {
    colorHelpers[i]->setColorDatabase(colorCalibrationDefaultDatabase, NUM_COLORS);
    tcaSelect(i);
    delay(50);
    // Always begin the sensor
    colorHelpers[i]->begin();
    Serial.print("Sensor # ");
    Serial.print(i);
    Serial.println(" begun");
    }
    //if we have valid calibrations, we overwrite the default values with the stored ones. 
    if(calibrationValid){
      
      //// setup A
      //dark
      EEPROM.get(SENSOR_A_RDARK_ADDR, colorHelperA.rDark);
      EEPROM.get(SENSOR_A_GDARK_ADDR, colorHelperA.gDark);
      EEPROM.get(SENSOR_A_BDARK_ADDR, colorHelperA.bDark);
      //gains
      EEPROM.get(SENSOR_A_RW_ADDR, colorHelperA.rW);
      EEPROM.get(SENSOR_A_GW_ADDR, colorHelperA.gW);
      EEPROM.get(SENSOR_A_BW_ADDR, colorHelperA.bW);
      //recalculate gains
      float avgW = (colorHelperA.rW + colorHelperA.gW + colorHelperA.bW) / 3.0f;
      colorHelperA.rGain = avgW / (float)colorHelperA.rW;
      colorHelperA.gGain = avgW / (float)colorHelperA.gW;
      colorHelperA.bGain = avgW / (float)colorHelperA.bW;
      
      //new calibration database
      ColorCalibration calibrationDatabaseA[NUM_COLORS];
     
      EEPROM.get(SENSOR_A_LIGHT_BLUE_CAL_ADDR, calibrationDatabaseA[0]);
      EEPROM.get(SENSOR_A_ORANGE_CAL_ADDR, calibrationDatabaseA[1]);
      EEPROM.get(SENSOR_A_PINK_CAL_ADDR, calibrationDatabaseA[2]);
      EEPROM.get(SENSOR_A_YELLOW_CAL_ADDR, calibrationDatabaseA[3]);
      EEPROM.get(SENSOR_A_GREEN_CAL_ADDR, calibrationDatabaseA[4]);
      EEPROM.get(SENSOR_A_RED_CAL_ADDR, calibrationDatabaseA[5]);
      EEPROM.get(SENSOR_A_BLACK_CAL_ADDR, calibrationDatabaseA[6]);
      EEPROM.get(SENSOR_A_DARK_BLUE_CAL_ADDR, calibrationDatabaseA[7]);
      EEPROM.get(SENSOR_A_PURPLE_CAL_ADDR, calibrationDatabaseA[8]);
      EEPROM.get(SENSOR_A_WHITE_CAL_ADDR, calibrationDatabaseA[9]);

      colorHelperA.setColorDatabase(calibrationDatabaseA, NUM_COLORS);

      //// setup B
      //dark
      EEPROM.get(SENSOR_B_RDARK_ADDR, colorHelperB.rDark);
      EEPROM.get(SENSOR_B_GDARK_ADDR, colorHelperB.gDark);
      EEPROM.get(SENSOR_B_BDARK_ADDR, colorHelperB.bDark);
      //gains
      EEPROM.get(SENSOR_B_RW_ADDR, colorHelperB.rW);
      EEPROM.get(SENSOR_B_GW_ADDR, colorHelperB.gW);
      EEPROM.get(SENSOR_B_BW_ADDR, colorHelperB.bW);
      //recalculate gains
      avgW = (colorHelperB.rW + colorHelperB.gW + colorHelperB.bW) / 3.0f;
      colorHelperB.rGain = avgW / (float)colorHelperB.rW;
      colorHelperB.gGain = avgW / (float)colorHelperB.gW;
      colorHelperB.bGain = avgW / (float)colorHelperB.bW;
      //new calibration database
      ColorCalibration calibrationDatabaseB[NUM_COLORS];
      EEPROM.get(SENSOR_B_LIGHT_BLUE_CAL_ADDR, calibrationDatabaseB[0]);
      EEPROM.get(SENSOR_B_ORANGE_CAL_ADDR, calibrationDatabaseB[1]);
      EEPROM.get(SENSOR_B_PINK_CAL_ADDR, calibrationDatabaseB[2]);
      EEPROM.get(SENSOR_B_YELLOW_CAL_ADDR, calibrationDatabaseB[3]);
      EEPROM.get(SENSOR_B_GREEN_CAL_ADDR, calibrationDatabaseB[4]);
      EEPROM.get(SENSOR_B_RED_CAL_ADDR, calibrationDatabaseB[5]);
      EEPROM.get(SENSOR_B_BLACK_CAL_ADDR, calibrationDatabaseB[6]);
      EEPROM.get(SENSOR_B_DARK_BLUE_CAL_ADDR, calibrationDatabaseB[7]);
      EEPROM.get(SENSOR_B_PURPLE_CAL_ADDR, calibrationDatabaseB[8]);
      EEPROM.get(SENSOR_B_WHITE_CAL_ADDR, calibrationDatabaseB[9]);

      colorHelperB.setColorDatabase(calibrationDatabaseB, NUM_COLORS);

      //// setup C
      //dark
      EEPROM.get(SENSOR_C_RDARK_ADDR, colorHelperC.rDark);
      EEPROM.get(SENSOR_C_GDARK_ADDR, colorHelperC.gDark);
      EEPROM.get(SENSOR_C_BDARK_ADDR, colorHelperC.bDark);
      //gains
      EEPROM.get(SENSOR_C_RW_ADDR, colorHelperC.rW);
      EEPROM.get(SENSOR_C_GW_ADDR, colorHelperC.gW);
      EEPROM.get(SENSOR_C_BW_ADDR, colorHelperC.bW);
      //recalculate gains
      avgW = (colorHelperC.rW + colorHelperC.gW + colorHelperC.bW) / 3.0f;
      colorHelperC.rGain = avgW / (float)colorHelperC.rW;
      colorHelperC.gGain = avgW / (float)colorHelperC.gW;
      colorHelperC.bGain = avgW / (float)colorHelperC.bW;
      //new calibration database
      ColorCalibration calibrationDatabaseC[NUM_COLORS];
      EEPROM.get(SENSOR_C_LIGHT_BLUE_CAL_ADDR, calibrationDatabaseC[0]);
      EEPROM.get(SENSOR_C_ORANGE_CAL_ADDR, calibrationDatabaseC[1]);
      EEPROM.get(SENSOR_C_PINK_CAL_ADDR, calibrationDatabaseC[2]);
      EEPROM.get(SENSOR_C_YELLOW_CAL_ADDR, calibrationDatabaseC[3]);
      EEPROM.get(SENSOR_C_GREEN_CAL_ADDR, calibrationDatabaseC[4]);
      EEPROM.get(SENSOR_C_RED_CAL_ADDR, calibrationDatabaseC[5]);
      EEPROM.get(SENSOR_C_BLACK_CAL_ADDR, calibrationDatabaseC[6]);
      EEPROM.get(SENSOR_C_DARK_BLUE_CAL_ADDR, calibrationDatabaseC[7]);
      EEPROM.get(SENSOR_C_PURPLE_CAL_ADDR, calibrationDatabaseC[8]);
      EEPROM.get(SENSOR_C_WHITE_CAL_ADDR, calibrationDatabaseC[9]);
      
      colorHelperC.setColorDatabase(calibrationDatabaseC, NUM_COLORS);

      //// setup D
      //dark
      EEPROM.get(SENSOR_D_RDARK_ADDR, colorHelperD.rDark);
      EEPROM.get(SENSOR_D_GDARK_ADDR, colorHelperD.gDark);
      EEPROM.get(SENSOR_D_BDARK_ADDR, colorHelperD.bDark);
      //gains
      EEPROM.get(SENSOR_D_RW_ADDR, colorHelperD.rW);
      EEPROM.get(SENSOR_D_GW_ADDR, colorHelperD.gW);
      EEPROM.get(SENSOR_D_BW_ADDR, colorHelperD.bW);
      //recalculate gains
      avgW = (colorHelperD.rW + colorHelperD.gW + colorHelperD.bW) / 3.0f;
      colorHelperD.rGain = avgW / (float)colorHelperD.rW; 
      colorHelperD.gGain = avgW / (float)colorHelperD.gW;
      colorHelperD.bGain = avgW / (float)colorHelperD.bW;
      //new calibration database
      ColorCalibration calibrationDatabaseD[NUM_COLORS];
      EEPROM.get(SENSOR_D_LIGHT_BLUE_CAL_ADDR, calibrationDatabaseD[0]);
      EEPROM.get(SENSOR_D_ORANGE_CAL_ADDR, calibrationDatabaseD[1]);
      EEPROM.get(SENSOR_D_PINK_CAL_ADDR, calibrationDatabaseD[2]);
      EEPROM.get(SENSOR_D_YELLOW_CAL_ADDR, calibrationDatabaseD[3]);
      EEPROM.get(SENSOR_D_GREEN_CAL_ADDR, calibrationDatabaseD[4]);
      EEPROM.get(SENSOR_D_RED_CAL_ADDR, calibrationDatabaseD[5]);
      EEPROM.get(SENSOR_D_BLACK_CAL_ADDR, calibrationDatabaseD[6]);
      EEPROM.get(SENSOR_D_DARK_BLUE_CAL_ADDR, calibrationDatabaseD[7]);
      EEPROM.get(SENSOR_D_PURPLE_CAL_ADDR, calibrationDatabaseD[8]);
      EEPROM.get(SENSOR_D_WHITE_CAL_ADDR, calibrationDatabaseD[9]);

      colorHelperD.setColorDatabase(calibrationDatabaseD, NUM_COLORS);
      Serial.println("color calibrations hopefully restored");
    }
    

    if(!calibrationValid){
      //write the default values to eeprom
      Serial.println("Writing default calibration values to EEPROM...");
      //// setup A
      // EEPROM.put(EEPROM_MAGIC_ADDRESS,EEPROM_MAGIC_VALUE);
      //dark
      EEPROM.put(SENSOR_A_RDARK_ADDR, colorHelperA.rDark);
      EEPROM.put(SENSOR_A_GDARK_ADDR, colorHelperA.gDark);
      EEPROM.put(SENSOR_A_BDARK_ADDR, colorHelperA.bDark);
      //gains
      EEPROM.put(SENSOR_A_RW_ADDR, colorHelperA.rW);
      EEPROM.put(SENSOR_A_GW_ADDR, colorHelperA.gW);
      EEPROM.put(SENSOR_A_BW_ADDR, colorHelperA.bW);
      
      //calibrations
      EEPROM.put(SENSOR_A_LIGHT_BLUE_CAL_ADDR, colorCalibrationDefaultDatabase[0]);
      EEPROM.put(SENSOR_A_ORANGE_CAL_ADDR, colorCalibrationDefaultDatabase[1]);
      EEPROM.put(SENSOR_A_PINK_CAL_ADDR, colorCalibrationDefaultDatabase[2]);
      EEPROM.put(SENSOR_A_YELLOW_CAL_ADDR, colorCalibrationDefaultDatabase[3]);
      EEPROM.put(SENSOR_A_GREEN_CAL_ADDR, colorCalibrationDefaultDatabase[4]);
      EEPROM.put(SENSOR_A_RED_CAL_ADDR, colorCalibrationDefaultDatabase[5]);
      EEPROM.put(SENSOR_A_BLACK_CAL_ADDR, colorCalibrationDefaultDatabase[6]);
      EEPROM.put(SENSOR_A_DARK_BLUE_CAL_ADDR, colorCalibrationDefaultDatabase[7]);
      EEPROM.put(SENSOR_A_PURPLE_CAL_ADDR, colorCalibrationDefaultDatabase[8]);
      EEPROM.put(SENSOR_A_WHITE_CAL_ADDR, colorCalibrationDefaultDatabase[9]);
      //// setup B
      //dark
      EEPROM.put(SENSOR_B_RDARK_ADDR, colorHelperB.rDark);
      EEPROM.put(SENSOR_B_GDARK_ADDR, colorHelperB.gDark);
      EEPROM.put(SENSOR_B_BDARK_ADDR, colorHelperB.bDark);
      //gains
      EEPROM.put(SENSOR_B_RW_ADDR, colorHelperB.rW);

      EEPROM.put(SENSOR_B_GW_ADDR, colorHelperB.gW);
      EEPROM.put(SENSOR_B_BW_ADDR, colorHelperB.bW);

      //calibrations
      EEPROM.put(SENSOR_B_LIGHT_BLUE_CAL_ADDR, colorCalibrationDefaultDatabase[0]);
      EEPROM.put(SENSOR_B_ORANGE_CAL_ADDR, colorCalibrationDefaultDatabase[1]);
      EEPROM.put(SENSOR_B_PINK_CAL_ADDR, colorCalibrationDefaultDatabase[2]);
      EEPROM.put(SENSOR_B_YELLOW_CAL_ADDR, colorCalibrationDefaultDatabase[3]);
      EEPROM.put(SENSOR_B_GREEN_CAL_ADDR, colorCalibrationDefaultDatabase[4]);
      EEPROM.put(SENSOR_B_RED_CAL_ADDR, colorCalibrationDefaultDatabase[5]);
      EEPROM.put(SENSOR_B_BLACK_CAL_ADDR, colorCalibrationDefaultDatabase[6]);
      EEPROM.put(SENSOR_B_DARK_BLUE_CAL_ADDR, colorCalibrationDefaultDatabase[7]);
      EEPROM.put(SENSOR_B_PURPLE_CAL_ADDR, colorCalibrationDefaultDatabase[8]);
      EEPROM.put(SENSOR_B_WHITE_CAL_ADDR, colorCalibrationDefaultDatabase[9]);

      //// setup C
      //dark
      EEPROM.put(SENSOR_C_RDARK_ADDR, colorHelperC.rDark);
      EEPROM.put(SENSOR_C_GDARK_ADDR, colorHelperC.gDark);
      EEPROM.put(SENSOR_C_BDARK_ADDR, colorHelperC.bDark);
      //gains
      EEPROM.put(SENSOR_C_RW_ADDR, colorHelperC.rW);
      EEPROM.put(SENSOR_C_GW_ADDR, colorHelperC.gW);
      EEPROM.put(SENSOR_C_BW_ADDR, colorHelperC.bW);

      //calibrations
      EEPROM.put(SENSOR_C_LIGHT_BLUE_CAL_ADDR, colorCalibrationDefaultDatabase[0]);
      EEPROM.put(SENSOR_C_ORANGE_CAL_ADDR, colorCalibrationDefaultDatabase[1]);
      EEPROM.put(SENSOR_C_PINK_CAL_ADDR, colorCalibrationDefaultDatabase[2]);
      EEPROM.put(SENSOR_C_YELLOW_CAL_ADDR, colorCalibrationDefaultDatabase[3]);
      EEPROM.put(SENSOR_C_GREEN_CAL_ADDR, colorCalibrationDefaultDatabase[4]);
      EEPROM.put(SENSOR_C_RED_CAL_ADDR, colorCalibrationDefaultDatabase[5]);
      EEPROM.put(SENSOR_C_BLACK_CAL_ADDR, colorCalibrationDefaultDatabase[6]);
      EEPROM.put(SENSOR_C_DARK_BLUE_CAL_ADDR, colorCalibrationDefaultDatabase[7]);
      EEPROM.put(SENSOR_C_PURPLE_CAL_ADDR, colorCalibrationDefaultDatabase[8]);
      EEPROM.put(SENSOR_C_WHITE_CAL_ADDR, colorCalibrationDefaultDatabase[9]);


      //// setup D
      //dark
      EEPROM.put(SENSOR_D_RDARK_ADDR, colorHelperD.rDark);
      EEPROM.put(SENSOR_D_GDARK_ADDR, colorHelperD.gDark);
      EEPROM.put(SENSOR_D_BDARK_ADDR, colorHelperD.bDark);
      //gains
      EEPROM.put(SENSOR_D_RW_ADDR, colorHelperD.rW);
      EEPROM.put(SENSOR_D_GW_ADDR, colorHelperD.gW);
      EEPROM.put(SENSOR_D_BW_ADDR, colorHelperD.bW);

      //calibrations
      EEPROM.put(SENSOR_D_LIGHT_BLUE_CAL_ADDR, colorCalibrationDefaultDatabase[0]);
      EEPROM.put(SENSOR_D_ORANGE_CAL_ADDR, colorCalibrationDefaultDatabase[1]);
      EEPROM.put(SENSOR_D_PINK_CAL_ADDR, colorCalibrationDefaultDatabase[2]);
      EEPROM.put(SENSOR_D_YELLOW_CAL_ADDR, colorCalibrationDefaultDatabase[3]);
      EEPROM.put(SENSOR_D_GREEN_CAL_ADDR, colorCalibrationDefaultDatabase[4]);
      EEPROM.put(SENSOR_D_RED_CAL_ADDR, colorCalibrationDefaultDatabase[5]);
      EEPROM.put(SENSOR_D_BLACK_CAL_ADDR, colorCalibrationDefaultDatabase[6]);
      EEPROM.put(SENSOR_D_DARK_BLUE_CAL_ADDR, colorCalibrationDefaultDatabase[7]);
      EEPROM.put(SENSOR_D_PURPLE_CAL_ADDR, colorCalibrationDefaultDatabase[8]);
      EEPROM.put(SENSOR_D_WHITE_CAL_ADDR, colorCalibrationDefaultDatabase[9]);


      EEPROM.commit();
    }

  // Disable all channels for now
  tcaDisableAll();

  Serial.println("Loading menu values....");
  //load menu values
  if(calibrationValid){

    byte check;
    EEPROM.get(ACTIVE_MIDI_CHANNEL_A_ADDR, check);
    Serial.print("Read back from EEPROM for A at startup: ");
    Serial.println(check);

    Serial.println("Using stored menu values");
    EEPROM.get(ACTIVE_MIDI_CHANNEL_A_ADDR, menu.activeMIDIChannelA);
    Serial.print("Settting sensor A channel to ");
    Serial.println(menu.activeMIDIChannelA);
    EEPROM.get(ACTIVE_MIDI_CHANNEL_B_ADDR, menu.activeMIDIChannelB);
    EEPROM.get(ACTIVE_MIDI_CHANNEL_C_ADDR, menu.activeMIDIChannelC);
    EEPROM.get(ACTIVE_MIDI_CHANNEL_D_ADDR, menu.activeMIDIChannelD);

    EEPROM.get(OCTAVE_A_ADDR, menu.octaveA);
    EEPROM.get(OCTAVE_B_ADDR, menu.octaveB);
    EEPROM.get(OCTAVE_C_ADDR, menu.octaveC);
    EEPROM.get(OCTAVE_D_ADDR, menu.octaveD);

  }
  else{
    Serial.println("EEPROM not valid, using default values");
    menu.activeMIDIChannelA = 3; // Default to channel 3
    menu.activeMIDIChannelB = 6; // Default to channel 6
    menu.activeMIDIChannelC = 7; // Default to channel 7
    menu.activeMIDIChannelD = 12;// Default to channel 12

    menu.octaveA = 1;
    menu.octaveB = 3;
    menu.octaveC = 4;
    menu.octaveD = 6;

    EEPROM.put(ACTIVE_MIDI_CHANNEL_A_ADDR, menu.activeMIDIChannelA);
    EEPROM.put(ACTIVE_MIDI_CHANNEL_B_ADDR, menu.activeMIDIChannelB);
    EEPROM.put(ACTIVE_MIDI_CHANNEL_C_ADDR, menu.activeMIDIChannelC);
    EEPROM.put(ACTIVE_MIDI_CHANNEL_D_ADDR, menu.activeMIDIChannelD);

    EEPROM.put(OCTAVE_A_ADDR, menu.octaveA);
    EEPROM.put(OCTAVE_B_ADDR, menu.octaveB);
    EEPROM.put(OCTAVE_C_ADDR, menu.octaveC);
    EEPROM.put(OCTAVE_D_ADDR, menu.octaveD);

    EEPROM.put(EEPROM_MAGIC_ADDRESS,EEPROM_MAGIC_VALUE);
    EEPROM.commit();
  

    Serial.println("Default menus settings now saved to EEPROM");
  }
  
  // Set up MIDI callback for MenuManager
  menu.setAllNotesOffCallback(sendAllNotesOff);
  
  // Initial menu render
  menu.render();
  
  Serial.println("Setup complete");


  Serial.println("---------- DEBUG -----------");
  //print out the saved dark offset, gain, and yellow cal values for sensor A
  Serial.print("dark R,G,B: ");
  Serial.print(colorHelperA.rDark);
  Serial.print(", ");
  Serial.print(colorHelperA.gDark);
  Serial.print(", ");
  Serial.println(colorHelperA.bDark);
  
  Serial.print("gain RGB: ");
  Serial.print(colorHelperA.rGain);
  Serial.print(", ");
  Serial.print(colorHelperA.gGain);
  Serial.print(", ");
  Serial.println(colorHelperA.bGain);

  Serial.print("Red RGB:");
  byte redIdx = colorToIndex(Color::RED);
  Serial.print(colorHelperA.calibrationDatabase[redIdx].red);
  Serial.print(", ");
  Serial.print(colorHelperA.calibrationDatabase[redIdx].green);
  Serial.print(", ");
  Serial.println(colorHelperA.calibrationDatabase[redIdx].blue);
}

void loop() {
  static unsigned long lastPollTime = 0;
  static unsigned long lastColorTime = 0;
  static unsigned long lastSensorSettleTime = 0;
  static uint8_t currentSensorIndex = 0;
  static bool sensorSettling = false;
  
  const unsigned long pollInterval = 5; // Poll every 5ms for better responsiveness
  const unsigned long colorInterval = 50; // Much faster color checking (50ms)
  const unsigned long settleTime = 1; // Minimal settle time


  
  unsigned long currentTime = millis();
  
  //check encoder and pass in turns if turns!=0
  int newEncoderPos =  enc.getCount();
  int encoderTurns = newEncoderPos - lastEncoderPos;
  if (encoderTurns != 0) {
    lastEncoderPos = newEncoderPos;
    menu.handleEncoder(encoderTurns); 
    menu.render();
  }  

#ifdef TROUBLESHOOT
  // Debug: Print ISR call count and flag counts periodically
  static unsigned long lastDebugTime = 0;
  static int cwCount = 0, ccwCount = 0;
  
  if (encoderCWFlag) {
    cwCount++;
  }
  if (encoderCCWFlag) {
    ccwCount++;
  }
  
  if (currentTime - lastDebugTime > 5000) { // Every 5 seconds
    Serial.print("ISR calls: ");
    Serial.print(encoderISRCounter);
    Serial.print(", CW flags: ");
    Serial.print(cwCount);
    Serial.print(", CCW flags: ");
    Serial.println(ccwCount);
    
    encoderISRCounter = 0;
    cwCount = 0;
    ccwCount = 0;
    lastDebugTime = currentTime;
  }
#endif
  
  // Check button flags set by interrupts with toggle debouncing
  static bool ignoreNextEncoderButton = false;
  static bool ignoreNextConButton = false;
  static bool ignoreNextBackButton = false;
  
  if (encoderButtonFlag) {
    encoderButtonFlag = false;
    
    if (ignoreNextEncoderButton) {
      ignoreNextEncoderButton = false;
      // Ignore this trigger
    } else {
      ignoreNextEncoderButton = true;
      menu.handleInput(ENCODER_BUTTON);
      menu.render();
    }
  }
  
  if (conButtonFlag) {
    conButtonFlag = false;
    
    if (ignoreNextConButton) {
      ignoreNextConButton = false;
      // Ignore this trigger
    } else {
      ignoreNextConButton = true;
      menu.handleInput(CON_BUTTON);
      menu.render();
    }
  }
  
  if (backButtonFlag) {
    backButtonFlag = false;
    
    if (ignoreNextBackButton) {
      ignoreNextBackButton = false;
      // Ignore this trigger
    } else {
      ignoreNextBackButton = true;
      menu.handleInput(BAK_BUTTON);
      menu.render();
    }
  }
  
  // Handle RGB update request for all sensors when switching to troubleshoot mode 1
  if (menu.requestRGBUpdate && menu.currentMenu == TROUBLESHOOT_MENU && menu.troubleshootMode == 1) {
    Serial.println("Force updating RGB for all sensors...");
    
    // Update RGB for all four sensors
    for (int sensorIdx = 0; sensorIdx < 4; sensorIdx++) {
      tcaSelect(sensorIdx);
      delay(5); // Brief settling time
      
      float r, g, b, c;
      activeColorSensor->getCalibratedData(&r, &g, &b);
      
      // Update RGB values in menu
      switch (sensorIdx) {
        case 0: menu.updateCurrentRGBA(r, g, b); break;
        case 1: menu.updateCurrentRGBB(r, g, b); break;
        case 2: menu.updateCurrentRGBC(r, g, b); break;
        case 3: menu.updateCurrentRGBD(r, g, b); break;
      }
    }
    
    // Clear the flag and restore current sensor
    menu.requestRGBUpdate = false;
    tcaSelect(currentSensorIndex);
  }

  // Keep panic button as polling since it's hardware-debounced
  if (currentTime - lastPollTime >= pollInterval) {
    lastPollTime = currentTime;
  }

  //panic button handling (high priority)
  if (panicBtn.isPressed()){
    Serial.println("Panic!");
    midiPanic();
    resetOLED();
  }

  // Non-blocking color detection with sensor cycling
  if (currentTime - lastColorTime >= colorInterval) {
    // Start sensor settling if not already settling
    if (!sensorSettling) {
      // Serial.print("tca select on sensor");
      // Serial.println(currentSensorIndex);
      activeColorSensor = colorHelpers[currentSensorIndex]; //trying this added here
      tcaSelect(currentSensorIndex);
      lastSensorSettleTime = currentTime;
      sensorSettling = true;
      return; // Exit loop to allow other operations
    }
    else{
      // Serial.print("Sensor settling in progress for sensor #");
      // Serial.println(currentSensorIndex);
    }

    // Check if sensor has settled
    if (currentTime - lastSensorSettleTime >= settleTime) {
      // Process current sensor
      // Serial.println("checking for sensor availability");
      if (activeColorSensor->isAvailable()) {
        // Serial.print("Attempting to get color, sensor #");
        if (!activeColorSensor) {
          Serial.println("ERROR: activeColorSensor is null!");
          return;
        }
        // Serial.println(currentSensorIndex);
        // Serial.print("activeColorSensor ptr: "); 
        // Serial.println((uintptr_t)activeColorSensor, HEX);
        // Serial.print("colorDatabase ptr: "); 
        // Serial.println((uintptr_t)activeColorSensor->colorDatabase, HEX);

      //   for (int i = 0; i < 9; i++) {
      //   Serial.print("colorDatabase["); Serial.print(i); Serial.print("]: ");
      //   Serial.print(activeColorSensor->colorDatabase[i].avgR); Serial.print(", ");
      //   Serial.print(activeColorSensor->colorDatabase[i].avgG); Serial.print(", ");
      //   Serial.println(activeColorSensor->colorDatabase[i].avgB);
      // }


        Color detectedColor = activeColorSensor->getCurrentColorEnum();
        // Serial.println("Got color");
        Color* currentColorPtr = nullptr;
        String sensorName = "";
        uint8_t activeMIDIChannel = 1;
        uint8_t velocity = 127;
        
        // Determine which sensor we're processing
        switch (currentSensorIndex) {
          case 0:
            currentColorPtr = &currentColorA;
            sensorName = "Sensor A";
            activeMIDIChannel = menu.activeMIDIChannelA;
            velocity = menu.velocityA;
            break;
          case 1:
            currentColorPtr = &currentColorB;
            sensorName = "Sensor B";
            activeMIDIChannel = menu.activeMIDIChannelB;
            velocity = menu.velocityB;
            break;
          case 2:
            currentColorPtr = &currentColorC;
            sensorName = "Sensor C";
            activeMIDIChannel = menu.activeMIDIChannelC;
            velocity = menu.velocityC;
            break;
          case 3:
            currentColorPtr = &currentColorD;
            sensorName = "Sensor D";
            activeMIDIChannel = menu.activeMIDIChannelD;
            velocity = menu.velocityD;
            break;
        }
        
#ifdef TROUBLESHOOT
        // Debug: Print sensor readings periodically with raw values
        static unsigned long lastDebugPrint = 0;
        if (currentTime - lastDebugPrint > 2000) { // Every 2 seconds
          uint16_t r, g, b, c;
          colorSensor.getRawData(&r, &g, &b, &c);
          
          Serial.print(sensorName);
          Serial.print(": ");
          Serial.print(colorToString(detectedColor));
          Serial.print(" (R:");
          Serial.print(r);
          Serial.print(" G:");
          Serial.print(g);
          Serial.print(" B:");
          Serial.print(b);
          Serial.print(" C:");
          Serial.print(c);
          Serial.print(")");
          
          if (currentSensorIndex == 3) { // Print newline after sensor D
            Serial.println();
            lastDebugPrint = currentTime;
          } else {
            Serial.print(" | ");
          }
        }
#endif

        // Update RGB values if in troubleshoot mode 1 (RGB display) and color changed
        if (menu.currentMenu == TROUBLESHOOT_MENU && menu.troubleshootMode == 1 && 
            detectedColor != Color::UNKNOWN && detectedColor != *currentColorPtr && currentColorPtr != nullptr) {
          float r, g, b;
          activeColorSensor->getCalibratedData(&r, &g, &b);
          
          
          // Update RGB values in menu for troubleshoot mode
          switch (currentSensorIndex) {
            case 0: menu.updateCurrentRGBA(r, g, b); break;
            case 1: menu.updateCurrentRGBB(r, g, b); break;
            case 2: menu.updateCurrentRGBC(r, g, b); break;
            case 3: menu.updateCurrentRGBD(r, g, b); break;
          }
        }
        
        
        // Process color change if detected and valid
        if (detectedColor != Color::UNKNOWN && detectedColor != *currentColorPtr && currentColorPtr != nullptr) {
         Serial.print("New color:");
          Serial.println(colorToString(detectedColor));
          switch(currentSensorIndex){
            case 0:
              oldMidiNote = lastNoteA;
              break;
            case 1:
              oldMidiNote = lastNoteB;
              break;
            case 2:
              oldMidiNote = lastNoteC;
              break;
            case 3:
              oldMidiNote = lastNoteD;
              break;
          }
         
          // Send note off for previous color
         MIDI.sendNoteOff(oldMidiNote, 0, activeMIDIChannel);
         Serial.print("Sending note off to note ");
         Serial.print(oldMidiNote);
         Serial.print("on channel ");
         Serial.println(activeMIDIChannel);
          
          // Send note on for new color
          int newMidiNote = menu.scaleManager.colorToMIDINote(detectedColor);
          Serial.print("new midi note (pre-octave):");
          Serial.println(newMidiNote);
          // Adjust based on octave using signed arithmetic to allow negative offsets
          int offset;
          switch (currentSensorIndex){
            case 0:
              offset = (int(menu.octaveA) - 4) * 12;
              newMidiNote += offset;
              // clamp to valid MIDI range
              if (newMidiNote < 0) newMidiNote = 0;
              if (newMidiNote > 127) newMidiNote = 127;
              lastNoteA = (uint8_t)newMidiNote;
              break;
            case 1:
              offset = (int(menu.octaveB) - 4) * 12;
              newMidiNote += offset;
              if (newMidiNote < 0) newMidiNote = 0;
              if (newMidiNote > 127) newMidiNote = 127;
              lastNoteB = (uint8_t)newMidiNote;
              break;  
            case 2:
              offset = (int(menu.octaveC) - 4) * 12;
              newMidiNote += offset;
              if (newMidiNote < 0) newMidiNote = 0;
              if (newMidiNote > 127) newMidiNote = 127;
              lastNoteC = (uint8_t)newMidiNote;
              break;
            case 3:
              offset = (int(menu.octaveD) - 4) * 12;
              newMidiNote += offset;
              if (newMidiNote < 0) newMidiNote = 0;
              if (newMidiNote > 127) newMidiNote = 127;
              lastNoteD = (uint8_t)newMidiNote;
              break;
          }
   

          byte currentChannel = (detectedColor == Color::WHITE) ? 0 : activeMIDIChannel;
          MIDI.sendNoteOn((uint8_t)newMidiNote, velocity, currentChannel);

          switch(currentSensorIndex){
            case 0:
              lastNoteA = newMidiNote;
              Serial.print("Set last note A to ");
              Serial.println(lastNoteA);
              break;
            case 1:
              lastNoteB = newMidiNote;
              Serial.print("Set last note B to ");
              Serial.println(lastNoteB);
              break;  
            case 2:
              lastNoteC = newMidiNote;
              Serial.print("Set last note C to ");
              Serial.println(lastNoteC);
              break;
            case 3:
              lastNoteD = newMidiNote;
              Serial.print("Set last note D to ");
              Serial.println(lastNoteD);
              break;
          }
          
          // Update menu display
          switch (currentSensorIndex) {
            case 0:
              menu.updateCurrentColorA(colorToString(detectedColor));
              menu.updateCurrentMIDINoteA(newMidiNote);
              break;
            case 1:
              menu.updateCurrentColorB(colorToString(detectedColor));
              menu.updateCurrentMIDINoteB(newMidiNote);
              break;
            case 2:
              menu.updateCurrentColorC(colorToString(detectedColor));
              menu.updateCurrentMIDINoteC(newMidiNote);
              break;
            case 3:
              menu.updateCurrentColorD(colorToString(detectedColor));
              menu.updateCurrentMIDINoteD(newMidiNote);
              break;
          }
          
          // Update troubleshoot display if active (less frequent)
          static unsigned long lastTroubleshootUpdate = 0;
          if (menu.currentMenu == TROUBLESHOOT_MENU && 
              (currentTime - lastTroubleshootUpdate) > 100) { // Update display max every 100ms
            menu.render();
            lastTroubleshootUpdate = currentTime;
          }
          
          *currentColorPtr = detectedColor;
        }
      }
      
      // Move to next sensor
      currentSensorIndex = (currentSensorIndex + 1) % 4;
      sensorSettling = false;
      
      // If we've cycled through all sensors, reset timer
      if (currentSensorIndex == 0) {
        tcaDisableAll(); // Disable multiplexer after full cycle
        lastColorTime = currentTime;
      }
    }
  }

  //check if we're calibrating A
  if(menu.pendingCalibrationA!=PendingCalibrationA::NONE){
    menu.startCalibrationCountdown();
    ColorHelper* sensorA = colorHelpers[0];
    tcaSelect(0);
    if(menu.pendingCalibrationA == PendingCalibrationA::DARK_OFFSET){
      Serial.print("Initial r,g,b dark offsets:");
      Serial.print(sensorA->rDark);
      Serial.print(", ");
      Serial.print(sensorA->gDark);
      Serial.print(", ");
      Serial.println(sensorA->bDark);
      sensorA->calibrateDark();
      Serial.print("Adjusted r,g,b dark offsets:");
      Serial.print(sensorA->rDark);
      Serial.print(", ");
      Serial.print(sensorA->gDark);
      Serial.print(", ");
      Serial.println(sensorA->bDark);
    }
    else if(menu.pendingCalibrationA==PendingCalibrationA::GAINS){
      Serial.print("Initial r,g,b gains:");
      Serial.print(sensorA->rGain);
      Serial.print(", ");
      Serial.print(sensorA->gGain);
      Serial.print(", ");
      Serial.println(sensorA->bGain);
      sensorA->calibrateWhiteGains();
      Serial.print("Adjusted r,g,b gains:");
      Serial.print(sensorA->rGain);
      Serial.print(", ");
      Serial.print(sensorA->gGain);
      Serial.print(", ");
      Serial.println(sensorA->bGain);
    }
    else if(menu.pendingCalibrationA==PendingCalibrationA::APPLY_TO_BCD){
      //Apply calibration values from A to B, C, and D
      ColorHelper* targetHelper;
      for(int i=1;i<4; i++){
         targetHelper = colorHelpers[i];
         //set dark values
         targetHelper->rDark = colorHelperA.rDark;
         targetHelper->gDark = colorHelperA.gDark;
         targetHelper->bDark = colorHelperA.bDark;

         //set gain values
         targetHelper->rW = colorHelperA.rW;
         targetHelper->gW = colorHelperA.gW;
         targetHelper->bW = colorHelperA.bW;
         targetHelper->rGain = colorHelperA.rGain;
         targetHelper->gGain = colorHelperA.gGain;
         targetHelper->bGain = colorHelperA.bGain;

         //set color values
         for(int j=0; j<NUM_COLORS; j++){
          menu.display.clearDisplay();
          menu.display.setCursor(0, 0);
          menu.display.print(">  ");
          menu.display.print(colorToString(indexToColor(j)));
          targetHelper->calibrationDatabase[j] = colorHelperA.calibrationDatabase[j];
         }
      }
      menu.pendingCalibrationA = PendingCalibrationA::NONE;
      //todo: some kind of menu feedback
      menu.render();

    }
    else{
    Color selectedColor = menu.pendingCalibrationA==PendingCalibrationA::LIGHT_BLUE?Color::LIGHT_BLUE:
                           menu.pendingCalibrationA==PendingCalibrationA::ORANGE?Color::ORANGE:
                           menu.pendingCalibrationA==PendingCalibrationA::PINK?Color::PINK:
                           menu.pendingCalibrationA==PendingCalibrationA::YELLOW?Color::YELLOW:
                           menu.pendingCalibrationA==PendingCalibrationA::GREEN?Color::GREEN:
                           menu.pendingCalibrationA==PendingCalibrationA::RED?Color::RED:
                           menu.pendingCalibrationA==PendingCalibrationA::BLACK?Color::BLACK:
                           menu.pendingCalibrationA==PendingCalibrationA::DARK_BLUE?Color::DARK_BLUE:
                           menu.pendingCalibrationA==PendingCalibrationA::PURPLE?Color::PURPLE:
                           menu.pendingCalibrationA==PendingCalibrationA::WHITE?Color::WHITE:
                           Color::UNKNOWN;

    colorHelperA.calibrateColor(selectedColor); //safety
    
  }
  menu.pendingCalibrationA = PendingCalibrationA::NONE;
  menu.render();
}
  //check if we're calibrating B
  if(menu.pendingCalibrationB!=PendingCalibrationB::NONE){
    menu.startCalibrationCountdown();
    ColorHelper* sensorB = colorHelpers[1];
    tcaSelect(1);
    if(menu.pendingCalibrationB == PendingCalibrationB::DARK_OFFSET){
      Serial.print("Initial r,g,b dark offsets:");
      Serial.print(sensorB->rDark);
      Serial.print(", ");
      Serial.print(sensorB->gDark);
      Serial.print(", ");
      Serial.println(sensorB->bDark);
      sensorB->calibrateDark();
      Serial.print("Adjusted r,g,b dark offsets:");
      Serial.print(sensorB->rDark);
      Serial.print(", ");
      Serial.print(sensorB->gDark);
      Serial.print(", ");
      Serial.println(sensorB->bDark);
    }
    else if(menu.pendingCalibrationB==PendingCalibrationB::GAINS){
      Serial.print("Initial r,g,b gains:");
      Serial.print(sensorB->rGain);
      Serial.print(", ");
      Serial.print(sensorB->gGain);
      Serial.print(", ");
      Serial.println(sensorB->bGain);
      sensorB->calibrateWhiteGains();
      Serial.print("Adjusted r,g,b gains:");
      Serial.print(sensorB->rGain);
      Serial.print(", ");
      Serial.print(sensorB->gGain);
      Serial.print(", ");
      Serial.println(sensorB->bGain);
    }
    else{
    Color selectedColor = menu.pendingCalibrationB==PendingCalibrationB::LIGHT_BLUE?Color::LIGHT_BLUE:
                           menu.pendingCalibrationB==PendingCalibrationB::ORANGE?Color::ORANGE:
                           menu.pendingCalibrationB==PendingCalibrationB::PINK?Color::PINK:
                           menu.pendingCalibrationB==PendingCalibrationB::YELLOW?Color::YELLOW:
                           menu.pendingCalibrationB==PendingCalibrationB::GREEN?Color::GREEN:
                           menu.pendingCalibrationB==PendingCalibrationB::RED?Color::RED:
                           menu.pendingCalibrationB==PendingCalibrationB::BLACK?Color::BLACK:
                           menu.pendingCalibrationB==PendingCalibrationB::DARK_BLUE?Color::DARK_BLUE:
                           menu.pendingCalibrationB==PendingCalibrationB::PURPLE?Color::PURPLE:
                           menu.pendingCalibrationB==PendingCalibrationB::WHITE?Color::WHITE:
                           Color::UNKNOWN;

    sensorB->calibrateColor(selectedColor); //safety
    
  }
  menu.pendingCalibrationB = PendingCalibrationB::NONE;
  menu.render();
  }
  //check if we're calibrating C
  if(menu.pendingCalibrationC!=PendingCalibrationC::NONE){
    menu.startCalibrationCountdown();
    ColorHelper* sensorC = colorHelpers[2];
    tcaSelect(2);
    if(menu.pendingCalibrationC == PendingCalibrationC::DARK_OFFSET){
      Serial.print("Initial r,g,b dark offsets:");
      Serial.print(sensorC->rDark);
      Serial.print(", ");
      Serial.print(sensorC->gDark);
      Serial.print(", ");
      Serial.println(sensorC->bDark);
      sensorC->calibrateDark();
      Serial.print("Adjusted r,g,b dark offsets:");
      Serial.print(sensorC->rDark);
      Serial.print(", ");
      Serial.print(sensorC->gDark);
      Serial.print(", ");
      Serial.println(sensorC->bDark);
    }
    else if(menu.pendingCalibrationC==PendingCalibrationC::GAINS){
      Serial.print("Initial r,g,b gains:");
      Serial.print(sensorC->rGain);
      Serial.print(", ");
      Serial.print(sensorC->gGain);
      Serial.print(", ");
      Serial.println(sensorC->bGain);
      sensorC->calibrateWhiteGains();
      Serial.print("Adjusted r,g,b gains:");
      Serial.print(sensorC->rGain);
      Serial.print(", ");
      Serial.print(sensorC->gGain);
      Serial.print(", ");
      Serial.println(sensorC->bGain);
    }
    else{
    Color selectedColor = menu.pendingCalibrationC==PendingCalibrationC::LIGHT_BLUE?Color::LIGHT_BLUE:
                           menu.pendingCalibrationC==PendingCalibrationC::ORANGE?Color::ORANGE:
                           menu.pendingCalibrationC==PendingCalibrationC::PINK?Color::PINK:
                           menu.pendingCalibrationC==PendingCalibrationC::YELLOW?Color::YELLOW:
                           menu.pendingCalibrationC==PendingCalibrationC::GREEN?Color::GREEN:
                           menu.pendingCalibrationC==PendingCalibrationC::RED?Color::RED:
                           menu.pendingCalibrationC==PendingCalibrationC::BLACK?Color::BLACK:
                           menu.pendingCalibrationC==PendingCalibrationC::DARK_BLUE?Color::DARK_BLUE:
                           menu.pendingCalibrationC==PendingCalibrationC::PURPLE?Color::PURPLE:
                           menu.pendingCalibrationC==PendingCalibrationC::WHITE?Color::WHITE:
                           Color::UNKNOWN;

    sensorC->calibrateColor(selectedColor); //safety
    
  }
  menu.pendingCalibrationC = PendingCalibrationC::NONE;
  menu.render();
}

  //check if we're calibrating D
  if(menu.pendingCalibrationD!=PendingCalibrationD::NONE){
    menu.startCalibrationCountdown();
    ColorHelper* sensorD = colorHelpers[3];
    tcaSelect(3);
    if(menu.pendingCalibrationD == PendingCalibrationD::DARK_OFFSET){
      Serial.print("Initial r,g,b dark offsets:");
      Serial.print(sensorD->rDark);
      Serial.print(", ");
      Serial.print(sensorD->gDark);
      Serial.print(", ");
      Serial.println(sensorD->bDark);
      sensorD->calibrateDark();
      Serial.print("Adjusted r,g,b dark offsets:");
      Serial.print(sensorD->rDark);
      Serial.print(", ");
      Serial.print(sensorD->gDark);
      Serial.print(", ");
      Serial.println(sensorD->bDark);
    }
    else if(menu.pendingCalibrationD==PendingCalibrationD::GAINS){
      Serial.print("Initial r,g,b gains:");
      Serial.print(sensorD->rGain);
      Serial.print(", ");
      Serial.print(sensorD->gGain);
      Serial.print(", ");
      Serial.println(sensorD->bGain);
      sensorD->calibrateWhiteGains();
      Serial.print("Adjusted r,g,b gains:");
      Serial.print(sensorD->rGain);
      Serial.print(", ");
      Serial.print(sensorD->gGain);
      Serial.print(", ");
      Serial.println(sensorD->bGain);
    }
    else{
    Color selectedColor = menu.pendingCalibrationD==PendingCalibrationD::LIGHT_BLUE?Color::LIGHT_BLUE:
                           menu.pendingCalibrationD==PendingCalibrationD::ORANGE?Color::ORANGE:
                           menu.pendingCalibrationD==PendingCalibrationD::PINK?Color::PINK:
                           menu.pendingCalibrationD==PendingCalibrationD::YELLOW?Color::YELLOW:
                           menu.pendingCalibrationD==PendingCalibrationD::GREEN?Color::GREEN:
                           menu.pendingCalibrationD==PendingCalibrationD::RED?Color::RED:
                           menu.pendingCalibrationD==PendingCalibrationD::BLACK?Color::BLACK:
                           menu.pendingCalibrationD==PendingCalibrationD::DARK_BLUE?Color::DARK_BLUE:
                           menu.pendingCalibrationD==PendingCalibrationD::PURPLE?Color::PURPLE:
                           menu.pendingCalibrationD==PendingCalibrationD::WHITE?Color::WHITE:
                           Color::UNKNOWN;

    sensorD->calibrateColor(selectedColor); //safety
    
  }
  menu.pendingCalibrationD = PendingCalibrationD::NONE;
  menu.render();
}
  }



