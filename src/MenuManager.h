#pragma once
#include <Adafruit_SH110X.h>
#include "SystemConfig.h"

class MenuManager;

// Table-driven menu action handler types
typedef void (MenuManager::*MenuActionHandler)();

struct MenuHandlers {
    MenuActionHandler onCW;
    MenuActionHandler onCCW;
    MenuActionHandler onEncoderButton;
    MenuActionHandler onConButton;
    MenuActionHandler onBackButton;
};

// Handler table declaration
extern const MenuHandlers menuHandlersTable[];

enum MenuState {
    MAIN_MENU,
    MIDI_GRID_MENU,
    TROUBLESHOOT_MENU
};

enum MenuButton {
    BUTTON_NONE,
    CW,
    CCW,
    ENCODER_BUTTON,
    CON_BUTTON,
    BAK_BUTTON
};

// Function pointer type for MIDI ALL NOTES OFF callback
typedef void (*AllNotesOffCallback)(uint8_t channel);

class MenuManager {
public:
    MenuManager(Adafruit_SH1106G& display);
    void render();
    void handleInput(MenuButton btn);
    
    // Set callback for sending ALL NOTES OFF
    void setAllNotesOffCallback(AllNotesOffCallback callback);
    
    // Text centering helper functions
    void centerTextAt(int y, String text, int textSize = 2);
    void centerTextInContent(String text, int textSize = 2);
    
    // Handler functions for MAIN_MENU
    void mainMenuCW();
    void mainMenuCCW();
    void mainMenuEncoderButton();
    void mainMenuConButton();
    void mainMenuBackButton();
    
    // Handler functions for GRID_MENU
    void gridMenuCW();
    void gridMenuCCW();
    void gridMenuEncoderButton();
    void gridMenuConButton();
    void gridMenuBackButton();
    
    // Handler functions for TROUBLESHOOT_MENU
    void troubleshootMenuCW();
    void troubleshootMenuCCW();
    void troubleshootMenuEncoderButton();
    void troubleshootMenuConButton();
    void troubleshootMenuBackButton();
    


    MenuState currentMenu;
    Adafruit_SH1106G& display;
    
    // Main menu selection
    int mainMenuSelectedIdx = 0;
    int mainMenuScrollIdx = 0;
    static const int MAIN_MENU_VISIBLE_ITEMS = 5;
    
    // Grid menu selection (1-16 = numbers, no "..." anymore)
    int gridSelectedIdx = 1;
    
    // Active MIDI channels (1-16) for each sensor
    int activeMIDIChannelA = 3; // Default to channel 3
    int activeMIDIChannelB = 2; // Default to channel 2
    int activeMIDIChannelC = 7; // Default to channel 7
    int activeMIDIChannelD = 12; // Default to channel 12

    byte velocityA = 127; // Default velocity for notes
    byte velocityB = 127; // Default velocity for notes
    byte velocityC = 127; // Default velocity for notes
    byte velocityD = 127; // Default velocity for notes
    
    // Current detected colors for troubleshoot menu
    String currentDetectedColorA = "unknown";
    String currentDetectedColorB = "unknown";
    String currentDetectedColorC = "unknown";
    String currentDetectedColorD = "unknown";
    
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
    
private:
    // Callback for sending ALL NOTES OFF messages
    AllNotesOffCallback allNotesOffCallback = nullptr;

};