#pragma once
#include <Arduino.h>
#include "ColorEnum.h"

class ScaleManager {
public:
    enum ScaleType {
        MAJOR,
        MINOR,
        // Future: MINOR, PENTATONIC, BLUES, etc.
    };
    
    ScaleManager(ScaleType initialScale = MAJOR, uint8_t initialOctave = 4, uint8_t initialRootNote = 60); // Default to C4
    
    // Get MIDI note number for a detected color (EFFICIENT - use this!)
    uint8_t colorToMIDINote(Color color);
    
    // Get MIDI note number for a detected color (backwards compatibility - slower)
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
    
    // Check if a color should trigger note-off (i.e., white background)
    bool isNoteOffColor(Color color) const;
    
    // Special MIDI note value for note-off colors
    static const uint8_t MIDI_NOTE_OFF = 255;
    
private:
    ScaleType currentScale;
    uint8_t baseOctave;
    uint8_t rootNote;
    
    // Get MIDI note offset for color index in current scale
    uint8_t getScaleOffset(int colorIndex);
};