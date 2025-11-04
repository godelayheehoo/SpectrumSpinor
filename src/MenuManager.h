#pragma once
#include <Adafruit_SH110X.h>
#include "SystemConfig.h"
#include "ScaleManager.h"

class MenuManager;

// Table-driven menu action handler types
typedef void (MenuManager::*MenuActionHandler)();
typedef void (MenuManager::*MenuEncoderHandler)(int turns);

struct MenuHandlers {
    MenuEncoderHandler onEncoder;
    MenuActionHandler onEncoderButton;
    MenuActionHandler onConButton;
    MenuActionHandler onBackButton;
};

// Handler table declaration
extern const MenuHandlers menuHandlersTable[];

enum MenuState {
    MAIN_MENU,
    MIDI_GRID_MENU,
    TROUBLESHOOT_MENU,
    CALIBRATION_MENU,
    OCTAVE_MENU, 
    CALIBRATION_A_MENU,
    CALIBRATION_B_MENU,
    CALIBRATION_C_MENU,
    CALIBRATION_D_MENU
};
static const int NUM_MAIN_MENU_ITEMS = 5;

enum MenuButton {
    BUTTON_NONE,
    CW,
    CCW,
    ENCODER_BUTTON,
    CON_BUTTON,
    BAK_BUTTON
};

enum ActiveSensor {
    SENSOR_A,
    SENSOR_B,
    SENSOR_C,
    SENSOR_D
};
// {"Dark Offset", "Gains", "LightBlue","Orange", "Pink", "Yellow", "Green", "Red", "Black","DarkBlue", "Purple", "White"};
    
enum class PendingCalibrationA{
    NONE,
    DARK_OFFSET,
    GAINS,
    LIGHT_BLUE,
    ORANGE,
    PINK,
    YELLOW,
    GREEN,
    RED,
    BLACK,
    DARK_BLUE,
    PURPLE,
    WHITE,
    RESET_DEFAULTS
};
enum class PendingCalibrationB{
     NONE,
    DARK_OFFSET,
    GAINS,
    LIGHT_BLUE,
    ORANGE,
    PINK,
    YELLOW,
    GREEN,
    RED,
    BLACK,
    DARK_BLUE,
    PURPLE,
    WHITE,
    RESET_DEFAULTS
};
enum class PendingCalibrationC{
      NONE,
    DARK_OFFSET,
    GAINS,
    LIGHT_BLUE,
    ORANGE,
    PINK,
    YELLOW,
    GREEN,
    RED,
    BLACK,
    DARK_BLUE,
    PURPLE,
    WHITE,
    RESET_DEFAULTS
};
enum class PendingCalibrationD{
    NONE,
    DARK_OFFSET,
    GAINS,
    LIGHT_BLUE,
    ORANGE,
    PINK,
    YELLOW,
    GREEN,
    RED,
    BLACK,
    DARK_BLUE,
    PURPLE,
    WHITE,
    RESET_DEFAULTS
};

// Function pointer type for MIDI ALL NOTES OFF callback
typedef void (*AllNotesOffCallback)(uint8_t channel);

class MenuManager {
public:
    MenuManager(Adafruit_SH1106G& display);
    void render();
    void handleInput(MenuButton btn);
    void handleEncoder(int turns);

    // RGB update flag for troubleshoot mode
    bool requestRGBUpdate = true;

    // Set callback for sending ALL NOTES OFF
    void setAllNotesOffCallback(AllNotesOffCallback callback);

    // Text centering helper functions
    void centerTextAt(int y, String text, int textSize = 2);
    void centerTextInContent(String text, int textSize = 2);
    
    // Handler functions for MAIN_MENU
    void mainMenuEncoder(int turns);
    void mainMenuEncoderButton();
    void mainMenuConButton();
    void mainMenuBackButton();
    
    // Handler functions for GRID_MENU
    void gridMenuEncoder(int turns);
    void gridMenuEncoderButton();
    void gridMenuConButton();
    void gridMenuBackButton();
    void saveMIDIGrid();
    
    // Handler functions for TROUBLESHOOT_MENU
    void troubleshootMenuEncoder(int turns);
    void troubleshootMenuEncoderButton();
    void troubleshootMenuConButton();
    void troubleshootMenuBackButton();
    
    // Handler functions for CALIBRATION_MENU
    void calibrationMenuEncoder(int turns);
    void calibrationMenuEncoderButton();
    void calibrationMenuConButton();
    void calibrationMenuBackButton();

    void calibrationMenuAEncoder(int turns);
    void calibrationMenuAEncoderButton();
    void calibrationMenuAConButton();
    void calibrationMenuABackButton();

    void calibrationMenuBEncoder(int turns);
    void calibrationMenuBEncoderButton();
    void calibrationMenuBConButton();
    void calibrationMenuBBackButton();


    void calibrationMenuCEncoder(int turns);
    void calibrationMenuCEncoderButton();
    void calibrationMenuCConButton();
    void calibrationMenuCBackButton();

    void calibrationMenuDEncoder(int turns);
    void calibrationMenuDEncoderButton();
    void calibrationMenuDConButton();
    void calibrationMenuDBackButton();
    
    // Handler functions for OCTAVE_MENU
    void octaveMenuEncoder(int turns);
    void octaveMenuEncoderButton();
    void octaveMenuConButton();
    void octaveMenuBackButton();   
    void saveOctaves();

    MenuState currentMenu;
    Adafruit_SH1106G& display;
    
    // Main menu selection
    int mainMenuSelectedIdx = 0;
    int mainMenuScrollIdx = 0;
    static const int MAIN_MENU_VISIBLE_ITEMS = 5;
    
    // Grid menu selection (1-16 = numbers, no "..." anymore)
    int gridSelectedIdx = 1;
    
    // Active MIDI Grid Sensor (A, B, C, or D) - which sensor we're configuring
    ActiveSensor activeMIDIGridSensor = SENSOR_A; // Default to sensor A
    
    // Calibration menu selection (0-3 = A, B, C, D)
    int calibrationSelectedIdx = 0; // Default to sensor A
    
    // Active MIDI channels (1-16) for each sensor
    byte activeMIDIChannelA = 3; // Default to channel 3
    byte activeMIDIChannelB = 2; // Default to channel 2
    byte activeMIDIChannelC = 7; // Default to channel 7
    byte activeMIDIChannelD = 12; // Default to channel 12

    byte velocityA = 127; // Default velocity for notes
    byte velocityB = 127; // Default velocity for notes
    byte velocityC = 127; // Default velocity for notes
    byte velocityD = 127; // Default velocity for notes
    
    // Troubleshoot mode: 0 = color names, 1 = RGB values
    int troubleshootMode = 0;
    
    // Current detected colors for troubleshoot menu
    String currentDetectedColorA = "unknown";
    String currentDetectedColorB = "unknown";
    String currentDetectedColorC = "unknown";
    String currentDetectedColorD = "unknown";
    
    // Current RGB values for troubleshoot menu (mode 1)
    uint16_t currentRGBA[4] = {0}; // R,G,B,C for sensor A
    uint16_t currentRGBB[4] = {0}; // R,G,B,C for sensor B  
    uint16_t currentRGBC[4] = {0}; // R,G,B,C for sensor C
    uint16_t currentRGBD[4] = {0}; // R,G,B,C for sensor D
    
    // Current MIDI notes for each sensor
    uint8_t currentMIDINoteA = 60;
    uint8_t currentMIDINoteB = 60;
    uint8_t currentMIDINoteC = 60;
    uint8_t currentMIDINoteD = 60;

    
    // Update current color and MIDI note (called from main.cpp)
    void updateCurrentColorA(const char* color);
    void updateCurrentColorB(const char* color);
    void updateCurrentColorC(const char* color);
    void updateCurrentColorD(const char* color);
    
    void updateCurrentMIDINoteA(uint8_t midiNote);
    void updateCurrentMIDINoteB(uint8_t midiNote);
    void updateCurrentMIDINoteC(uint8_t midiNote);
    void updateCurrentMIDINoteD(uint8_t midiNote);
    
    // Update RGB values for troubleshoot mode 1
    void updateCurrentRGBA(uint16_t r, uint16_t g, uint16_t b, uint16_t c);
    void updateCurrentRGBB(uint16_t r, uint16_t g, uint16_t b, uint16_t c);
    void updateCurrentRGBC(uint16_t r, uint16_t g, uint16_t b, uint16_t c);
    void updateCurrentRGBD(uint16_t r, uint16_t g, uint16_t b, uint16_t c);

    uint8_t octaveA; // Current octave for sensor A
    uint8_t octaveB; // Current octave for sensor B
    uint8_t octaveC; // Current octave for sensor C
    uint8_t octaveD; // Current octave for sensor D

    int activeOctaveSensor = SENSOR_A; //0: A, 1: B, 2: C, 3: D
    
    // Helper functions for working with active sensor
    byte* getActiveSensorMIDIChannel(); // Returns pointer to the active sensor's MIDI channel
    void setActiveSensorMIDIChannel(int channel); // Sets the MIDI channel for the active sensor
    
    int calibrationMenuSelectedIdx = 0; //there will only be four so this is fine.

    static const int CALIBRATION_SUBMENU_VISIBLE_ITEMS = 5;
    static const int CALIBRATION_SUBMENU_TOTAL_ITEMS = 13;
    int calibrationMenuASelectedIdx = 0;
    int calibrationMenuAScrollIdx = 0;
    int calibrationMenuBSelectedIdx = 0;
    int calibrationMenuBScrollIdx = 0;
    int calibrationMenuCSelectedIdx = 0;
    int calibrationMenuCScrollIdx = 0;
    int calibrationMenuDSelectedIdx = 0;
    int calibrationMenuDScrollIdx = 0;

    PendingCalibrationA pendingCalibrationA;
    PendingCalibrationB pendingCalibrationB;
    PendingCalibrationC pendingCalibrationC;
    PendingCalibrationD pendingCalibrationD;

    
    void SharedCalibrationMenuRender(int selectedIdx, int scrollIdx);
    void startCalibrationCountdown();
    void calibrationStartProgressBar(); 
    void calibrationIncrementProgressBar(uint8_t i);

    ScaleManager scaleManager = ScaleManager(ScaleManager::MAJOR, 4, 60);
private:
    // Callback for sending ALL NOTES OFF messages
    AllNotesOffCallback allNotesOffCallback = nullptr;

};