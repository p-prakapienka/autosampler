#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <map>

enum class ExportFormat
{
    SF2,
    SFZ,
    DecentSampler
};

struct SampleData
{
    juce::uint8 midiNote = 0;
    juce::AudioBuffer<float> audioBuffer;
    double sampleRate = 44100.0;
    int numChannels = 2;
};

struct AutosamplerState
{
    int startNote = 48;      // C3
    int endNote = 72;        // C5
    double noteDuration = 3.0;
    double releaseDuration = 1.0;
    int midiChannel = 1;
    int velocity = 100;
    juce::String samplePackName = "MySamples";
    ExportFormat exportFormat = ExportFormat::SFZ;
    std::map<int, SampleData> samples;
    bool isRecording = false;
};

inline juce::String getMidiNoteName(int noteNumber)
{
    static const char* noteNames[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    int octave = (noteNumber / 12) - 1;
    int semitone = noteNumber % 12;
    return juce::String(noteNames[semitone]) + juce::String(octave);
}

inline juce::String getMidiNoteDisplayName(int noteNumber)
{
    static const char* noteNames[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    int octave = (noteNumber / 12) - 1;
    int semitone = noteNumber % 12;
    return juce::String(noteNames[semitone]) + juce::String(octave);
}
