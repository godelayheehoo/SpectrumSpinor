#include "ColorInfo.h"
#include "ColorHelper.h"
#include <cstring>

// Single-definition of the global sensor calibration storage referenced by externs
SensorCalibration sensorCalibrations[4];

// Initialize entries with sensible defaults at program start to avoid writing
// uninitialized data into EEPROM later on.
struct _SensorCalInit {
	_SensorCalInit() {
		for (int i = 0; i < 4; ++i) {
			sensorCalibrations[i].numColors = NUM_COLORS;
			sensorCalibrations[i].isCalibrated = false;
			// Copy default color database into each sensor's calibration
			for (int j = 0; j < NUM_COLORS; ++j) {
				sensorCalibrations[i].calibrationDatabase[j] = colorCalibrationDefaultDatabase[j];
			}
			// Set sensor name A/B/C/D
			char name[2] = {'A' + (char)i, '\0'};
			std::memcpy(sensorCalibrations[i].sensorName, name, 2);
		}
	}
} _sensorCalInit;
