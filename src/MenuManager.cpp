#include "MenuManager.h"
#include "SystemConfig.h"
#include <EEPROM.h>
#include "EEPROMAddresses.h"

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
    { &MenuManager::mainMenuEncoder, &MenuManager::mainMenuEncoderButton, &MenuManager::mainMenuConButton, &MenuManager::mainMenuBackButton }, // MAIN_MENU
    { &MenuManager::gridMenuEncoder, &MenuManager::gridMenuEncoderButton, &MenuManager::gridMenuConButton, &MenuManager::gridMenuBackButton }, // GRID_MENU
    { &MenuManager::troubleshootMenuEncoder, &MenuManager::troubleshootMenuEncoderButton, &MenuManager::troubleshootMenuConButton, &MenuManager::troubleshootMenuBackButton }, // TROUBLESHOOT_MENU
    { &MenuManager::calibrationMenuEncoder, &MenuManager::calibrationMenuEncoderButton, &MenuManager::calibrationMenuConButton, &MenuManager::calibrationMenuBackButton }, // CALIBRATION_MENU
    { &MenuManager::octaveMenuEncoder, &MenuManager::octaveMenuEncoderButton, &MenuManager::octaveMenuConButton, &MenuManager::octaveMenuBackButton },  // OCTAVE_MENU
    { &MenuManager::calibrationMenuAEncoder, &MenuManager::calibrationMenuAEncoderButton, &MenuManager::calibrationMenuAConButton, &MenuManager::calibrationMenuABackButton }, // CALIBRATION_A_MENU
    { &MenuManager::calibrationMenuBEncoder, &MenuManager::calibrationMenuBEncoderButton, &MenuManager::calibrationMenuBConButton, &MenuManager::calibrationMenuBBackButton }, // CALIBRATION_B_MENU
    { &MenuManager::calibrationMenuCEncoder, &MenuManager::calibrationMenuCEncoderButton, &MenuManager::calibrationMenuCConButton, &MenuManager::calibrationMenuCBackButton }, // CALIBRATION_C_MENU
    { &MenuManager::calibrationMenuDEncoder, &MenuManager::calibrationMenuDEncoderButton, &MenuManager::calibrationMenuDConButton, &MenuManager::calibrationMenuDBackButton }  // CALIBRATION_D_MENU
};

MenuManager::MenuManager(Adafruit_SH1106G& disp) : display(disp), currentMenu(TROUBLESHOOT_MENU) {
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

void MenuManager::updateCurrentRGBA(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
    currentRGBA[0] = r; currentRGBA[1] = g; currentRGBA[2] = b; currentRGBA[3] = c;
}

void MenuManager::updateCurrentRGBB(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
    currentRGBB[0] = r; currentRGBB[1] = g; currentRGBB[2] = b; currentRGBB[3] = c;
}

void MenuManager::updateCurrentRGBC(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
    currentRGBC[0] = r; currentRGBC[1] = g; currentRGBC[2] = b; currentRGBC[3] = c;
}

void MenuManager::updateCurrentRGBD(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
    currentRGBD[0] = r; currentRGBD[1] = g; currentRGBD[2] = b; currentRGBD[3] = c;
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
        case ENCODER_BUTTON: handlerIdx = 0; break;
        case CON_BUTTON: handlerIdx = 1; break;
        case BAK_BUTTON: handlerIdx = 2; break;
        default: {Serial.println("Unknown button!"); 
        return; // Unknown button
        };
    }
    
    // Call the handler from the table
    if (currentMenu >= 0 && currentMenu < (sizeof(menuHandlersTable)/sizeof(menuHandlersTable[0]))) {
        MenuActionHandler handler = nullptr;
        switch (handlerIdx) {
            case 0: handler = menuHandlersTable[currentMenu].onEncoderButton; break;
            case 1: handler = menuHandlersTable[currentMenu].onConButton; break;
            case 2: handler = menuHandlersTable[currentMenu].onBackButton; break;
            default: handler = nullptr; break;
        }
        if (handler) {
            (this->*handler)();
        }
    }
}

void MenuManager::handleEncoder(int turns){
    (this->*menuHandlersTable[currentMenu].onEncoder)(turns);
}

void MenuManager::render() {
    if (currentMenu == MAIN_MENU) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(OLED_WHITE);
        display.setCursor(10, 0);
        // Menu item 1: Grid View
        if (mainMenuSelectedIdx == 0) {
            display.setTextColor(OLED_BLACK, OLED_WHITE);
        } else {
            display.setTextColor(OLED_WHITE);
        }
        display.print("Grid View");
        // Menu item 2: Troubleshoot
        display.setCursor(10, 15);
        if (mainMenuSelectedIdx == 1) {
            display.setTextColor(OLED_BLACK, OLED_WHITE);
        } else {
            display.setTextColor(OLED_WHITE);
        }
        display.print("Troubleshoot");
        // Menu item 3: Calibration
        display.setCursor(10, 30);
        if (mainMenuSelectedIdx == 2) {
            display.setTextColor(OLED_BLACK, OLED_WHITE);
        } else {
            display.setTextColor(OLED_WHITE);
        }
        display.print("Calibration");
        // Menu item 4: Octave
        display.setCursor(10,45);
        if (mainMenuSelectedIdx == 3) {
            display.setTextColor(OLED_BLACK, OLED_WHITE);
        } else {
            display.setTextColor(OLED_WHITE);
        }
        display.print("Octave");
        
        display.display(); // Send buffer to screen
        
    } else if (currentMenu == MIDI_GRID_MENU) {
        display.clearDisplay();
        display.setTextSize(1);
        
        // Draw 4x4 grid of MIDI channels 1-16 (narrower to make room for sensor indicator)
        const int cellWidth = 22; // Reduced from 30 to 22
        const int cellHeight = 14;
        const int startX = 2;
        const int startY = 2;
        
        // Draw active MIDI grid sensor in top right corner
        display.setTextSize(2);
        display.setTextColor(OLED_WHITE);
        display.setCursor(SCREEN_WIDTH - 20, 5); // Position in top right
        
        // Convert enum to character for display
        char sensorChar;
        switch (activeMIDIGridSensor) {
            case SENSOR_A: sensorChar = 'A'; break;
            case SENSOR_B: sensorChar = 'B'; break;
            case SENSOR_C: sensorChar = 'C'; break;
            case SENSOR_D: sensorChar = 'D'; break;
            default: sensorChar = 'A'; break;
        }
        display.print(sensorChar);
        display.setTextSize(1); // Reset text size for grid
        
        for (int i = 0; i < 16; i++) {
            int row = i / 4;
            int col = i % 4;
            int x = startX + col * cellWidth;
            int y = startY + row * cellHeight;
            int channel = i + 1;
            
            bool isSelected = (gridSelectedIdx == channel);
            
            // Determine which channel is active based on current sensor selection
            int currentActiveMIDIChannel;
            switch (activeMIDIGridSensor) {
                case SENSOR_A: currentActiveMIDIChannel = activeMIDIChannelA; break;
                case SENSOR_B: currentActiveMIDIChannel = activeMIDIChannelB; break;
                case SENSOR_C: currentActiveMIDIChannel = activeMIDIChannelC; break;
                case SENSOR_D: currentActiveMIDIChannel = activeMIDIChannelD; break;
                default: currentActiveMIDIChannel = activeMIDIChannelA; break;
            }
            
            bool isActiveChannel = (currentActiveMIDIChannel == channel);
            
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
            int labelX = cellX + cellWidth - 8;
            int labelY = cellY + 2;
            
            // Draw label letter (no box)
            display.setCursor(labelX, labelY);
            display.print(sensorLabels[cellIndex]);
            
            if (troubleshootMode == 0) {
                // Mode 0: Color names (centered)
                String displayText = sensorColors[cellIndex];
                int16_t x1, y1;
                uint16_t textWidth, textHeight;
                display.getTextBounds(displayText, 0, 0, &x1, &y1, &textWidth, &textHeight);
                
                int textX = cellCenterX - (textWidth / 2);
                int textY = cellCenterY - (textHeight / 2);
                
                display.setCursor(textX, textY);
                display.print(displayText);
            } else if (troubleshootMode == 1) {
                // Mode 1: RGB values in three rows (no labels, drop C value)
                uint16_t* rgbValues = nullptr;
                switch (cellIndex) {
                    case 0: rgbValues = currentRGBA; break;
                    case 1: rgbValues = currentRGBB; break;
                    case 2: rgbValues = currentRGBC; break;
                    case 3: rgbValues = currentRGBD; break;
                }
                if (rgbValues) {
                    // Display RGB values in three rows, starting at top of cell (aligned with sensor label)
                    String rValue = String(rgbValues[0]);
                    String gValue = String(rgbValues[1]);
                    String bValue = String(rgbValues[2]);
                    
                    int lineHeight = 10; // Approximate height of text size 1
                    int startY = cellY + 2; // Align with sensor label
                    
                    // R value (top row)
                    int16_t x1, y1;
                    uint16_t textWidth, textHeight;
                    display.getTextBounds(rValue, 0, 0, &x1, &y1, &textWidth, &textHeight);
                    display.setCursor(cellCenterX - (textWidth / 2), startY);
                    display.print(rValue);
                    
                    // G value (middle row)
                    display.getTextBounds(gValue, 0, 0, &x1, &y1, &textWidth, &textHeight);
                    display.setCursor(cellCenterX - (textWidth / 2), startY + lineHeight);
                    display.print(gValue);
                    
                    // B value (bottom row)
                    display.getTextBounds(bValue, 0, 0, &x1, &y1, &textWidth, &textHeight);
                    display.setCursor(cellCenterX - (textWidth / 2), startY + (2 * lineHeight));
                    display.print(bValue);
                } else {
                    // Fallback if no RGB data
                    display.setCursor(cellCenterX - 6, cellCenterY);
                    display.print("N/A");
                }
            } else if (troubleshootMode == 2) {
                // Mode 2: MIDI Note values (centered)
                uint8_t midiNote = 0;
                switch (cellIndex) {
                    case 0: midiNote = currentMIDINoteA; break;
                    case 1: midiNote = currentMIDINoteB; break;
                    case 2: midiNote = currentMIDINoteC; break;
                    case 3: midiNote = currentMIDINoteD; break;
                }
                String displayText = String(midiNote);
                int16_t x1, y1;
                uint16_t textWidth, textHeight;
                display.getTextBounds(displayText, 0, 0, &x1, &y1, &textWidth, &textHeight);
                
                int textX = cellCenterX - (textWidth / 2);
                int textY = cellCenterY - (textHeight / 2);
                
                display.setCursor(textX, textY);
                display.print(displayText);
            }
        }
        
        display.display(); // Send buffer to screen
        
    } else if (currentMenu == CALIBRATION_MENU) {
        display.clearDisplay();
        display.setTextSize(1);
        
        // Sensor options: A, B, C, D (no title, start from top to fit all 4)
        const char* sensorNames[4] = {"A", "B", "C", "D"};
        
        for (int i = 0; i < 4; i++) {
            display.setCursor(10, 5 + (i * 15)); // Start at y=5 instead of y=20
            
            // Highlight selected sensor
            if (calibrationSelectedIdx == i) {
                display.setTextColor(OLED_BLACK, OLED_WHITE);
            } else {
                display.setTextColor(OLED_WHITE);
            }
            
            display.print("Ring ");
            display.print(sensorNames[i]);
        }
        
        display.display(); // Send buffer to screen
    } else if(currentMenu == OCTAVE_MENU) {
        display.clearDisplay();
                // Convert enum to character for display
        char sensorChar;
        int octaveToDisplay;
        switch (activeOctaveSensor) {
            case SENSOR_A: sensorChar = 'A'; octaveToDisplay = octaveA; break;
            case SENSOR_B: sensorChar = 'B'; octaveToDisplay = octaveB; break;
            case SENSOR_C: sensorChar = 'C'; octaveToDisplay = octaveC; break;
            case SENSOR_D: sensorChar = 'D'; octaveToDisplay = octaveD; break;
            default: sensorChar = 'A'; octaveToDisplay = octaveA; break;
        }

        display.setTextSize(2);
        display.setTextColor(OLED_WHITE);
        display.setCursor(SCREEN_WIDTH - 20, 5); // Position in top right
        display.print(sensorChar);

        // display.setTextSize(4);
        // display.setCursor(10,10);
        // display.print(octaveToDisplay);
        centerTextInContent(String(octaveToDisplay), 5);

        display.display();
    }
    else if(currentMenu == CALIBRATION_A_MENU){
        SharedCalibrationMenuRender(calibrationMenuASelectedIdx, calibrationMenuAScrollIdx);
    }
}

void MenuManager::startCalibrationCountdown(){
    display.clearDisplay();
    display.setTextColor(OLED_WHITE,OLED_BLACK);
    for(int i=5;i>=0;i--){
        display.clearDisplay();
        centerTextInContent(String(i), 5);
        display.display();
        delay(500);
    }
}

// Handler functions for each menu

// MAIN_MENU

void MenuManager::mainMenuEncoder(int turns){
    mainMenuSelectedIdx = constrain(mainMenuSelectedIdx + turns, 0, NUM_MAIN_MENU_ITEMS-2); //-2 because we don't count MAIN_MENU itself!
    if (mainMenuSelectedIdx < mainMenuScrollIdx) {
        mainMenuScrollIdx = mainMenuSelectedIdx;
    } else if (mainMenuSelectedIdx > mainMenuScrollIdx + MAIN_MENU_VISIBLE_ITEMS - 1) {
        mainMenuScrollIdx = mainMenuSelectedIdx - MAIN_MENU_VISIBLE_ITEMS + 1;
    }
}

void MenuManager::mainMenuEncoderButton() {
    if (mainMenuSelectedIdx == 0) {
        currentMenu = MIDI_GRID_MENU;
        gridSelectedIdx = 1; // Start with channel 1 selected
    } else if (mainMenuSelectedIdx == 1) {
        currentMenu = TROUBLESHOOT_MENU;
    } else if (mainMenuSelectedIdx == 2) {
        currentMenu = CALIBRATION_MENU;
    }
    else if (mainMenuSelectedIdx ==3){
        currentMenu = OCTAVE_MENU;
    }
}

void MenuManager::mainMenuConButton() {
    // Secondary action - to be implemented based on needs
}

void MenuManager::mainMenuBackButton() {
    // Back button in main menu does nothing (already at root)
}

// GRID_MENU
void MenuManager::gridMenuEncoder(int turns) {
    gridSelectedIdx = ((gridSelectedIdx - 1 + turns) % 16 + 16) % 16 + 1;
}

void MenuManager::gridMenuEncoderButton() {
    // Encoder button cycles through active sensors: A -> B -> C -> D -> A
    switch (activeMIDIGridSensor) {
        case SENSOR_A: activeMIDIGridSensor = SENSOR_B; break;
        case SENSOR_B: activeMIDIGridSensor = SENSOR_C; break;
        case SENSOR_C: activeMIDIGridSensor = SENSOR_D; break;
        case SENSOR_D: activeMIDIGridSensor = SENSOR_A; break;
        default: activeMIDIGridSensor = SENSOR_A; break;
    }
}

void MenuManager::gridMenuConButton() {
    // CON button sets selected channel as active MIDI channel for current sensor
    // Send ALL NOTES OFF to current channel before switching
    if (allNotesOffCallback != nullptr) {
        allNotesOffCallback(*getActiveSensorMIDIChannel());
    }
    
    // Set selected channel as active MIDI channel for current sensor
    setActiveSensorMIDIChannel(gridSelectedIdx);
    saveMIDIGrid();
}

void MenuManager::gridMenuBackButton() {
    // Back button returns to main menu
    currentMenu = MAIN_MENU;
}

// TROUBLESHOOT_MENU
void MenuManager::troubleshootMenuEncoder(int turns) {
    // Encoder rotation does nothing in troubleshoot menu
}

void MenuManager::troubleshootMenuEncoderButton() {
    // Encoder button cycles troubleshoot modes: 0 (colors) -> 1 (RGB) -> 2 (MIDI Notes) -> 0
    int oldMode = troubleshootMode;
    troubleshootMode = (troubleshootMode + 1) % 3;
    // If we just switched to RGB mode, request current RGB readings
    if (oldMode != 1 && troubleshootMode == 1) {
        requestRGBUpdate = true;
    }
}

void MenuManager::troubleshootMenuConButton() {
    // Confirm button does nothing in troubleshoot menu
}

void MenuManager::troubleshootMenuBackButton() {
    // Back button returns to main menu
    currentMenu = MAIN_MENU;
}

// CALIBRATION_MENU
void MenuManager::calibrationMenuEncoder(int turns){
    calibrationSelectedIdx = constrain(calibrationSelectedIdx + turns, 0, 3);
}

void MenuManager::calibrationMenuEncoderButton() {
    // Encoder button enters calibration for selected sensor
    switch(calibrationMenuSelectedIdx){
        case 0:
            currentMenu = CALIBRATION_A_MENU;
            calibrationMenuASelectedIdx = 0;
            calibrationMenuAScrollIdx = 0;
            break;
        case 1:
            currentMenu = CALIBRATION_A_MENU;
            calibrationMenuASelectedIdx = 0;
            calibrationMenuAScrollIdx = 0;
            break;
        case 2:
            currentMenu = CALIBRATION_A_MENU;
            calibrationMenuASelectedIdx = 0;
            calibrationMenuAScrollIdx = 0;
            break;
        case 3:
            currentMenu = CALIBRATION_A_MENU;
            calibrationMenuASelectedIdx = 0;
            calibrationMenuAScrollIdx = 0;
            break;
    }
}

void MenuManager::calibrationMenuConButton() {
    // CON button enters calibration for selected sensor
    // For now, do nothing - will implement calibration sub-menus later
}

void MenuManager::calibrationMenuBackButton() {
    // Back button returns to main menu
    currentMenu = MAIN_MENU;
}

//// Calibration submenu commands
void MenuManager::calibrationMenuAEncoder(int turns){
    calibrationMenuASelectedIdx = constrain(calibrationMenuASelectedIdx+turns, 0, CALIBRATION_SUBMENU_TOTAL_ITEMS-1);
    if (calibrationMenuASelectedIdx < calibrationMenuAScrollIdx) {
            calibrationMenuAScrollIdx = calibrationMenuASelectedIdx;
        }
    if (calibrationMenuASelectedIdx > calibrationMenuAScrollIdx + CALIBRATION_SUBMENU_VISIBLE_ITEMS - 1) {
            calibrationMenuAScrollIdx = calibrationMenuASelectedIdx - CALIBRATION_SUBMENU_VISIBLE_ITEMS + 1;
        }
}
void MenuManager::calibrationMenuAEncoderButton(){
    switch(calibrationMenuASelectedIdx){
        case 0:
            pendingCalibrationA = PendingCalibrationA::WHITE;
            break;
        case 1:
            pendingCalibrationA = PendingCalibrationA::BLACK;
            break;
        case 2:
            pendingCalibrationA = PendingCalibrationA::PINK;
            break;

        case 3:
            pendingCalibrationA = PendingCalibrationA::ORANGE;
            break;
        case 4:
            pendingCalibrationA = PendingCalibrationA::BLUE;
            break;
        case 5:
            pendingCalibrationA = PendingCalibrationA::YELLOW;
            break;
        case 6:
            pendingCalibrationA = PendingCalibrationA::GREEN;
            break;
        case 7:
            pendingCalibrationA = PendingCalibrationA::RED;
            break;
        case 8:
            pendingCalibrationA = PendingCalibrationA::PURPLE;
            break;
        case 9:
            pendingCalibrationA = PendingCalibrationA::BROWN;
            break;
    }
}

void MenuManager::calibrationMenuAConButton(){}
void MenuManager::calibrationMenuABackButton(){
    currentMenu = CALIBRATION_MENU;
}

void MenuManager::calibrationMenuBEncoder(int turns){
    calibrationMenuBSelectedIdx = constrain(calibrationMenuBSelectedIdx+turns, 0, CALIBRATION_SUBMENU_TOTAL_ITEMS-1);
    if (calibrationMenuBSelectedIdx < calibrationMenuBScrollIdx) {
            calibrationMenuBScrollIdx = calibrationMenuBSelectedIdx;
        }
    if (calibrationMenuBSelectedIdx > calibrationMenuBScrollIdx + CALIBRATION_SUBMENU_VISIBLE_ITEMS - 1) {
            calibrationMenuBScrollIdx = calibrationMenuBSelectedIdx - CALIBRATION_SUBMENU_VISIBLE_ITEMS + 1;
        }
}
void MenuManager::calibrationMenuBEncoderButton(){
}
void MenuManager::calibrationMenuBConButton(){}
void MenuManager::calibrationMenuBBackButton(){
    currentMenu = CALIBRATION_MENU;
}

void MenuManager::calibrationMenuCEncoder(int turns){
    calibrationMenuCSelectedIdx = constrain(calibrationMenuCSelectedIdx+turns, 0, CALIBRATION_SUBMENU_TOTAL_ITEMS-1);
    if (calibrationMenuCSelectedIdx < calibrationMenuCScrollIdx) {
            calibrationMenuCScrollIdx = calibrationMenuCSelectedIdx;
        }
    if (calibrationMenuCSelectedIdx > calibrationMenuCScrollIdx + CALIBRATION_SUBMENU_VISIBLE_ITEMS - 1) {
            calibrationMenuCScrollIdx = calibrationMenuCSelectedIdx - CALIBRATION_SUBMENU_VISIBLE_ITEMS + 1;
        }
}
void MenuManager::calibrationMenuCEncoderButton(){
}
void MenuManager::calibrationMenuCConButton(){}
void MenuManager::calibrationMenuCBackButton(){
    currentMenu = CALIBRATION_MENU;
}

void MenuManager::calibrationMenuDEncoder(int turns){
    calibrationMenuDSelectedIdx = constrain(calibrationMenuDSelectedIdx+turns, 0, CALIBRATION_SUBMENU_TOTAL_ITEMS-1);
    if (calibrationMenuDSelectedIdx < calibrationMenuDScrollIdx) {
            calibrationMenuDScrollIdx = calibrationMenuDSelectedIdx;
        }
    if (calibrationMenuDSelectedIdx > calibrationMenuDScrollIdx + CALIBRATION_SUBMENU_VISIBLE_ITEMS - 1) {
            calibrationMenuDScrollIdx = calibrationMenuDSelectedIdx - CALIBRATION_SUBMENU_VISIBLE_ITEMS + 1;
        }
}
void MenuManager::calibrationMenuDEncoderButton(){
}
void MenuManager::calibrationMenuDConButton(){}
void MenuManager::calibrationMenuDBackButton(){
    currentMenu = CALIBRATION_MENU;
}





// OCTAVE MENU
void MenuManager::octaveMenuEncoder(int turns) {
    switch (activeOctaveSensor) {
        case SENSOR_A: octaveA = constrain(octaveA + turns, 0, 8); break;
        case SENSOR_B: octaveB = constrain(octaveB + turns, 0, 8); break;
        case SENSOR_C: octaveC = constrain(octaveC + turns, 0, 8); break;
        case SENSOR_D: octaveD = constrain(octaveD + turns, 0, 8); break;
        default: octaveA = constrain(octaveA + turns, 0, 8); break;
    }
}

void MenuManager::octaveMenuEncoderButton(

){
       switch (activeOctaveSensor) {
        case SENSOR_A: activeOctaveSensor = SENSOR_B; break;
        case SENSOR_B: activeOctaveSensor = SENSOR_C; break;
        case SENSOR_C: activeOctaveSensor = SENSOR_D; break;
        case SENSOR_D: activeOctaveSensor = SENSOR_A; break;
        default: activeOctaveSensor = SENSOR_A; break;
    }
}

void MenuManager::octaveMenuConButton(){}

void MenuManager::octaveMenuBackButton(){
    currentMenu = MAIN_MENU;
}


//// helper functions
void MenuManager::SharedCalibrationMenuRender(int selectedIdx, int scrollIdx){
    display.clearDisplay();
    display.setTextSize(1);
    const char* menus[10] = {"White Balance", "Dark Offset", "Pink", "Orange", "Blue",
    "Yellow","Green","Red","Purple","Brown"};
    int yStart = 10;
    display.setCursor(10,yStart);
    display.setTextColor(OLED_WHITE);

    int itemIdx = scrollIdx;
    int y = yStart;
    for(int visible = 0; visible < CALIBRATION_SUBMENU_VISIBLE_ITEMS; ++visible,++itemIdx){
        display.setCursor(10,y);
        if(itemIdx == selectedIdx){
            display.setTextColor(OLED_BLACK, OLED_WHITE);
        } else {
            display.setTextColor(OLED_WHITE, OLED_BLACK);
        }
        display.print(menus[itemIdx]);
    y+=10;
    }
    display.display();    
}
// Set the callback function for ALL NOTES OFF
void MenuManager::setAllNotesOffCallback(AllNotesOffCallback callback) {
    allNotesOffCallback = callback;
}

// Helper functions for working with active sensor
byte* MenuManager::getActiveSensorMIDIChannel() {
    switch (activeMIDIGridSensor) {
        case SENSOR_A: return &activeMIDIChannelA;
        case SENSOR_B: return &activeMIDIChannelB;
        case SENSOR_C: return &activeMIDIChannelC;
        case SENSOR_D: return &activeMIDIChannelD;
        default: return &activeMIDIChannelA;
    }
}

void MenuManager::setActiveSensorMIDIChannel(int channel) {
    switch (activeMIDIGridSensor) {
        case SENSOR_A: activeMIDIChannelA = channel; break;
        case SENSOR_B: activeMIDIChannelB = channel; break;
        case SENSOR_C: activeMIDIChannelC = channel; break;
        case SENSOR_D: activeMIDIChannelD = channel; break;
        default: activeMIDIChannelA = channel; break;
    }
}

// save functions
void MenuManager::saveMIDIGrid(){
    switch(activeMIDIGridSensor){
        case SENSOR_A:
            EEPROM.put(ACTIVE_MIDI_CHANNEL_A_ADDR, activeMIDIChannelA);
            break;
        case SENSOR_B:
            EEPROM.put(ACTIVE_MIDI_CHANNEL_B_ADDR, activeMIDIChannelB);
            break;
        case SENSOR_C:
            EEPROM.put(ACTIVE_MIDI_CHANNEL_C_ADDR, activeMIDIChannelC);
            break;
        case SENSOR_D:
            EEPROM.put(ACTIVE_MIDI_CHANNEL_D_ADDR, activeMIDIChannelD);
            break;
    }
}