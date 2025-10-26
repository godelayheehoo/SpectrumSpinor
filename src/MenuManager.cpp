#include "MenuManager.h"

// Screen constants
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MARGIN_TOP 10
#define MARGIN_LEFT 10
#define MARGIN_BOTTOM 10
#define USABLE_WIDTH (SCREEN_WIDTH - 2 * MARGIN_LEFT)
#define USABLE_HEIGHT (SCREEN_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM)
#define CENTER_X(width) (MARGIN_LEFT + (USABLE_WIDTH - width) / 2)
#define BOTTOM_LINE (SCREEN_HEIGHT - MARGIN_BOTTOM)
#define LARGE_TEXT_SIZE 3
#define SAFE_VISIBLE_OPTIONS 5

const MenuHandlers menuHandlersTable[] = {
    { &MenuManager::mainMenuCW, &MenuManager::mainMenuCCW, &MenuManager::mainMenuEncoderButton, &MenuManager::mainMenuAuxButton }, // MAIN_MENU
    { &MenuManager::gridMenuCW, &MenuManager::gridMenuCCW, &MenuManager::gridMenuEncoderButton, &MenuManager::gridMenuAuxButton }, // GRID_MENU
    { &MenuManager::troubleshootMenuCW, &MenuManager::troubleshootMenuCCW, &MenuManager::troubleshootMenuEncoderButton, &MenuManager::troubleshootMenuAuxButton }, // TROUBLESHOOT_MENU
};

MenuManager::MenuManager(TFT_eSPI& display) : tft(display), currentMenu(MAIN_MENU) {
}

void MenuManager::updateCurrentColor(const char* color) {
    currentDetectedColor = String(color);
}

void MenuManager::updateCurrentMIDINote(uint8_t midiNote) {
    currentMIDINote = midiNote;
}

// Text centering helper functions
void MenuManager::centerTextAt(int y, String text, int textSize) {
    tft.setTextSize(textSize);
    
    // Get text dimensions using TFT_eSPI API
    int w = tft.textWidth(text);
    int h = tft.fontHeight();
    
    // Calculate center X position
    int x = CENTER_X(w);
    
    tft.setCursor(x, y);
    tft.print(text);
}

void MenuManager::centerTextInContent(String text, int textSize) {
    tft.setTextSize(textSize);
    
    // Get text dimensions using TFT_eSPI API
    int w = tft.textWidth(text);
    int h = tft.fontHeight();
    
    // Calculate center position within usable content area
    int x = MARGIN_LEFT + (USABLE_WIDTH - w) / 2;
    int y = MARGIN_TOP + (USABLE_HEIGHT - h) / 2;
    
    tft.setCursor(x, y);
    tft.print(text);
}

void MenuManager::handleInput(MenuButton btn) {
    // Map button to handler index
    int handlerIdx = -1;
    switch (btn) {
        case CW: handlerIdx = 0; break;
        case CCW: handlerIdx = 1; break;
        case ENCODER_BUTTON: handlerIdx = 2; break;
        case AUX_BUTTON: handlerIdx = 3; break;
        default: return; // Unknown button
    }
    
    // Call the handler from the table
    if (currentMenu >= 0 && currentMenu < (sizeof(menuHandlersTable)/sizeof(menuHandlersTable[0]))) {
        MenuActionHandler handler = nullptr;
        switch (handlerIdx) {
            case 0: handler = menuHandlersTable[currentMenu].onCW; break;
            case 1: handler = menuHandlersTable[currentMenu].onCCW; break;
            case 2: handler = menuHandlersTable[currentMenu].onEncoderButton; break;
            case 3: handler = menuHandlersTable[currentMenu].onAuxButton; break;
            default: handler = nullptr; break;
        }
        if (handler) {
            (this->*handler)();
        }
    }
}

void MenuManager::render() {
    if (currentMenu == MAIN_MENU) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(10, 10);
        tft.print("Main Menu");
        
        // Menu item 1: Grid View
        tft.setCursor(20, 50);
        if (mainMenuSelectedIdx == 0) {
            tft.setTextColor(TFT_BLACK, TFT_WHITE);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        tft.print("Grid View");
        
        // Menu item 2: Troubleshoot
        tft.setCursor(20, 80);
        if (mainMenuSelectedIdx == 1) {
            tft.setTextColor(TFT_BLACK, TFT_WHITE);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        tft.print("Troubleshoot");
        
    } else if (currentMenu == MIDI_GRID_MENU) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        
        // "..." in top left corner with highlighting
        tft.setCursor(10, 10);
        if (gridSelectedIdx == 0) {
            tft.setTextColor(TFT_BLACK, TFT_WHITE); // Inverted colors for highlight
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        tft.print("...");
        
        // Draw 4x4 grid of numbers 1-16
        int gridStartX = 50;
        int gridStartY = 50;
        int cellWidth = 60;
        int cellHeight = 40;
        
        for (int i = 0; i < 16; i++) {
            int row = i / 4;
            int col = i % 4;
            int x = gridStartX + col * cellWidth;
            int y = gridStartY + row * cellHeight;
            
            bool isSelected = (gridSelectedIdx == i + 1);
            bool isActiveChannel = (activeMIDIChannelA == i + 1);
            
            // Draw cell with highlighting
            if (isSelected) {
                // Fill background for selected cell (white when highlighted)
                tft.fillRect(x, y, cellWidth, cellHeight, TFT_WHITE);
                tft.setTextColor(TFT_BLACK);
            } else if (isActiveChannel) {
                // Fill background for active channel (green when not highlighted)
                tft.fillRect(x, y, cellWidth, cellHeight, TFT_GREEN);
                tft.setTextColor(TFT_BLACK);
            } else {
                // Normal cell
                tft.drawRect(x, y, cellWidth, cellHeight, TFT_WHITE);
                tft.setTextColor(TFT_WHITE);
            }
            
            // Draw number centered in cell
            String numStr = String(i + 1);
            int w = tft.textWidth(numStr);
            int h = tft.fontHeight();
            
            int textX = x + (cellWidth - w) / 2;
            int textY = y + (cellHeight - h) / 2;
            
            tft.setCursor(textX, textY);
            tft.print(numStr);
        }
    } else if (currentMenu == TROUBLESHOOT_MENU) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        
        // "..." in top left corner
        tft.setCursor(10, 10);
        tft.setTextColor(TFT_BLACK, TFT_WHITE); // Always highlighted for return
        tft.print("...");
        
        // Draw quadrant dividers
        int centerX = 320 / 2;
        int centerY = 240 / 2;
        tft.drawLine(centerX, 40, centerX, 240, TFT_WHITE); // Vertical divider
        tft.drawLine(0, centerY, 320, centerY, TFT_WHITE);  // Horizontal divider
        
        // Quadrant 1 (top-left): Current Color and MIDI Note
        tft.setTextSize(1);
        tft.setTextColor(TFT_YELLOW);
        tft.setCursor(10, 50);
        tft.print("Color:");
        tft.setTextSize(2);
        tft.setTextColor(TFT_GREEN);
        tft.setCursor(10, 65);
        tft.print(currentDetectedColor);
        
        tft.setTextSize(1);
        tft.setTextColor(TFT_YELLOW);
        tft.setCursor(10, 90);
        tft.print("MIDI Note:");
        tft.setTextSize(2);
        tft.setTextColor(TFT_CYAN);
        tft.setCursor(10, 105);
        tft.print(currentMIDINote);
        
        // Quadrant 2 (top-right): Reserved for future use
        tft.setTextSize(1);
        tft.setTextColor(TFT_CYAN);
        tft.setCursor(centerX + 10, 50);
        tft.print("Quad 2:");
        tft.setCursor(centerX + 10, 70);
        tft.print("Reserved");
        
        // Quadrant 3 (bottom-left): Reserved for future use
        tft.setTextColor(TFT_MAGENTA);
        tft.setCursor(10, centerY + 10);
        tft.print("Quad 3:");
        tft.setCursor(10, centerY + 30);
        tft.print("Reserved");
        
        // Quadrant 4 (bottom-right): Reserved for future use
        tft.setTextColor(TFT_RED);
        tft.setCursor(centerX + 10, centerY + 10);
        tft.print("Quad 4:");
        tft.setCursor(centerX + 10, centerY + 30);
        tft.print("Reserved");
    }
}

// Handler functions for each menu

// MAIN_MENU
void MenuManager::mainMenuCW() {
    if (mainMenuSelectedIdx < 1) { // Now we have 2 items (0-1)
        mainMenuSelectedIdx++;
        if (mainMenuSelectedIdx > mainMenuScrollIdx + MAIN_MENU_VISIBLE_ITEMS - 1) {
            mainMenuScrollIdx = mainMenuSelectedIdx - MAIN_MENU_VISIBLE_ITEMS + 1;
        }
    }
}

void MenuManager::mainMenuCCW() {
    if (mainMenuSelectedIdx > 0) { // Navigate up/back
        mainMenuSelectedIdx--;
        if (mainMenuSelectedIdx < mainMenuScrollIdx) {
            mainMenuScrollIdx = mainMenuSelectedIdx;
        }
    }
}

void MenuManager::mainMenuEncoderButton() {
    if (mainMenuSelectedIdx == 0) {
        currentMenu = MIDI_GRID_MENU;
        gridSelectedIdx = 0; // Start with "..." highlighted
    } else if (mainMenuSelectedIdx == 1) {
        currentMenu = TROUBLESHOOT_MENU;
    }
}

void MenuManager::mainMenuAuxButton() {
    // Secondary action - to be implemented based on needs
}

// GRID_MENU
void MenuManager::gridMenuCW() {
    // Navigate forward: 0("...") -> 1 -> 2 -> ... -> 16 -> 0("...")
    gridSelectedIdx = (gridSelectedIdx + 1) % 17;
}

void MenuManager::gridMenuCCW() {
    // Navigate backward: 0("...") -> 16 -> 15 -> ... -> 1 -> 0("...")
    gridSelectedIdx = (gridSelectedIdx - 1 + 17) % 17;
}

void MenuManager::gridMenuEncoderButton() {
    // If a MIDI channel is selected (not "..."), set it as active
    if (gridSelectedIdx >= 1 && gridSelectedIdx <= 16) {
        activeMIDIChannelA = gridSelectedIdx;
    } else if (gridSelectedIdx == 0) {
        // If "..." is selected, return to main menu
        currentMenu = MAIN_MENU;
    }
}

void MenuManager::gridMenuAuxButton() {
    // Return to main menu
    currentMenu = MAIN_MENU;
}

// TROUBLESHOOT_MENU
void MenuManager::troubleshootMenuCW() {
    // No navigation in troubleshoot menu
}

void MenuManager::troubleshootMenuCCW() {
    // No navigation in troubleshoot menu
}

void MenuManager::troubleshootMenuEncoderButton() {
    // "..." is always selected, return to main menu
    currentMenu = MAIN_MENU;
}

void MenuManager::troubleshootMenuAuxButton() {
    // Return to main menu
    currentMenu = MAIN_MENU;
}

