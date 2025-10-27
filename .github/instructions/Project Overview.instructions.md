---
applyTo: '**'
---
This "SpectrumSpinor" project intends to roughly recreate the playtronic "MIDI COLOR SEQUENCER ORBITA" using an ESP32 microcontroller.  

The device uses a rotating disk.  Above one radial line of that disk is an arm that will ultimatley contain some number of color sensors.  As the disk rotates, the sensors will read the colors on the disk and generate MIDI signals accordingly.  The project will also include a small display to provide feedback to the user.  We will probably start out with just one color sensor to get the basic functionality working, and then add more sensors later.

The project is being developed using PlatformIO and the Arduino framework. We want lots of code comments and documentation to make the code easy to understand for ourselves and others.

The hardware components being used in this project include:
- ESP32 microcontroller
- Adafruit ST7789 display (ST7789 TFT Screen, 320x240 resolution)
    - built-in clickable rotary encoder
    - auxillary button
- Color sensors (TCS34725)
- NEMA 17 stepper motor
- A4988 stepper motor driver

The menu will have at least the following options:
- MIDI channel selection
- Scale selection

# Code suggestions
Instead of using delay() functions, use non-blocking code with millis() to allow for better responsiveness and multitasking.  Special exception can be made if the situation calls for it.