#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../Sampling/SampleEdit.h"
#include <atomic>

class SamplePlaybackBuffer
{
public:
    SamplePlaybackBuffer() = default;

    void startPlayback(const juce::AudioBuffer<float>& source, double sourceSampleRate)
    {
        sourceBuffer = &source;
        const int numSamples = source.getNumSamples();
        if (numSamples <= 0 || sourceSampleRate <= 0.0) {
            playing.store(false);
            return;
        }

        readPosition = juce::jlimit(0, numSamples - 1, getStartSample());

        const float attackSeconds = (float) getAttackSamples() / (float) sourceSampleRate;
        envelope.reset();
        envelope.setSampleRate(sourceSampleRate);
        envelope.setParameters(SampleEdit::getAdsrParameters(attackSeconds, releaseSeconds));
        envelope.noteOn();

        playing.store(true);
    }

    void stopPlayback()
    {
        if (playing.load()) {
            envelope.noteOff();
        }
    }

    void readBlock(float* const* outputChannelData, int numOutputChannels, int numSamples)
    {
        if (!playing.load() || sourceBuffer == nullptr) {
            return;
        }

        const int sourceChannels = sourceBuffer->getNumChannels();
        const int sourceSamples = sourceBuffer->getNumSamples();
        if (sourceChannels <= 0) {
            return;
        }

        for (int i = 0; i < numSamples; i++) {
            if (readPosition >= sourceSamples || !envelope.isActive()) {
                playing.store(false);
                return;
            }

            const float gain = envelope.getNextSample();

            for (int ch = 0; ch < numOutputChannels; ch++) {
                const int srcCh = juce::jmin(ch, sourceChannels - 1);
                outputChannelData[ch][i] += sourceBuffer->getSample(srcCh, readPosition) * gain;
            }

            ++readPosition;
        }
    }

    void setStartSample(int newStartSample) { startSample = newStartSample; }
    int getStartSample() const { return startSample; }
    void setAttackSamples(int newAttackSamples) { attackSamples = newAttackSamples; }
    int getAttackSamples() const { return attackSamples; }
    bool isPlaying() const { return playing.load(); }

private:
    static constexpr float releaseSeconds = 0.05f;

    const juce::AudioBuffer<float>* sourceBuffer = nullptr;
    juce::ADSR envelope;
    int startSample = 0;
    int attackSamples = 0;
    int readPosition = 0;

    std::atomic<bool> playing { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePlaybackBuffer)
};
