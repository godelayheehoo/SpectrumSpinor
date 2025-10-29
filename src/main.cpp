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
#include <ESP32Encoder.h>
#include <EEPROM.h>
#include "EEPROMAddresses.h"

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
ColorHelper colorHelperA(true); // Enable normalization
ColorHelper colorHelperB(true);
ColorHelper colorHelperC(true);
ColorHelper colorHelperD(true);

ColorHelper colorHelpers[4]{colorHelperA, colorHelperB, colorHelperC, colorHelperD};
ColorHelper* activeColorSensor = nullptr;

SensorCalibration sensorCalibrations[4];

// Scale manager setup
ScaleManager scaleManager(ScaleManager::MAJOR, 4, 60); // Major scale, octave 4, root C4

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
MenuButton readButtons();
MenuButton readEncoder();
void calibrateColor(Color color);

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
  bool calibrationValid = EEPROM.read(0)==EEPROM_MAGIC_VALUE;

  for (int i = 0; i < 4; i++) {
        if (calibrationValid) {
            // Load calibration from EEPROM
            int addr = 0;
            switch (i) {
                case 0: addr = SENSOR_A_CALIBRATION; break;
                case 1: addr = SENSOR_B_CALIBRATION; break;
                case 2: addr = SENSOR_C_CALIBRATION; break;
                case 3: addr = SENSOR_D_CALIBRATION; break;
            }
            EEPROM.get(addr, sensorCalibrations[i]);
            // Set calibration in ColorHelper
            colorHelpers[i].setColorDatabase(sensorCalibrations[i].colorDatabase, sensorCalibrations[i].numColors);
        } else {
            // Use defaults (already set in your code, or copy defaultColors if needed)
            colorHelpers[i].setColorDatabase(defaultColors, 9);
        }
        tcaSelect(i);
        delay(50);
        // Always begin the sensor
        colorHelpers[i].begin();
    }

  // Disable all channels for now
  tcaDisableAll();

  //load menu values
  if(calibrationValid){
    EEPROM.get(ACTIVE_MIDI_CHANNEL_A, menu.activeMIDIChannelA);
    EEPROM.get(ACTIVE_MIDI_CHANNEL_B, menu.activeMIDIChannelB);
    EEPROM.get(ACTIVE_MIDI_CHANNEL_C, menu.activeMIDIChannelC);
    EEPROM.get(ACTIVE_MIDI_CHANNEL_D, menu.activeMIDIChannelD);

    EEPROM.get(OCTAVE_A, menu.octaveA);
    EEPROM.get(OCTAVE_B, menu.octaveB);
    EEPROM.get(OCTAVE_C, menu.octaveC);
    EEPROM.get(OCTAVE_D, menu.octaveD);

  }
  else{
    Serial.println("EEPROM not valid, using default values");
    menu.activeMIDIChannelA = 3; // Default to channel 3
    menu.activeMIDIChannelB = 2; // Default to channel 2
    menu.activeMIDIChannelC = 7; // Default to channel 7
    menu.activeMIDIChannelD = 12;// Default to channel 12

    menu.octaveA = 1;
    menu.octaveB = 3;
    menu.octaveC = 4;
    menu.octaveD = 6;
  }
  
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
  const unsigned long colorInterval = 50; // Much faster color checking (50ms)
  const unsigned long settleTime = 1; // Minimal settle time

  EEPROM.begin(1024); // Initialize EEPROM with 1KB size
  
  unsigned long currentTime = millis();
  
  //check encoder and pass in turns if turns!=0
  int newEncoderPos =  enc.getCount();
  //temp
  //remove baove
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
      
      uint16_t r, g, b, c;
      activeColorSensor->getRawData(&r, &g, &b, &c);
      
      // Update RGB values in menu
      switch (sensorIdx) {
        case 0: menu.updateCurrentRGBA(r, g, b, c); break;
        case 1: menu.updateCurrentRGBB(r, g, b, c); break;
        case 2: menu.updateCurrentRGBC(r, g, b, c); break;
        case 3: menu.updateCurrentRGBD(r, g, b, c); break;
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
      tcaSelect(currentSensorIndex);
      lastSensorSettleTime = currentTime;
      sensorSettling = true;
      return; // Exit loop to allow other operations
    }
    
    // Check if sensor has settled
    if (currentTime - lastSensorSettleTime >= settleTime) {
      // Process current sensor
      if (activeColorSensor->isAvailable()) {
        Color detectedColor = activeColorSensor->getCurrentColorEnum();
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
          uint16_t r, g, b, c;
          activeColorSensor->getRawData(&r, &g, &b, &c);
          
          
          // Update RGB values in menu for troubleshoot mode
          switch (currentSensorIndex) {
            case 0: menu.updateCurrentRGBA(r, g, b, c); break;
            case 1: menu.updateCurrentRGBB(r, g, b, c); break;
            case 2: menu.updateCurrentRGBC(r, g, b, c); break;
            case 3: menu.updateCurrentRGBD(r, g, b, c); break;
          }
        }
        
        
        // Process color change if detected and valid
        if (detectedColor != Color::UNKNOWN && detectedColor != *currentColorPtr && currentColorPtr != nullptr) {
         
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
          uint8_t newMidiNote = scaleManager.colorToMIDINote(detectedColor);
          // Adjust based on octave
          uint8_t offset;
          switch (currentSensorIndex){
            case 0:
              offset = (menu.octaveA - 4) * 12;
              newMidiNote += offset;
              lastNoteA = newMidiNote;
              break;
            case 1:
              offset = (menu.octaveB - 4) * 12;
              newMidiNote += offset;
              lastNoteB = newMidiNote;
              break;  
            case 2:
              offset = (menu.octaveC - 4) * 12;
              newMidiNote += offset;
              lastNoteC = newMidiNote;
              break;
            case 3:
              offset = (menu.octaveD - 4) * 12;
              newMidiNote += offset;
              lastNoteD = newMidiNote;
              break;
          }

   

          byte currentChannel = (detectedColor == Color::WHITE) ? 0 : activeMIDIChannel;
          MIDI.sendNoteOn(newMidiNote, velocity, currentChannel);

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

  //check if we're calibrating
  if(menu.pendingCalibrationA!=PendingCalibrationA::NONE){
    //unfinished: white balance
    menu.startCalibrationCountdown();
    Serial.println("A is pending and unsupported");
    //there has to be a better way to convert between the two. We'll have to add black though.
    //for now, need to check that the enums line up right and then add special case handling.
    calibrateColor(indexToColor(static_cast<uint8_t>(menu.pendingCalibrationA)-1));
    menu.render();

    menu.pendingCalibrationA = PendingCalibrationA::NONE;
    //handle pending calibration A
  }
}

// Legacy function - now unused since we use interrupts
// Keeping for compatibility but it just returns BUTTON_NONE
MenuButton readEncoder() {
  return BUTTON_NONE;
}

// Legacy function - now mostly unused since buttons use interrupts
// Only panic button still uses polling for safety
MenuButton readButtons() {
  // Panic button stays as polling since it's critical and has hardware debouncing
  // Other buttons now handled by interrupts
  return BUTTON_NONE;
}

void calibrateColor(Color color){
  for (int i=0; i<NUM_CALIBRATION_STEPS; i++){
    Serial.print("Calibration step ");
    Serial.print(i);
    Serial.print(" color# ");
    Serial.println(static_cast<uint8_t>(color));
  }
}