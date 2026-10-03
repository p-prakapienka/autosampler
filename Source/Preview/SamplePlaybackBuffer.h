#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

class SamplePlaybackBuffer
{
public:
    SamplePlaybackBuffer() = default;

    void startPlayback(const juce::AudioBuffer<float>& source, double sourceSampleRate)
    {
        sourceBuffer = &source;
        readPosition = 0;
        fadingOut = false;
        fadeGain = 1.0f;
        fadeOutSamples = static_cast<int>(sourceSampleRate * 0.05); // 50ms
        playing.store(true);
    }

    void stopPlayback()
    {
        if (playing.load()) {
            fadingOut = true;
        }
    }

    bool isPlaying() const { return playing.load(); }

    void readBlock(float* const* outputChannelData, int numOutputChannels, int numSamples)
    {
        if (!playing.load() || sourceBuffer == nullptr) {
            return;
        }

        int sourceChannels = sourceBuffer->getNumChannels();
        int sourceSamples = sourceBuffer->getNumSamples();

        for (int i = 0; i < numSamples; i++) {
            if (readPosition >= sourceSamples) {
                playing.store(false);
                return;
            }

            float gain = 1.0f;
            if (fadingOut) {
                gain = fadeGain;
                if (fadeOutSamples > 0) {
                    fadeGain -= 1.0f / static_cast<float>(fadeOutSamples);
                } else {
                    fadeGain = 0.0f;
                }

                if (fadeGain <= 0.0f) {
                    playing.store(false);
                    return;
                }
            }

            for (int ch = 0; ch < numOutputChannels; ch++) {
                int srcCh = juce::jmin(ch, sourceChannels - 1);
                outputChannelData[ch][i] += sourceBuffer->getSample(srcCh, readPosition) * gain;
            }

            ++readPosition;
        }
    }

private:
    const juce::AudioBuffer<float>* sourceBuffer = nullptr;
    int readPosition = 0;
    std::atomic<bool> playing { false };
    bool fadingOut = false;
    float fadeGain = 1.0f;
    int fadeOutSamples = 2205; // default 50ms at 44.1kHz

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePlaybackBuffer)
};
