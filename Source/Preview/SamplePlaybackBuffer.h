#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../AutosamplerState.h"
#include <atomic>
#include <algorithm>
#include <cmath>

// Plays one sample from a start point with a juce::ADSR attack. Raw buffers are not modified.
class SamplePlaybackBuffer
{
public:
    SamplePlaybackBuffer() = default;

    static int msToSamples(double ms, double sampleRate)
    {
        if (ms <= 0.0 || sampleRate <= 0.0) {
            return 0;
        }
        return (int) std::llround(ms * sampleRate / 1000.0);
    }

    // Message thread only. Plays this voice into a new buffer and does not release, so the file matches preview without the note-off fade.
    SampleData render(const SampleData& source)
    {
        SampleData out;
        out.midiNote = source.midiNote;
        out.sampleRate = source.sampleRate;

        const int numSamples = source.audioBuffer.getNumSamples();
        const int numChannels = source.audioBuffer.getNumChannels();

        if (numSamples <= 0 || numChannels <= 0) {
            out.numChannels = std::max(1, numChannels);
            out.audioBuffer.setSize(out.numChannels, 0);
            return out;
        }

        const int start = juce::jlimit(0, numSamples - 1, getStartSample());
        const int outSamples = numSamples - start;
        out.numChannels = numChannels;
        out.audioBuffer.setSize(numChannels, outSamples);

        startPlayback(source.audioBuffer, source.sampleRate);
        if (isPlaying()) {
            out.audioBuffer.clear();
            readBlock(out.audioBuffer.getArrayOfWritePointers(), numChannels, outSamples);
            sourceBuffer = nullptr;
            playing.store(false);
        } else {
            for (int ch = 0; ch < numChannels; ch++) {
                out.audioBuffer.copyFrom(ch, 0, source.audioBuffer, ch, start, outSamples);
            }
        }

        return out;
    }

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
        envelope.setParameters(getAdsrParameters(attackSeconds, releaseSeconds));
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

    // Decay is 0 and sustain is 1. Release is the preview fade after note-off. Export never releases.
    static juce::ADSR::Parameters getAdsrParameters(float attackSeconds, float releaseSeconds)
    {
        return { attackSeconds, 0.0f, 1.0f, releaseSeconds };
    }

    const juce::AudioBuffer<float>* sourceBuffer = nullptr;
    juce::ADSR envelope;
    int startSample = 0;
    int attackSamples = 0;
    int readPosition = 0;

    std::atomic<bool> playing { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePlaybackBuffer)
};
