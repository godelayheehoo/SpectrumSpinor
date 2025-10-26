/*
TODO:
- when we have multiple rings and sensors, we'll replace each number in the menu grid with the assigned letter.
The current working letter will be in the top left color, aux button will cycle through them.

- Way down the road, we need to work in a way to auto-calibrate, or at least speed up the process.
Maybe a menu called "calibrate colors" and then have calibrate orange, calibrate blue, etc.
Also will need a menu functionality that just displays the current color seen.
*/
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "MenuManager.h"
#include "PinDefinitions.h"
#include "ColorHelper.h"
#include "ScaleManager.h"

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

// Display setup - TFT_eSPI uses User_Setup.h for pin configuration
TFT_eSPI tft = TFT_eSPI();
MenuManager menu(tft);

// Color sensor setup
ColorHelper colorSensor(true); // Enable normalization

// Scale manager setup
ScaleManager scaleManager(ScaleManager::MAJOR, 4, 60); // Major scale, octave 4, root C4

// Encoder state variables
int lastEncoderA = HIGH;
int lastEncoderB = HIGH;
unsigned long lastEncoderTime = 0;
const unsigned long debounceDelay = 50;

// Button helpers
ButtonHelper encoderBtn(ENCODER_BTN);
ButtonHelper auxBtn(AUX_BTN);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting up...");
  
  // Initialize SPI with custom pins
  Serial.println("Initializing SPI...");
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);  // SCK, MISO, MOSI, SS
  Serial.println("SPI initialized");
  
  // Try different initialization approaches
  Serial.println("Trying display init approach 1...");
  tft.init();
  delay(100);
  
  // Serial.println("Trying display init approach 2...");
  // tft.init(240, 240);  // Try square format
  // delay(100);
  
  // Serial.println("Trying display init approach 3...");
  // tft.init(135, 240);  // Try different common size
  // delay(100);
  
  // Set rotation and try both inversion states
  tft.setRotation(1); // Landscape mode (320x240)
  Serial.println("Rotation set");
  
  // Try both inversion states
  Serial.println("Trying inversion false...");
  tft.invertDisplay(false);
  delay(100);
  
  Serial.println("Trying inversion true...");
  tft.invertDisplay(true);
  delay(100);
  
  Serial.println("Back to inversion false...");
  tft.invertDisplay(false);
  
  // Simple display test - fill with different colors
  Serial.println("Testing display colors...");
  tft.fillScreen(TFT_RED);
  delay(1000);
  tft.fillScreen(TFT_GREEN);
  delay(1000);
  tft.fillScreen(TFT_BLUE);
  delay(1000);
  tft.fillScreen(TFT_BLACK);
  
  // Draw a simple test pattern
  tft.drawRect(10, 10, 50, 50, TFT_WHITE);
  tft.fillCircle(160, 120, 30, TFT_YELLOW);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 200);
  tft.print("TEST");
  
  Serial.println("Display test pattern drawn");
  delay(2000);
  
  // Initialize encoder pins
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  
  // Read initial encoder state
  lastEncoderA = digitalRead(ENCODER_A);
  lastEncoderB = digitalRead(ENCODER_B);
  
  Serial.println("Display and encoder initialized");
  
  // Initialize color sensor
  Serial.println("Initializing color sensor...");
  if (colorSensor.begin()) {
    Serial.println("Color sensor ready");
  } else {
    Serial.println("Color sensor initialization failed - continuing without it");
  }
  
  // Initial menu render
  menu.render();
}

MenuButton readEncoder() {
  unsigned long currentTime = millis();
  
  // Debounce encoder rotation
  if (currentTime - lastEncoderTime < debounceDelay) {
    return BUTTON_NONE;
  }
  
  int currentA = digitalRead(ENCODER_A);
  int currentB = digitalRead(ENCODER_B);
  
  // Check for encoder rotation (state change on pin A)
  if (currentA != lastEncoderA && currentA == LOW) {
    lastEncoderTime = currentTime;
    
    if (currentB == LOW) {
      // Clockwise rotation
      lastEncoderA = currentA;
      lastEncoderB = currentB;
      Serial.println("Encoder rotated CCW");
      return CCW;
    } else {
      // Counter-clockwise rotation
      lastEncoderA = currentA;
      lastEncoderB = currentB;
      Serial.println("Encoder rotated CW");
      return CW;
    }
  }
  
  lastEncoderA = currentA;
  lastEncoderB = currentB;
  return BUTTON_NONE;
}

MenuButton readButtons() {
  // Check encoder button
  if (encoderBtn.isPressed()) {
    Serial.println("Encoder button pressed");
    return ENCODER_BUTTON;
  }
  
  // Check aux button
  if (auxBtn.isPressed()) {
    Serial.println("Aux button pressed");
    return AUX_BUTTON;
  }
  
  return BUTTON_NONE;
}

void loop() {
  static unsigned long lastPollTime = 0;
  static unsigned long lastColorTime = 0;
  const unsigned long pollInterval = 10; // Poll every 10ms
  const unsigned long colorInterval = 500; // Check color every 500ms
  
  unsigned long currentTime = millis();
  
  if (currentTime - lastPollTime >= pollInterval) {
    // Check for encoder rotation
    MenuButton encoderInput = readEncoder();
    if (encoderInput != BUTTON_NONE) {
      menu.handleInput(encoderInput);
      menu.render();
    }
    
    // Check for button presses
    MenuButton buttonInput = readButtons();
    if (buttonInput != BUTTON_NONE) {
      menu.handleInput(buttonInput);
      menu.render();
    }
    
    lastPollTime = currentTime;
  }
  
  // Periodic color detection
  if (currentTime - lastColorTime >= colorInterval) {
    if (colorSensor.isAvailable()) {
      const char* detectedColor = colorSensor.getCurrentColor();
      Serial.print("Detected color: ");
      Serial.println(detectedColor);
      
      // Generate MIDI note based on detected color
      uint8_t midiNote = scaleManager.colorToMIDINote(detectedColor);
      Serial.print("MIDI Note: ");
      Serial.println(midiNote);
      
      // Update menu with current color and MIDI note for troubleshoot display
      menu.updateCurrentColor(detectedColor);
      menu.updateCurrentMIDINote(midiNote);
      
      // Re-render if we're in troubleshoot menu to show updated info
      if (menu.currentMenu == TROUBLESHOOT_MENU) {
        menu.render();
      }
      
      // TODO: Send actual MIDI message here
      // For example: sendMIDINote(midiNote, velocity);
    }
    
    lastColorTime = currentTime;
  }
}