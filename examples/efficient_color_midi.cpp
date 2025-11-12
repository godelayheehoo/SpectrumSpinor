/*
 * Example of efficient color-based MIDI note generation
 * 
 * This example shows how to use the new enum-based color system
 * for maximum efficiency when generating MIDI notes.
 */

#include "ColorHelper.h"
#include "ScaleManager.h"

// Global objects
ColorHelper colorSensor;
ScaleManager scaleManager;

void setup() {
    Serial.begin(115200);
    
    // Initialize color sensor
    if (!colorSensor.begin()) {
        Serial.println("Failed to initialize color sensor!");
        return;
    }
    
    // Set up scale (C Major in 4th octave)
    scaleManager.setScale(ScaleManager::MAJOR);
    scaleManager.setRootNote(RootNote::C4); // C4 = MIDI note 60
}

void loop() {
    // EFFICIENT WAY - Get color as enum (no string comparisons!)
    Color detectedColor = colorSensor.getCurrentColorEnum();
    
    if (detectedColor != Color::UNKNOWN) {
        // Convert color directly to MIDI note (very fast!)
        uint8_t midiNote = scaleManager.colorToMIDINote(detectedColor);
        
        // Send MIDI note or do whatever you need with it
        Serial.print("Detected color: ");
        Serial.print(colorToString(detectedColor));  // Only convert to string for display
        Serial.print(" -> MIDI note: ");
        Serial.println(midiNote);
        
        // Here you would send the MIDI note to your synthesizer
        // sendMIDINote(midiNote);
    }
    
    delay(100); // Adjust timing as needed
}

/*
 * Performance comparison:
 * 
 * OLD WAY (string-based):
 * 1. Get color as string: getCurrentColor() 
 * 2. String comparison loop in colorToMIDINote(const char*)
 * 3. Multiple strcmp() calls = SLOW
 * 
 * NEW WAY (enum-based):
 * 1. Get color as enum: getCurrentColorEnum()
 * 2. Direct array lookup in colorToMIDINote(Color)
 * 3. Simple integer comparisons = FAST
 * 
 * The enum approach is roughly 10-50x faster depending on which
 * color gets detected (worst case with strings is when the color
 * is the last one in the list).
 */