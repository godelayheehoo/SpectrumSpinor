#include "ScaleManager.h"
#include "ColorEnum.h"

// Special MIDI note value for white (background) color - indicates "turn off notes"
const uint8_t MIDI_NOTE_OFF = 0;  // Use note 0 on invalid channel, or we could use 255 as a special flag

// Major scale intervals (semitones from root)
// Maps to Color enum order: PINK, ORANGE, BLUE, YELLOW, GREEN, RED, PURPLE, BROWN, WHITE
// C=0, D=2, E=4, F=5, G=7, A=9, B=11, C(next)=12, WHITE=special
static const uint8_t majorScaleOffsets[] = {
    0, 2, 4, 5, 7, 9, 11, 12, MIDI_NOTE_OFF, MIDI_NOTE_OFF  // Maps to: C, D, E, F, G, A, B, C, OFF
};

static const uint8_t minorScaleOffsets[] = {
    0, 2, 3, 5, 7, 8, 10, 12, MIDI_NOTE_OFF, MIDI_NOTE_OFF  // Maps to: C, D, D#, F, G, G#, A#, C, OFF
};

ScaleManager::ScaleManager(ScaleType initialScale, uint8_t initialOctave, uint8_t initialRootNote)
    : currentScale(initialScale), baseOctave(initialOctave), rootNote(initialRootNote) {
}

uint8_t ScaleManager::colorToMIDINote(Color color) {
    // Special case: WHITE color means "turn off notes"
    if (color == Color::WHITE) {
        return MIDI_NOTE_OFF;  // Special value indicating note-off
    }
    
    int colorIndex = colorToIndex(color);
    
    if (colorIndex == -1) {
        // Unknown color, return root note as default
        return rootNote;
    }
    
    uint8_t offset = getScaleOffset(colorIndex);
    
    // For white, offset will be MIDI_NOTE_OFF, so return it directly
    if (offset == MIDI_NOTE_OFF) {
        return MIDI_NOTE_OFF;
    }
    
    uint8_t midiNote = rootNote + offset;
    
    // Ensure we stay within valid MIDI range (0-127)
    if (midiNote > 127) {
        midiNote = 127;
    }
    
    return midiNote;
}

// Backwards compatibility method - less efficient due to string comparison
uint8_t ScaleManager::colorToMIDINote(const char* colorName) {
    // Convert string to enum first, then use efficient enum method
    for (uint8_t i = 0; i < getColorCount(); i++) {
        Color color = indexToColor(i);
        if (strcmp(colorToString(color), colorName) == 0) {
            return colorToMIDINote(color);
        }
    }
    
    // Unknown color, return root note as default
    return rootNote;
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

bool ScaleManager::isNoteOffColor(Color color) const {
    return color == Color::WHITE;
}

uint8_t ScaleManager::getScaleOffset(int colorIndex) {
    if (colorIndex < 0 || colorIndex >= getColorCount()) {
        return 0; // Default to root note
    }
    
    switch (currentScale) {
        case MAJOR:
            return majorScaleOffsets[colorIndex];
        case MINOR:
            return minorScaleOffsets[colorIndex];
        default:
            return 0;
    }
}