# SpectrumSpinor ESP32 Project

This project recreates the functionality of the Playtronic "MIDI COLOR SEQUENCER ORBITA" using an ESP32 microcontroller. The device uses a rotating disk with color sensors to generate MIDI signals based on detected colors.

## Hardware Components

- **ESP32 Development Board**: Main microcontroller
- **SH1106 OLED Display**: 128x64 monochrome I2C display
- **TCS34725 Color Sensor**: I2C color detection sensor
- **Rotary Encoder**: With built-in push button for navigation
- **Control Button (CON)**: Secondary input button
- **Back Button (BAK)**: Navigation button
- **Panic Button**: Emergency MIDI all-notes-off button
- **NEMA 17 Stepper Motor**: For rotating the color disk (planned)
- **A4988 Stepper Driver**: Motor control (planned)

## Pin Connections

```
OLED Display (I2C):
- SDA:  GPIO 21
- SCL:  GPIO 22
- RST:  GPIO 4

Color Sensor (I2C - shared bus):
- SDA:  GPIO 21  
- SCL:  GPIO 22

Rotary Encoder:
- A:    GPIO 32
- B:    GPIO 33  
- BTN:  GPIO 25

Buttons:
- CON:   GPIO 26
- BAK:   GPIO 14
- PANIC: GPIO 27

MIDI Output:
- TX:   GPIO 17 (Hardware Serial)
```

## Menu System

The project features a table-driven menu system optimized for the OLED display:

### Main Menu
- **Grid View**: Access MIDI channel selection
- **Troubleshoot**: View current color detection and system status
- Navigate with rotary encoder (CW/CCW)
- Select with encoder button
- Back button returns to previous menu

### MIDI Grid Menu  
- 4x4 grid showing MIDI channels 1-16
- Currently selected channel highlighted with white background
- Active MIDI channel marked with "X"
- When active channel is selected: shows white background with black X
- Navigate with encoder rotation (CW/CCW wraps around 1-16)
- **CON button**: Confirms selection and sets active MIDI channel
- **Back button**: Returns to main menu
- Automatically sends ALL NOTES OFF to previous channel when switching

### Troubleshoot Menu
- 2x2 grid layout for system diagnostics
- Top-left cell shows currently detected color
- Other cells reserved for future diagnostics
- **Back button**: Returns to main menu

## Color Detection & MIDI

The system continuously monitors the color sensor and generates MIDI notes:

- **Color Enum System**: Efficient integer-based color identification (9 colors: PINK through WHITE)
- **Scale Management**: Converts colors to MIDI notes based on musical scales
- **MIDI Output**: Hardware serial MIDI at 31250 baud on GPIO 17
- **Note Handling**: 
  - Sends note-off for previous color before new note-on
  - WHITE color acts as note-off signal
  - Configurable velocity and channel selection
- **Panic Function**: Emergency all-notes-off on all channels (panic button)

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
cd spectrum_spinor
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