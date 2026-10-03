#pragma once

#include <juce_core/juce_core.h>
#include "MidiOutputManager.h"
#include "RecordingManager.h"
#include "../AutosamplerState.h"
#include <functional>
#include <map>

class SamplerEngine : private juce::Timer
{
public:
    SamplerEngine(MidiOutputManager& midiOut, RecordingManager& recorder);
    ~SamplerEngine() override;

    using StatusCallback = std::function<void(const juce::String& status, int currentNote, int totalNotes)>;
    using CompletionCallback = std::function<void(const std::map<int, SampleData>& samples)>;

    void startSampling(int startNote, int endNote, double noteDuration,
                       double releaseDuration, int midiChannel, int velocity);
    void stopSampling();

    void setStatusCallback(StatusCallback callback) { statusCallback = std::move(callback); }
    void setCompletionCallback(CompletionCallback callback) { completionCallback = std::move(callback); }
    bool isSampling() const { return sampling; }
    int getCurrentNote() const { return currentNote; }

private:
    void timerCallback() override;
    void advanceToNextNote();
    void storeCurrentSample();
    void finishSampling();

    MidiOutputManager& midiOutputManager;
    RecordingManager& recordingManager;

    bool sampling = false;
    int startNote = 0;
    int endNote = 127;
    int currentNote = 0;
    double noteDuration = 3.0;
    double releaseDuration = 1.0;
    int midiChannel = 1;
    int velocity = 100;

    enum class NotePhase { SendingNoteOn, Recording, SendingNoteOff, Releasing, Storing };
    NotePhase currentPhase = NotePhase::SendingNoteOn;
    double recordingElapsed = 0.0;
    double releaseElapsed = 0.0;

    std::map<int, SampleData> collectedSamples;
    StatusCallback statusCallback;
    CompletionCallback completionCallback;

    static constexpr int timerIntervalMs = 50;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerEngine)
};
