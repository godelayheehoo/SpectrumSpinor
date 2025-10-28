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
    
    // Active MIDI channel (1-16)
    int activeMIDIChannelA = 3; // Default to channel 3

    byte velocityA = 127; // Default velocity for notes
    
    // Current detected color for troubleshoot menu
    String currentDetectedColorA = "unknown";
    uint8_t currentMIDINoteA = 60;
    
    // Update current color and MIDI note (called from main.cpp)
    void updateCurrentColorA(const char* color);
    void updateCurrentMIDINoteA(uint8_t midiNote);
    
private:
    // Callback for sending ALL NOTES OFF messages
    AllNotesOffCallback allNotesOffCallback = nullptr;

};