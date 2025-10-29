/*
TODO:
- when we have multiple rings and sensors, we'll replace each number in the menu grid with the assigned letter.
The current working letter will be in the top left color, aux button will cycle through them.

- Way down the road, we need to work in a way to auto-calibrate, or at least speed up the process.
Maybe a menu called "calibrate colors" and then have calibrate orange, calibrate blue, etc.
Also will need a menu functionality that just displays the current color seen.

- Improve encoder turns. 

- Forcing sensor update for troubleshoot menu 2 on startup and switch doesn't work i think.

- remove display test stuff from setup().
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
#include <Encoder.h>

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

// Interrupt-based encoder and button handling
volatile bool encoderCWFlag = false;
volatile bool encoderCCWFlag = false;
volatile bool encoderButtonFlag = false;
volatile bool conButtonFlag = false;
volatile bool backButtonFlag = false;

// Encoder state tracking for interrupts
volatile int lastEncoderA = HIGH;
volatile int lastEncoderB = HIGH;
// Removed volatile timing variables - debouncing now handled in main loop
const unsigned long encoderDebounceDelay = 5; // Moderate debounce for clean detent clicks  
const unsigned long buttonDebounceDelay = 300; // Very aggressive debouncing for buttons

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

// Interrupt Service Routines (ISRs) - Must be fast and minimal!
#ifdef TROUBLESHOOT
// Debug counter for ISR calls
volatile int encoderISRCounter = 0;
#endif

void IRAM_ATTR encoderISR() {
#ifdef TROUBLESHOOT
  encoderISRCounter++; // Count ISR calls for debugging
#endif
  
  static int lastState = 0;
  
  // Read current state of both pins
  int currentA = digitalRead(ENCODER_A);
  int currentB = digitalRead(ENCODER_B);
  
  // Encode current state as 2-bit value: B<<1 | A
  int currentState = (currentB << 1) | currentA;
  
  // Proper quadrature decoding - only trigger on valid transitions
  // Quadrature sequence: 00 -> 01 -> 11 -> 10 -> 00 (CW)
  //                      00 -> 10 -> 11 -> 01 -> 00 (CCW)
  
  int transition = (lastState << 2) | currentState;
  
  switch (transition) {
    case 0b0001: // 00->01
    case 0b0111: // 01->11  
    case 0b1110: // 11->10
    case 0b1000: // 10->00
      encoderCWFlag = true;
      break;
      
    case 0b0010: // 00->10
    case 0b1011: // 10->11
    case 0b1101: // 11->01  
    case 0b0100: // 01->00
      encoderCCWFlag = true;
      break;
      
    // Ignore invalid transitions (noise/bounce)
    default:
      break;
  }
  
  lastState = currentState;
}

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

  delay(200);
  
  // Initialize encoder pins
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  
  // Read initial encoder state
  lastEncoderA = digitalRead(ENCODER_A);
  lastEncoderB = digitalRead(ENCODER_B);
  
  // Attach interrupts to both encoder pins for proper quadrature decoding
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), encoderISR, CHANGE);
  
  // Attach interrupts for buttons (falling edge only - button press)
  attachInterrupt(digitalPinToInterrupt(ENCODER_BTN), encoderButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(CON_BTN), conButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BAK_BTN), backButtonISR, FALLING);
  
  Serial.println("Display and encoder initialized");
  
  // Initialize color sensors through I2C multiplexer
  Serial.println("Initializing color sensors on I2C multiplexer...");
  
  // Initialize all 4 sensors (channels 0-3)
  for (int channel = 0; channel < 4; channel++) {
    tcaSelect(channel);
    delay(50); // Longer settle time for initialization
    
    if (colorSensor.begin()) {
      Serial.print("Color sensor ");
      Serial.print((char)('A' + channel));
      Serial.print(" ready on mux channel ");
      Serial.println(channel);
    } else {
      Serial.print("Color sensor ");
      Serial.print((char)('A' + channel));
      Serial.print(" initialization failed on channel ");
      Serial.println(channel);
    }
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
  const unsigned long colorInterval = 50; // Much faster color checking (50ms)
  const unsigned long settleTime = 1; // Minimal settle time
  
  unsigned long currentTime = millis();
  
  // High priority: Check interrupt flags immediately (no polling delay needed!)
  
  // Check encoder flags set by interrupts with debouncing in main loop
  static unsigned long lastEncoderAction = 0;
  
  if (encoderCWFlag && (currentTime - lastEncoderAction > encoderDebounceDelay)) {
    encoderCWFlag = false;
    lastEncoderAction = currentTime;
    menu.handleInput(CW);
    menu.render();
  }
  
  if (encoderCCWFlag && (currentTime - lastEncoderAction > encoderDebounceDelay)) {
    encoderCCWFlag = false;
    lastEncoderAction = currentTime;
    menu.handleInput(CCW);
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
      colorSensor.getRawData(&r, &g, &b, &c);
      
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
          colorSensor.getRawData(&r, &g, &b, &c);
          
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
