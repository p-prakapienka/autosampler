#include "SamplePreviewManager.h"

void SamplePreviewManager::setSamples(const std::map<int, SampleData>* samples)
{
    activeVoices.clear();
    samplesPtr = samples;
}

void SamplePreviewManager::setSampleRate(double rate)
{
    currentSampleRate = rate;
}

void SamplePreviewManager::noteOn(int midiNote)
{
    if (samplesPtr == nullptr) {
        return;
    }

    auto it = samplesPtr->find(midiNote);
    if (it == samplesPtr->end()) {
        return;
    }

    // If already playing this note, restart it
    auto voiceIt = activeVoices.find(midiNote);
    if (voiceIt != activeVoices.end()) {
        voiceIt->second->startPlayback(it->second.audioBuffer, it->second.sampleRate);
        return;
    }

    // Evict oldest voice if at max
    if (static_cast<int>(activeVoices.size()) >= maxVoices) {
        evictOldestVoice();
    }

    auto voice = std::make_unique<SamplePlaybackBuffer>();
    voice->startPlayback(it->second.audioBuffer, it->second.sampleRate);
    activeVoices[midiNote] = std::move(voice);
}

void SamplePreviewManager::noteOff(int midiNote)
{
    auto it = activeVoices.find(midiNote);
    if (it != activeVoices.end()) {
        it->second->stopPlayback();
    }
}

void SamplePreviewManager::processBlock(float* const* outputChannelData, int numOutputChannels, int numSamples)
{
    // Process all active voices, remove finished ones
    for (auto it = activeVoices.begin(); it != activeVoices.end();) {
        it->second->readBlock(outputChannelData, numOutputChannels, numSamples);

        if (!it->second->isPlaying()) {
            it = activeVoices.erase(it);
        } else {
            ++it;
        }
    }
}

bool SamplePreviewManager::hasSamples() const
{
    return samplesPtr != nullptr && !samplesPtr->empty();
}

void SamplePreviewManager::evictOldestVoice()
{
    if (activeVoices.empty()) {
        return;
    }

    // Remove the first voice (lowest note number as a simple heuristic)
    activeVoices.erase(activeVoices.begin());
}
