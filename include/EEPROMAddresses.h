#pragma once
//EEPROM magic number to indicate valid stored settings
#define EEPROM_MAGIC_ADDRESS 0x00 // don't change this one
#define EEPROM_MAGIC_VALUE 0x12 //do change this one

//todo: these are only 28 bytes right now, but may change when we rework them to store calibration data.
#define SENSOR_A_CALIBRATION_ADDR 1
#define SENSOR_B_CALIBRATION_ADDR 137 //136 + 1 + 1
#define SENSOR_C_CALIBRATION_ADDR 274 // 2*136 + 1 + 1
#define SENSOR_D_CALIBRATION_ADDR 410 // 3*136 + 1 + 1
//Each sensor calibration block is 136 bytes

#define ACTIVE_MIDI_CHANNEL_A_ADDR 546
#define ACTIVE_MIDI_CHANNEL_B_ADDR 547
#define ACTIVE_MIDI_CHANNEL_C_ADDR 548
#define ACTIVE_MIDI_CHANNEL_D_ADDR 549

#define OCTAVE_A_ADDR 550
#define OCTAVE_B_ADDR 551
#define OCTAVE_C_ADDR 552
#define OCTAVE_D_ADDR 553