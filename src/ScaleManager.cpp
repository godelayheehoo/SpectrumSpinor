#include "ScaleManager.h"
#include "ColorEnum.h"
#include "EEPROMAddresses.h"
#include <EEPROM.h>

// Special sentinel value used in scale offset tables to indicate "no note / note-off"
// Must NOT be 0 because 0 is a valid offset (root). Use 0xFF (255) as an out-of-band sentinel.
const uint8_t MIDI_NOTE_OFF = 0xFF;

// Major scale intervals (semitones from root)
// Maps to Color enum order: PINK, ORANGE, BLUE, YELLOW, GREEN, RED, PURPLE, BROWN, WHITE
// C=0, D=2, E=4, F=5, G=7, A=9, B=11, WHITE=special
static const uint8_t majorScaleOffsets[] = {
    0, 2, 4, 5, 7, 9, 11, MIDI_NOTE_OFF  // Maps to: C, D, E, F, G, A, B, OFF
};

static_assert(sizeof(majorScaleOffsets) / sizeof(majorScaleOffsets[0]) == NUM_COLORS,
              "majorScaleOffsets must have NUM_COLORS entries");

static const uint8_t minorScaleOffsets[] = {
    0, 2, 3, 5, 7, 8, 10, MIDI_NOTE_OFF  // Maps to: C, D, D#, F, G, G#, A#,  OFF
};

static_assert(sizeof(minorScaleOffsets) / sizeof(majorScaleOffsets[0]) == NUM_COLORS,
              "majorScaleOffsets must have NUM_COLORS entries");

ScaleManager::ScaleManager(ScaleType initialScale, uint8_t initialOctave, uint8_t initialRootNote)
    : currentScale(initialScale), 
    // baseOctave(initialOctave), 
    rootNote(initialRootNote) {
}

uint8_t ScaleManager::colorToMIDINote(Color color) {
    // Special case: WHITE color means "turn off notes"
    if (color == Color::WHITE) {
        return MIDI_NOTE_OFF;  // Special value indicating note-off
    }
    
    int colorIndex = colorToIndex(color);
    
    if (colorIndex == -1) {
        // Unknown color, return root note as default
        Serial.println("Got UNKNOWN color in colorToMIDINote");
        return rootNote;
    }
    
    uint8_t offset = getScaleOffset(colorIndex);
    
    // For white, offset will be MIDI_NOTE_OFF, so return it directly
    if (offset == MIDI_NOTE_OFF) {
        return MIDI_NOTE_OFF;
    }
    
    // Serial.print("Using root note ");
    // Serial.println(rootNote);
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

// void ScaleManager::setOctave(uint8_t octave) {
//     // Adjust root note to new octave (each octave is 12 semitones)
//     uint8_t noteWithinOctave = rootNote % 12;
//     rootNote = (octave * 12) + noteWithinOctave;
//     baseOctave = octave;
// }

void ScaleManager::setRootNote(RootNote newRootNote) {
    rootNote = static_cast<uint8_t>(newRootNote);
//     baseOctave = newRootNote / 12;
}

void ScaleManager::saveRootNote(){
    EEPROM.put(ROOT_NOTE_ADDR, static_cast<uint8_t>(rootNote));
}

ScaleManager::ScaleType ScaleManager::getCurrentScale() const {
    return currentScale;
}

// uint8_t ScaleManager::getCurrentOctave() const {
//     return baseOctave;
// }

uint8_t ScaleManager::getRootNote() const {
    return rootNote;
}

const char* ScaleManager::getScaleName() const {
    switch (currentScale) {
        case MAJOR:
            return "Major";
        case MINOR:
            return "Minor";
        default:
            return "Unknown";
    }
}

bool ScaleManager::isNoteOffColor(Color color) const {
    return color == Color::WHITE;
}

uint8_t ScaleManager::getScaleOffset(int colorIndex) {
    if (colorIndex < 0 || colorIndex >= getColorCount()) {
        Serial.print("got color index out of range! Idx: ");
        Serial.println(colorIndex);
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