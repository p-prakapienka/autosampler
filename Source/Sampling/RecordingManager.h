#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

class RecordingManager
{
public:
    RecordingManager() = default;
    ~RecordingManager() = default;

    void prepareToRecord(double sampleRate, int numChannels, double maxDurationSeconds);
    void startRecording();
    void stopRecording();
    bool isCurrentlyRecording() const { return recording.load(); }

    void recordBlock(const float* const* inputChannelData, int numChannels, int numSamples);

    juce::AudioBuffer<float> getRecordedBuffer() const;
    int getRecordedSampleCount() const { return writePosition.load(); }
    double getCurrentSampleRate() const { return currentSampleRate; }
    int getCurrentNumChannels() const { return currentNumChannels; }

private:
    juce::AudioBuffer<float> buffer;
    std::atomic<int> writePosition { 0 };
    std::atomic<bool> recording { false };
    double currentSampleRate = 44100.0;
    int currentNumChannels = 2;
    int maxSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingManager)
};
