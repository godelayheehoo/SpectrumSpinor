# SpectrumSpinor Changelog

## Latest Updates

### RGB Update Optimization (2024)
**Problem:** When switching to RGB troubleshoot mode, the display would show 0,0,0 values for all sensors if no color changes had occurred since startup.

**Solution:** Implemented `requestRGBUpdate` flag system that forces immediate RGB readings when switching from color name mode to RGB value mode.

**Implementation Details:**
- Added `requestRGBUpdate` boolean flag to MenuManager class
- Modified troubleshoot menu encoder button handler to detect mode transitions (color → RGB) and set flag
- Added dedicated RGB update logic at start of main loop to update ALL sensors when flag is set
- Flag is automatically cleared after all sensor readings are updated

**Button Behavior in Troubleshoot Mode:**
- **Encoder Rotation (CW/CCW):** No action (disabled)
- **Encoder Button:** Switches between color name mode and RGB value mode
- **Back Button:** Returns to main menu
- **Confirm Button:** Returns to main menu

**Files Modified:**
- `src/MenuManager.h`: Added requestRGBUpdate flag declaration
- `src/MenuManager.cpp`: Fixed button assignments and added flag setting in troubleshootMenuEncoderButton()
- `src/main.cpp`: Added flag checking and RGB force update logic

**User Experience Improvement:** 
RGB troubleshoot mode now immediately shows current sensor readings instead of default 0,0,0 values when switching modes, with correct button assignments.

### Previous Updates
- Interrupt-based input system with toggle debouncing
- MIDI Grid menu with per-sensor channel assignment (A/B/C/D)
- Complete Calibration menu framework
- Dual-mode troubleshoot display (color names vs RGB values)
- Four-sensor initialization via TCA9548A multiplexer
- Comprehensive debug output system with conditional compilation