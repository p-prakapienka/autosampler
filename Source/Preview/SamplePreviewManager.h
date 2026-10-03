#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../AutosamplerState.h"
#include "SamplePlaybackBuffer.h"
#include <map>
#include <memory>

class SampleEdit;

class SamplePreviewManager
{
public:
    SamplePreviewManager() = default;

    void noteOn(int midiNote, const SampleEdit& edit);
    void noteOff(int midiNote);

    void processBlock(float* const* outputChannelData, int numOutputChannels, int numSamples);

    void setSamples(const std::map<int, SampleData>* samples);
    void setSampleRate(double rate);
    bool hasSamples() const;

private:
    void evictOldestVoice();
    void startVoice(SamplePlaybackBuffer& voice, const SampleData& data, const SampleEdit& edit);

    const std::map<int, SampleData>* samplesPtr = nullptr;
    std::map<int, std::unique_ptr<SamplePlaybackBuffer>> activeVoices;
    double currentSampleRate = 44100.0;

    static constexpr int maxVoices = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePreviewManager)
};
