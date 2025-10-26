# ColorWheel ESP32 Project

This project implements a menu-driven interface on an ESP32 with a ST7789 TFT display and rotary encoder input.

## Hardware Components

- **ESP32 Development Board**: Main microcontroller
- **ST7789 TFT Display**: 320x240 resolution screen
- **Rotary Encoder**: With built-in push button for navigation
- **Auxiliary Button**: Secondary input button

## Pin Connections

```
TFT Display:
- SCLK: GPIO 18
- MOSI: GPIO 23  
- DC:   GPIO 2
- CS:   GPIO 5
- RST:  Not connected (handled by software)

Rotary Encoder:
- A:    GPIO 32
- B:    GPIO 33  
- BTN:  GPIO 25

Auxiliary Button:
- BTN:  GPIO 26

Debug LED:
- LED:  GPIO 14
```

## Menu System

The project features a table-driven menu system with the following menus:

### Main Menu
- Navigate with rotary encoder (CW/CCW)
- Select "Grid View" to access MIDI channel selection

### MIDI Grid Menu  
- 4x4 grid showing MIDI channels 1-16
- "..." in top left to return to main menu
- Currently selected channel highlighted in white
- Active MIDI channel highlighted in green
- Use encoder to navigate, press to select active channel
- Aux button returns to main menu

## Features

- **Non-blocking Input**: Encoder and button states polled without delays
- **Debounced Buttons**: 50ms debounce for reliable input
- **TFT_eSPI Library**: Optimized display library for ESP32
- **Table-Driven Handlers**: Expandable menu system architecture
- **Debug Output**: Serial logging at 115200 baud

## Development Status

✅ **Completed:**
- ESP32 project structure
- TFT display initialization and rendering
- Rotary encoder input handling
- Menu navigation system
- MIDI channel grid with highlighting
- Button debouncing
- Library conversion from Adafruit to TFT_eSPI

🔄 **In Progress:**
- Testing display functionality with hardware

📋 **Planned:**
- Actual MIDI functionality implementation
- Color sensor integration
- Stepper motor control
- Scale/offset selection menus

## Building

This project uses PlatformIO. To build:

```bash
cd colorwheel
pio run
```

To upload to ESP32:
```bash
pio run --target upload
```

To monitor serial output:
```bash
pio device monitor
```

## Code Structure

- `src/main.cpp`: Main application loop with setup and input polling
- `src/MenuManager.h/.cpp`: Menu system implementation
- `include/PinDefinitions.h`: Hardware pin assignments  
- `platformio.ini`: Project configuration and dependencies

## Notes

- The project was converted from Adafruit_ST7789 to TFT_eSPI library for better ESP32 compatibility
- Touch functionality warnings can be ignored - we're using encoder input instead
- Debug LED on GPIO 14 provides visual feedback during development