# SpectrumSpinor Changelog

## Latest Updates

### RGB Update Optimization (2024)
**Problem:** When switching to RGB troubleshoot mode, the display would show 0,0,0 values for all sensors if no color changes had occurred since startup.

**Solution:** Implemented `requestRGBUpdate` flag system that forces immediate RGB readings when switching from color name mode to RGB value mode.

**Implementation Details:**
- Added `requestRGBUpdate` boolean flag to MenuManager class
- Modified troubleshoot menu handlers to detect mode transitions (color → RGB) and set flag
- Updated main.cpp sensor loop to check flag and force RGB readings when set
- Flag is automatically cleared after readings are updated

**Files Modified:**
- `src/MenuManager.h`: Added requestRGBUpdate flag declaration
- `src/MenuManager.cpp`: Added flag setting in troubleshootMenuCW() and troubleshootMenuCCW()
- `src/main.cpp`: Added flag checking and RGB force update logic

**User Experience Improvement:** 
RGB troubleshoot mode now immediately shows current sensor readings instead of default 0,0,0 values when switching modes.

### Previous Updates
- Interrupt-based input system with toggle debouncing
- MIDI Grid menu with per-sensor channel assignment (A/B/C/D)
- Complete Calibration menu framework
- Dual-mode troubleshoot display (color names vs RGB values)
- Four-sensor initialization via TCA9548A multiplexer
- Comprehensive debug output system with conditional compilation