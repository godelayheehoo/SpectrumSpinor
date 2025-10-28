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
Color currentColorB = Color::UNKNOWN;
Color currentColorC = Color::UNKNOWN;
Color currentColorD = Color::UNKNOWN;

//function prototypes
MenuButton readButtons();
MenuButton readEncoder();

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
  
  // Initialize color sensor through I2C multiplexer
  Serial.println("Initializing color sensor on I2C multiplexer...");
  
  // Select channel 0 for sensor A
  tcaSelect(0);
  delay(10); // small settle time
  
  if (colorSensor.begin()) {
    Serial.println("Color sensor A ready on mux channel 0");
  } else {
    Serial.println("Color sensor A initialization failed - continuing without it");
  }
  
  // Disable all channels for now
  tcaDisableAll();
  
  // Set up MIDI callback for MenuManager
  menu.setAllNotesOffCallback(sendAllNotesOff);
  
  // Initial menu render
  menu.render();
}

void loop() {
  static unsigned long lastPollTime = 0;
  static unsigned long lastColorTime = 0;
  static unsigned long lastSensorSettleTime = 0;
  static uint8_t currentSensorIndex = 0;
  static bool sensorSettling = false;
  
  const unsigned long pollInterval = 5; // Poll every 5ms for better responsiveness
  const unsigned long colorInterval = 200; // Check colors more frequently (200ms)
  const unsigned long settleTime = 2; // Reduced settle time
  
  unsigned long currentTime = millis();
  
  // High priority: Always check for user input first
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
      tcaSelect(currentSensorIndex);
      lastSensorSettleTime = currentTime;
      sensorSettling = true;
      return; // Exit loop to allow other operations
    }
    
    // Check if sensor has settled
    if (currentTime - lastSensorSettleTime >= settleTime) {
      // Process current sensor
      if (colorSensor.isAvailable()) {
        Color detectedColor = colorSensor.getCurrentColorEnum();
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
        
        // Process color change if detected and valid
        if (detectedColor != Color::UNKNOWN && detectedColor != *currentColorPtr && currentColorPtr != nullptr) {
          // Send note off for previous color
          uint8_t oldMidiNote = *currentColorPtr != Color::UNKNOWN ? scaleManager.colorToMIDINote(*currentColorPtr) : ScaleManager::MIDI_NOTE_OFF;
          MIDI.sendNoteOff(oldMidiNote, 0, activeMIDIChannel);
          
          // Send note on for new color
          uint8_t newMidiNote = scaleManager.colorToMIDINote(detectedColor);
          byte currentChannel = (detectedColor == Color::WHITE) ? 0 : activeMIDIChannel;
          MIDI.sendNoteOn(newMidiNote, velocity, currentChannel);
          
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
  
  // Check con button
  if (conBtn.isPressed()) {
    Serial.println("Con button pressed");
    return CON_BUTTON;
  }
  
  // Check back button
  if (bakBtn.isPressed()) {
    Serial.println("Back button pressed");
    return BAK_BUTTON;
  }
  
  return BUTTON_NONE;
}
