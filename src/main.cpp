/*
TODO:
- when we have multiple rings and sensors, we'll replace each number in the menu grid with the assigned letter.
The current working letter will be in the top left color, aux button will cycle through them.

- Way down the road, we need to work in a way to auto-calibrate, or at least speed up the process.
Maybe a menu called "calibrate colors" and then have calibrate orange, calibrate blue, etc.
Also will need a menu functionality that just displays the current color seen.
*/
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "MenuManager.h"
#include "PinDefinitions.h"
#include "ColorHelper.h"
#include "ScaleManager.h"
#include "ColorEnum.h"
#include <MIDI.h>
#include "SystemConfig.h"

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
ButtonHelper conBtn(CON_BTN);
ButtonHelper bakBtn(BAK_BTN);
ButtonHelper panicBtn(PANIC_BTN);

// Script States
Color currentColorA = Color::UNKNOWN;

//function prototypes
MenuButton readButtons();
MenuButton readEncoder();

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

void setup() {
  Serial.begin(115200);
  Serial.println("Starting up...");

  Serial.println("Setting up MIDI...");
  MIDIserial.begin(MIDI_BAUD_RATE, SERIAL_8N1, MIDI_IN_PIN, MIDI_OUT_PIN);
  
  // Initialize I2C for OLED display
  Serial.println("Initializing I2C for display...");
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println("I2C initialized");
  

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

  delay(200);
  
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
  
  // Set up MIDI callback for MenuManager
  menu.setAllNotesOffCallback(sendAllNotesOff);
  
  // Initial menu render
  menu.render();
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

  //panic button handling
  if (panicBtn.isPressed()){
    Serial.println("Panic!");
    midiPanic();
    resetOLED();
  }

  // Periodic color detection
  if (currentTime - lastColorTime >= colorInterval) {
    if (colorSensor.isAvailable()) {
      // Use efficient enum-based color detection
      Color detectedColor = colorSensor.getCurrentColorEnum();
      
      if (detectedColor != Color::UNKNOWN && detectedColor != currentColorA) {
        Serial.print("Detected color: ");
        Serial.println(colorToString(detectedColor));
        
        uint8_t oldMidiNote = currentColorA != Color::UNKNOWN ? scaleManager.colorToMIDINote(currentColorA) : ScaleManager::MIDI_NOTE_OFF;
        MIDI.sendNoteOff(oldMidiNote, 0, menu.activeMIDIChannelA); // Note off on channel 1
        // Generate MIDI note based on detected color (efficient!)
        uint8_t newMidiNote = scaleManager.colorToMIDINote(detectedColor);
        byte currentChannelA;
        if(detectedColor==Color::WHITE){
          currentChannelA=0;
        }
        else{
          currentChannelA = menu.activeMIDIChannelA;
        }
        
        Serial.print("MIDI Note: ");
        Serial.print(newMidiNote);
        Serial.print(" @ Channel");
        Serial.println(currentChannelA);
        MIDI.sendNoteOn(newMidiNote, menu.velocityA, currentChannelA); // Note on with velocity 127 on channel 1

        // Update menu with current color and MIDI note for troubleshoot display
        menu.updateCurrentColor(colorToString(detectedColor));
        menu.updateCurrentMIDINote(newMidiNote);
        
        // Re-render if we're in troubleshoot menu to show updated info
        if (menu.currentMenu == TROUBLESHOOT_MENU) {
          menu.render();
        }
        
        // TODO: Send actual MIDI message here
        // For example: sendMIDINote(midiNote, velocity);
        currentColorA = detectedColor;
      }
      else if (detectedColor == Color::UNKNOWN) {
        Serial.println("No valid color detected");
      } 
    }
    
    lastColorTime = currentTime;
  }
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
  if (conBtn.isPressed()) {
    Serial.println("Aux button pressed");
    return CON_BUTTON;
  }
  
  // Check back button
  if (bakBtn.isPressed()) {
    Serial.println("Back button pressed");
    return BAK_BUTTON;
  }
  
  return BUTTON_NONE;
}
