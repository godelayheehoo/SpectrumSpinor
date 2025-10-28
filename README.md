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

- **Non-blocking Architecture**: All input polling and color detection use millis() timing
- **Debounced Input**: 50ms debounce for reliable button and encoder handling  
- **Shared I2C Bus**: OLED display and color sensor on single I2C bus (GPIO 21/22)
- **Table-Driven Menu System**: Expandable architecture with separate handlers per menu
- **Efficient Color Processing**: Enum-based color detection vs string comparisons
- **MIDI Integration**: Full MIDI note generation with fortyseveneffects MIDI Library
- **Callback Architecture**: MenuManager uses callbacks to access MIDI functionality
- **Serial Debugging**: Comprehensive logging at 115200 baud

## Development Status

✅ **Completed:**
- ESP32 project structure with PlatformIO
- SH1106 OLED display integration (Adafruit_SH110X library)
- TCS34725 color sensor integration with shared I2C bus
- Rotary encoder and multi-button input handling
- Complete menu navigation system with table-driven handlers
- MIDI channel grid with visual selection and active channel display
- Real-time color detection and MIDI note generation
- Color enum system (PINK, RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, WHITE)
- Scale management system for color-to-MIDI conversion
- ALL NOTES OFF functionality when changing channels
- Button debouncing and non-blocking input polling
- Troubleshooting interface with 2x2 diagnostic grid

🔄 **In Progress:**
- Menu system refinements and additional diagnostic displays

📋 **Planned:**
- Stepper motor integration for disk rotation
- Scale/mode selection menus (Major, Minor, etc.)
- Color calibration interface
- Multi-ring support with multiple sensors
- Auto-calibration functionality

## Building

This project uses PlatformIO with the Arduino framework:

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

```
├── src/
│   ├── main.cpp              # Main application loop and hardware initialization
│   ├── MenuManager.h/.cpp    # Complete menu system with table-driven handlers
│   ├── ColorHelper.cpp       # TCS34725 color sensor integration
│   └── ScaleManager.cpp      # Color-to-MIDI note conversion
├── include/
│   ├── PinDefinitions.h      # Hardware pin assignments
│   ├── SystemConfig.h        # Display and system constants
│   ├── EEPROMAddresses.h     # Memory layout (unused currently)
│   ├── ColorEnum.h           # Efficient color enumeration system
│   ├── ColorInfo.h           # Color detection data structures
│   └── ScaleManager.h        # Musical scale management
└── platformio.ini            # Project config with library dependencies
```

## Libraries Used

- **Adafruit_SH110X**: OLED display driver
- **Adafruit_TCS34725**: Color sensor library  
- **fortyseveneffects MIDI Library**: Hardware MIDI communication
- **Wire**: I2C communication for display and sensor

## System Architecture

- **Main Loop**: Non-blocking polling of inputs and periodic color detection
- **Menu System**: Table-driven handlers for each menu state and input type
- **Color Detection**: Enum-based system for efficient color processing
- **MIDI Generation**: Real-time note generation based on detected colors
- **Callback Pattern**: MenuManager uses function pointers to access MIDI functionality from main.cpp

## Notes

- Uses shared I2C bus (GPIO 21/22) for both OLED display and color sensor
- All timing is non-blocking using millis() for responsive interface
- MIDI output uses hardware serial on GPIO 17 at standard 31250 baud
- Color detection runs every 500ms to balance responsiveness with processing load
- Button polling occurs every 10ms for responsive user interface
- Serial debug output at 115200 baud provides comprehensive system logging