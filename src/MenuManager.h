#pragma once
#include <TFT_eSPI.h>

class MenuManager;

// Table-driven menu action handler types
typedef void (MenuManager::*MenuActionHandler)();

struct MenuHandlers {
    MenuActionHandler onCW;
    MenuActionHandler onCCW;
    MenuActionHandler onEncoderButton;
    MenuActionHandler onAuxButton;
};

// Handler table declaration
extern const MenuHandlers menuHandlersTable[];

enum MenuState {
    MAIN_MENU,
    MIDI_GRID_MENU
};

enum MenuButton {
    BUTTON_NONE,
    CW,
    CCW,
    ENCODER_BUTTON,
    AUX_BUTTON
};

class MenuManager {
public:
    MenuManager(TFT_eSPI& display);
    void render();
    void handleInput(MenuButton btn);
    
    // Text centering helper functions
    void centerTextAt(int y, String text, int textSize = 2);
    void centerTextInContent(String text, int textSize = 2);
    
    // Handler functions for MAIN_MENU
    void mainMenuCW();
    void mainMenuCCW();
    void mainMenuEncoderButton();
    void mainMenuAuxButton();
    
    // Handler functions for GRID_MENU
    void gridMenuCW();
    void gridMenuCCW();
    void gridMenuEncoderButton();
    void gridMenuAuxButton();
    


    MenuState currentMenu;
    TFT_eSPI& tft;
    
    // Main menu selection
    int mainMenuSelectedIdx = 0;
    int mainMenuScrollIdx = 0;
    static const int MAIN_MENU_VISIBLE_ITEMS = 5;
    
    // Grid menu selection (0 = "...", 1-16 = numbers)
    int gridSelectedIdx = 0;
    
    // Active MIDI channel (1-16)
    int activeMIDIChannelA = 1;
    

};