#pragma once
//EEPROM magic number to indicate valid stored settings
#define EEPROM_MAGIC_ADDRESS 0x01
#define EEPROM_MAGIC_VALUE 0x00

#define SENSOR_A_CALIBRATION 1
#define SENSOR_B_CALIBRATION 125 //124 + 1
#define SENSOR_C_CALIBRATION 249 // 2*124 + 1
#define SENSOR_D_CALIBRATION 373 // 3*124 + 1
//Each sensor calibration block is 124 bytes

#define ACTIVE_MIDI_CHANNEL_A 374
#define ACTIVE_MIDI_CHANNEL_B 375
#define ACTIVE_MIDI_CHANNEL_C 376
#define ACTIVE_MIDI_CHANNEL_D 377

#define OCTAVE_A 378
#define OCTAVE_B 379
#define OCTAVE_C 380
#define OCTAVE_D 381
