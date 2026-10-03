#include "RecordingManager.h"

void RecordingManager::prepareToRecord(double sampleRate, int numChannels, double maxDurationSeconds)
{
    currentSampleRate = sampleRate;
    currentNumChannels = numChannels;
    maxSamples = static_cast<int>(sampleRate * maxDurationSeconds) + 1024;
    buffer.setSize(numChannels, maxSamples);
    buffer.clear();
    writePosition.store(0);
}

void RecordingManager::startRecording()
{
    writePosition.store(0);
    recording.store(true);
}

void RecordingManager::stopRecording()
{
    recording.store(false);
}

void RecordingManager::recordBlock(const float* const* inputChannelData, int numChannels, int numSamples)
{
    if (!recording.load()) {
        return;
    }

    int pos = writePosition.load();
    int samplesToWrite = juce::jmin(numSamples, maxSamples - pos);

    if (samplesToWrite <= 0) {
        return;
    }

    int channelsToWrite = juce::jmin(numChannels, buffer.getNumChannels());
    for (int ch = 0; ch < channelsToWrite; ++ch) {
        if (inputChannelData[ch] != nullptr) {
            buffer.copyFrom(ch, pos, inputChannelData[ch], samplesToWrite);
        }
    }

    writePosition.store(pos + samplesToWrite);
}

juce::AudioBuffer<float> RecordingManager::getRecordedBuffer() const
{
    int numSamples = writePosition.load();
    if (numSamples <= 0) {
        return {};
    }

    juce::AudioBuffer<float> result(buffer.getNumChannels(), numSamples);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        result.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    return result;
}
