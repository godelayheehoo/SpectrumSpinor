#pragma once
//EEPROM magic number to indicate valid stored settings
#define EEPROM_MAGIC_ADDRESS 0x00 // don't change this one
#define EEPROM_MAGIC_VALUE 9 //do change this one

/////////////// Calibration Addresses///////////
//rDarks
#define SENSOR_A_RDARK_ADDR 2
#define SENSOR_A_GDARK_ADDR 6
#define SENSOR_A_BDARK_ADDR 10

#define SENSOR_B_RDARK_ADDR 14
#define SENSOR_B_GDARK_ADDR 18
#define SENSOR_B_BDARK_ADDR 22

#define SENSOR_C_RDARK_ADDR 26
#define SENSOR_C_GDARK_ADDR 30
#define SENSOR_C_BDARK_ADDR 34

#define SENSOR_D_RDARK_ADDR 38
#define SENSOR_D_GDARK_ADDR 42
#define SENSOR_D_BDARK_ADDR 46

//W values -- NOT gains, which need to be recalculated
#define SENSOR_A_RW_ADDR 50
#define SENSOR_A_GW_ADDR 54
#define SENSOR_A_BW_ADDR 58

#define SENSOR_B_RW_ADDR 62
#define SENSOR_B_GW_ADDR 66
#define SENSOR_B_BW_ADDR 70

#define SENSOR_C_RW_ADDR 74
#define SENSOR_C_GW_ADDR 78
#define SENSOR_C_BW_ADDR 82

#define SENSOR_D_RW_ADDR 86
#define SENSOR_D_GW_ADDR 90
#define SENSOR_D_BW_ADDR 94 

//Color calibrations
#define COLOR_BLOCK_SIZE 12


// enum class Color : uint8_t {
//     RED = 0,
//     GREEN = 1,
//     PURPLE = 2,
//     BLUE = 3,
//     ORANGE = 4,
//     YELLOW = 5,
//     SILVER = 6,
//     WHITE = 7,      // Special color - ignore this
//     UNKNOWN = 255   // Special value for unrecognized colors
// };
//Sensor A
#define SENSOR_A_RED_CAL_ADDR 98
#define SENSOR_A_GREEN_CAL_ADDR 110
#define SENSOR_A_PURPLE_CAL_ADDR 122
#define SENSOR_A_BLUE_CAL_ADDR 134
#define SENSOR_A_ORANGE_CAL_ADDR 146
#define SENSOR_A_YELLOW_CAL_ADDR 158
#define SENSOR_A_SILVER_CAL_ADDR 170
#define SENSOR_A_WHITE_CAL_ADDR 182

//Sensor B
#define SENSOR_B_RED_CAL_ADDR 194
#define SENSOR_B_GREEN_CAL_ADDR 206
#define SENSOR_B_PURPLE_CAL_ADDR 218
#define SENSOR_B_BLUE_CAL_ADDR 230
#define SENSOR_B_ORANGE_CAL_ADDR 242
#define SENSOR_B_YELLOW_CAL_ADDR 254
#define SENSOR_B_SILVER_CAL_ADDR 266
#define SENSOR_B_WHITE_CAL_ADDR 278
//Sensor C
#define SENSOR_C_RED_CAL_ADDR 290
#define SENSOR_C_GREEN_CAL_ADDR 302
#define SENSOR_C_PURPLE_CAL_ADDR 314
#define SENSOR_C_BLUE_CAL_ADDR 326
#define SENSOR_C_ORANGE_CAL_ADDR 338
#define SENSOR_C_YELLOW_CAL_ADDR 350
#define SENSOR_C_SILVER_CAL_ADDR 362
#define SENSOR_C_WHITE_CAL_ADDR 374
//Sensor D
#define SENSOR_D_RED_CAL_ADDR 386
#define SENSOR_D_GREEN_CAL_ADDR 398
#define SENSOR_D_PURPLE_CAL_ADDR 410
#define SENSOR_D_BLUE_CAL_ADDR 422
#define SENSOR_D_ORANGE_CAL_ADDR 434
#define SENSOR_D_YELLOW_CAL_ADDR 446
#define SENSOR_D_SILVER_CAL_ADDR 458
#define SENSOR_D_WHITE_CAL_ADDR 470

/////////////// Menu Addresses///////////

#define ACTIVE_MIDI_CHANNEL_A_ADDR 482
#define ACTIVE_MIDI_CHANNEL_B_ADDR 483
#define ACTIVE_MIDI_CHANNEL_C_ADDR 484
#define ACTIVE_MIDI_CHANNEL_D_ADDR 485

#define OCTAVE_A_ADDR 486
#define OCTAVE_B_ADDR 487
#define OCTAVE_C_ADDR 488
#define OCTAVE_D_ADDR 489

#define SCALE_ADDR 490
#define ROOT_NOTE_ADDR 491