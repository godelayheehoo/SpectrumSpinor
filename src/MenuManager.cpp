#include "MenuManager.h"

// Legacy screen constants (now defined in SystemConfig.h)
// Keeping these for any remaining references until layout is updated
#define MARGIN_TOP 2
#define MARGIN_LEFT 2
#define MARGIN_BOTTOM 2
#define USABLE_WIDTH (SCREEN_WIDTH - 2 * MARGIN_LEFT)
#define USABLE_HEIGHT (SCREEN_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM)
#define CENTER_X(width) (MARGIN_LEFT + (USABLE_WIDTH - width) / 2)
#define BOTTOM_LINE (SCREEN_HEIGHT - MARGIN_BOTTOM)
#define LARGE_TEXT_SIZE 3
#define SAFE_VISIBLE_OPTIONS 5

const MenuHandlers menuHandlersTable[] = {
    { &MenuManager::mainMenuCW, &MenuManager::mainMenuCCW, &MenuManager::mainMenuEncoderButton, &MenuManager::mainMenuConButton, &MenuManager::mainMenuBackButton }, // MAIN_MENU
    { &MenuManager::gridMenuCW, &MenuManager::gridMenuCCW, &MenuManager::gridMenuEncoderButton, &MenuManager::gridMenuConButton, &MenuManager::gridMenuBackButton }, // GRID_MENU
    { &MenuManager::troubleshootMenuCW, &MenuManager::troubleshootMenuCCW, &MenuManager::troubleshootMenuEncoderButton, &MenuManager::troubleshootMenuConButton, &MenuManager::troubleshootMenuBackButton }, // TROUBLESHOOT_MENU
};

MenuManager::MenuManager(Adafruit_SH1106G& disp) : display(disp), currentMenu(MAIN_MENU) {
}

void MenuManager::updateCurrentColorA(const char* color) {
    currentDetectedColorA = String(color);
}

void MenuManager::updateCurrentColorB(const char* color) {
    currentDetectedColorB = String(color);
}

void MenuManager::updateCurrentColorC(const char* color) {
    currentDetectedColorC = String(color);
}

void MenuManager::updateCurrentColorD(const char* color) {
    currentDetectedColorD = String(color);
}

void MenuManager::updateCurrentMIDINoteA(uint8_t midiNote) {
    currentMIDINoteA = midiNote;
}

void MenuManager::updateCurrentMIDINoteB(uint8_t midiNote) {
    currentMIDINoteB = midiNote;
}

void MenuManager::updateCurrentMIDINoteC(uint8_t midiNote) {
    currentMIDINoteC = midiNote;
}

void MenuManager::updateCurrentMIDINoteD(uint8_t midiNote) {
    currentMIDINoteD = midiNote;
}

// Text centering helper functions
void MenuManager::centerTextAt(int y, String text, int textSize) {
    display.setTextSize(textSize);
    
    // Get text dimensions using Adafruit_GFX API
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    
    // Calculate center X position
    int x = (SCREEN_WIDTH - w) / 2;
    
    display.setCursor(x, y);
    display.print(text);
}

void MenuManager::centerTextInContent(String text, int textSize) {
    display.setTextSize(textSize);
    
    // Get text dimensions using Adafruit_GFX API
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    
    // Calculate center position within screen
    int x = (SCREEN_WIDTH - w) / 2;
    int y = (SCREEN_HEIGHT - h) / 2;
    
    display.setCursor(x, y);
    display.print(text);
}

void MenuManager::handleInput(MenuButton btn) {
    // Map button to handler index
    int handlerIdx = -1;
    switch (btn) {
        case CW: handlerIdx = 0; break;
        case CCW: handlerIdx = 1; break;
        case ENCODER_BUTTON: handlerIdx = 2; break;
        case CON_BUTTON: handlerIdx = 3; break;
        case BAK_BUTTON: handlerIdx = 4; break;
        default: return; // Unknown button
    }
    
    // Call the handler from the table
    if (currentMenu >= 0 && currentMenu < (sizeof(menuHandlersTable)/sizeof(menuHandlersTable[0]))) {
        MenuActionHandler handler = nullptr;
        switch (handlerIdx) {
            case 0: handler = menuHandlersTable[currentMenu].onCW; break;
            case 1: handler = menuHandlersTable[currentMenu].onCCW; break;
            case 2: handler = menuHandlersTable[currentMenu].onEncoderButton; break;
            case 3: handler = menuHandlersTable[currentMenu].onConButton; break;
            case 4: handler = menuHandlersTable[currentMenu].onBackButton; break;
            default: handler = nullptr; break;
        }
        if (handler) {
            (this->*handler)();
        }
    }
}

void MenuManager::render() {
    if (currentMenu == MAIN_MENU) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(OLED_WHITE);
        display.setCursor(0, 0);
        display.print("Main Menu");
        
        // Menu item 1: Grid View
        display.setCursor(10, 20);
        if (mainMenuSelectedIdx == 0) {
            display.setTextColor(OLED_BLACK, OLED_WHITE);
        } else {
            display.setTextColor(OLED_WHITE);
        }
        display.print("Grid View");
        
        // Menu item 2: Troubleshoot
        display.setCursor(10, 35);
        if (mainMenuSelectedIdx == 1) {
            display.setTextColor(OLED_BLACK, OLED_WHITE);
        } else {
            display.setTextColor(OLED_WHITE);
        }
        display.print("Troubleshoot");
        
        display.display(); // Send buffer to screen
        
    } else if (currentMenu == MIDI_GRID_MENU) {
        display.clearDisplay();
        display.setTextSize(1);
        
        // Draw 4x4 grid of MIDI channels 1-16
        const int cellWidth = 30;
        const int cellHeight = 14;
        const int startX = 2;
        const int startY = 2;
        
        for (int i = 0; i < 16; i++) {
            int row = i / 4;
            int col = i % 4;
            int x = startX + col * cellWidth;
            int y = startY + row * cellHeight;
            int channel = i + 1;
            
            bool isSelected = (gridSelectedIdx == channel);
            bool isActiveChannel = (activeMIDIChannelA == channel);
            
            // Draw cell border
            display.drawRect(x, y, cellWidth, cellHeight, OLED_WHITE);
            
            // Handle selection highlighting first
            if (isSelected) {
                // Fill background with white for selection highlighting
                display.fillRect(x + 1, y + 1, cellWidth - 2, cellHeight - 2, OLED_WHITE);
            }
            
            if (isActiveChannel) {
                // Draw big X for active channel
                uint16_t xColor = isSelected ? OLED_BLACK : OLED_WHITE; // Black X if selected, white X if not
                display.drawLine(x + 2, y + 2, x + cellWidth - 3, y + cellHeight - 3, xColor);
                display.drawLine(x + cellWidth - 3, y + 2, x + 2, y + cellHeight - 3, xColor);
            } else {
                // Draw channel number
                display.setCursor(x + cellWidth/2 - 6, y + cellHeight/2 - 4);
                if (isSelected) {
                    display.setTextColor(OLED_BLACK); // Black text on white background
                } else {
                    display.setTextColor(OLED_WHITE); // White text on black background
                }
                display.print(channel);
            }
        }
        
        display.display(); // Send buffer to screen
        
    } else if (currentMenu == TROUBLESHOOT_MENU) {
        display.clearDisplay();
        
        // Draw 2x2 grid for troubleshooting
        int gridCols = 2;
        int gridRows = 2;
        int cellWidth = SCREEN_WIDTH / gridCols;
        int cellHeight = SCREEN_HEIGHT / gridRows;
        
        // Draw grid lines
        for (int i = 1; i < gridCols; i++) {
            int x = i * cellWidth;
            display.drawLine(x, 0, x, SCREEN_HEIGHT - 1, OLED_WHITE);
        }
        for (int i = 1; i < gridRows; i++) {
            int y = i * cellHeight;
            display.drawLine(0, y, SCREEN_WIDTH - 1, y, OLED_WHITE);
        }
        
        // Array of colors and labels for each sensor
        String sensorColors[4] = {currentDetectedColorA, currentDetectedColorB, currentDetectedColorC, currentDetectedColorD};
        String sensorLabels[4] = {"A", "B", "C", "D"};
        
        // Draw each sensor's data in its respective cell
        for (int cellIndex = 0; cellIndex < 4; cellIndex++) {
            int row = cellIndex / 2;
            int col = cellIndex % 2;
            int cellX = col * cellWidth;
            int cellY = row * cellHeight;
            int cellCenterX = cellX + cellWidth / 2;
            int cellCenterY = cellY + cellHeight / 2;
            
            // Draw sensor label in top-right corner of each cell with box
            display.setTextSize(1);
            display.setTextColor(OLED_WHITE);
            int labelX = cellX + cellWidth - 12;
            int labelY = cellY + 2;
            
            // Draw small box around label
            display.drawRect(labelX - 2, labelY - 1, 10, 9, OLED_WHITE);
            
            // Draw label letter
            display.setCursor(labelX, labelY);
            display.print(sensorLabels[cellIndex]);
            
            // Center the color text in the cell
            int16_t x1, y1;
            uint16_t textWidth, textHeight;
            display.getTextBounds(sensorColors[cellIndex], 0, 0, &x1, &y1, &textWidth, &textHeight);
            
            int textX = cellCenterX - (textWidth / 2);
            int textY = cellCenterY - (textHeight / 2);
            
            display.setCursor(textX, textY);
            display.print(sensorColors[cellIndex]);
        }
        
        display.display(); // Send buffer to screen
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
        gridSelectedIdx = 1; // Start with channel 1 selected
    } else if (mainMenuSelectedIdx == 1) {
        currentMenu = TROUBLESHOOT_MENU;
    }
}

void MenuManager::mainMenuConButton() {
    // Secondary action - to be implemented based on needs
}

void MenuManager::mainMenuBackButton() {
    // Back button in main menu does nothing (already at root)
}

// GRID_MENU
void MenuManager::gridMenuCW() {
    // Navigate forward: 1 -> 2 -> ... -> 16 -> 1 (wrap around)
    gridSelectedIdx++;
    if (gridSelectedIdx > 16) {
        gridSelectedIdx = 1;
    }
}

void MenuManager::gridMenuCCW() {
    // Navigate backward: 16 -> 15 -> ... -> 1 -> 16 (wrap around)
    gridSelectedIdx--;
    if (gridSelectedIdx < 1) {
        gridSelectedIdx = 16;
    }
}

void MenuManager::gridMenuEncoderButton() {
    // Encoder button does nothing in grid menu
    // Use CON button to select channels, back button to return to main menu
}

void MenuManager::gridMenuConButton() {
    // CON button sets selected channel as active MIDI channel
    // Send ALL NOTES OFF to current channel before switching
    if (allNotesOffCallback != nullptr) {
        allNotesOffCallback(activeMIDIChannelA);
    }
    
    // Set selected channel as active MIDI channel
    activeMIDIChannelA = gridSelectedIdx;
}

void MenuManager::gridMenuBackButton() {
    // Back button returns to main menu
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

void MenuManager::troubleshootMenuConButton() {
    // CON button returns to main menu
    currentMenu = MAIN_MENU;
}

void MenuManager::troubleshootMenuBackButton() {
    // Back button returns to main menu
    currentMenu = MAIN_MENU;
}

// Set the callback function for ALL NOTES OFF
void MenuManager::setAllNotesOffCallback(AllNotesOffCallback callback) {
    allNotesOffCallback = callback;
}

