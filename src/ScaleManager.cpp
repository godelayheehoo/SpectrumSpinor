#include "ScaleManager.h"
#include <string.h>

// Color names in order from ColorInfo.h
static const char* colorNames[] = {
    "pink", "orange", "blue", "yellow", "green", "red", "purple", "brown"
};
static const int numColors = sizeof(colorNames) / sizeof(colorNames[0]);

// Major scale intervals (semitones from root)
// C=0, D=2, E=4, F=5, G=7, A=9, B=11, C(next)=12
static const uint8_t majorScaleOffsets[] = {
    0, 2, 4, 5, 7, 9, 11, 12  // Maps to: C, D, E, F, G, A, B, C
};

ScaleManager::ScaleManager(ScaleType initialScale, uint8_t initialOctave, uint8_t initialRootNote)
    : currentScale(initialScale), baseOctave(initialOctave), rootNote(initialRootNote) {
}

uint8_t ScaleManager::colorToMIDINote(const char* colorName) {
    int colorIndex = getColorIndex(colorName);
    
    if (colorIndex == -1) {
        // Unknown color, return root note as default
        return rootNote;
    }
    
    uint8_t offset = getScaleOffset(colorIndex);
    uint8_t midiNote = rootNote + offset;
    
    // Ensure we stay within valid MIDI range (0-127)
    if (midiNote > 127) {
        midiNote = 127;
    }
    
    return midiNote;
}

void ScaleManager::setScale(ScaleType scale) {
    currentScale = scale;
}

void ScaleManager::setOctave(uint8_t octave) {
    // Adjust root note to new octave (each octave is 12 semitones)
    uint8_t noteWithinOctave = rootNote % 12;
    rootNote = (octave * 12) + noteWithinOctave;
    baseOctave = octave;
}

void ScaleManager::setRootNote(uint8_t newRootNote) {
    rootNote = newRootNote;
    baseOctave = newRootNote / 12;
}

ScaleManager::ScaleType ScaleManager::getCurrentScale() const {
    return currentScale;
}

uint8_t ScaleManager::getCurrentOctave() const {
    return baseOctave;
}

uint8_t ScaleManager::getRootNote() const {
    return rootNote;
}

const char* ScaleManager::getScaleName() const {
    switch (currentScale) {
        case MAJOR:
            return "Major";
        default:
            return "Unknown";
    }
}

int ScaleManager::getColorIndex(const char* colorName) {
    for (int i = 0; i < numColors; i++) {
        if (strcmp(colorNames[i], colorName) == 0) {
            return i;
        }
    }
    return -1; // Color not found
}

uint8_t ScaleManager::getScaleOffset(int colorIndex) {
    if (colorIndex < 0 || colorIndex >= numColors) {
        return 0; // Default to root note
    }
    
    switch (currentScale) {
        case MAJOR:
            return majorScaleOffsets[colorIndex];
        default:
            return 0;
    }
}