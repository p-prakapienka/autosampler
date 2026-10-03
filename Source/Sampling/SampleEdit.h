#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../AutosamplerState.h"

// Start time and attack for one sampling run. Raw SampleData buffers are not modified.
class SampleEdit
{
public:
    static constexpr double defaultAttackMs = 5.0;

    // Decay is 0 and sustain is 1. Release is 0 when baking a file, and the preview fade when playing.
    static juce::ADSR::Parameters getAdsrParameters(float attackSeconds, float releaseSeconds)
    {
        return { attackSeconds, 0.0f, 1.0f, releaseSeconds };
    }

    static double getShortestSampleMs(const std::map<int, SampleData>& samples);
    static int msToSamples(double ms, double sampleRate);

    // Message thread only. Returns a copy that starts at startMs with the attack applied.
    SampleData render(const SampleData& source) const;

    void setStartMs(double ms) { startMs = ms; }
    void setAttackMs(double ms) { attackMs = ms; }
    double getStartMs() const { return startMs; }
    double getAttackMs() const { return attackMs; }

private:
    double startMs = 0.0;
    double attackMs = defaultAttackMs;
};
