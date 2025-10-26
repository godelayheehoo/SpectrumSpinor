#pragma once
#include <Arduino.h>

class ScaleManager {
public:
    enum ScaleType {
        MAJOR
        // Future: MINOR, PENTATONIC, BLUES, etc.
    };
    
    ScaleManager(ScaleType initialScale = MAJOR, uint8_t initialOctave = 4, uint8_t initialRootNote = 60); // Default to C4
    
    // Get MIDI note number for a detected color
    uint8_t colorToMIDINote(const char* colorName);
    
    // Scale/key management
    void setScale(ScaleType scale);
    void setOctave(uint8_t octave);
    void setRootNote(uint8_t rootNote); // MIDI note number for root (60 = C4)
    
    // Getters
    ScaleType getCurrentScale() const;
    uint8_t getCurrentOctave() const;
    uint8_t getRootNote() const;
    
    // Get scale name as string for display
    const char* getScaleName() const;
    
private:
    ScaleType currentScale;
    uint8_t baseOctave;
    uint8_t rootNote;
    
    // Map color name to color index (0-7)
    int getColorIndex(const char* colorName);
    
    // Get MIDI note offset for color index in current scale
    uint8_t getScaleOffset(int colorIndex);
};