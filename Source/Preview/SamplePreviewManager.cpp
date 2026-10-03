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

void SamplePreviewManager::startVoice(SamplePlaybackBuffer& voice, const SampleData& data, double startMs, double attackMs)
{
    voice.setStartSample(SamplePlaybackBuffer::msToSamples(startMs, data.sampleRate));
    voice.setAttackSamples(SamplePlaybackBuffer::msToSamples(attackMs, data.sampleRate));
    voice.startPlayback(data.audioBuffer, data.sampleRate);
}

void SamplePreviewManager::noteOn(int midiNote, double startMs, double attackMs)
{
    if (samplesPtr == nullptr) {
        return;
    }

    auto it = samplesPtr->find(midiNote);
    if (it == samplesPtr->end()) {
        return;
    }

    auto voiceIt = activeVoices.find(midiNote);
    if (voiceIt != activeVoices.end()) {
        startVoice(*voiceIt->second, it->second, startMs, attackMs);
        return;
    }

    if (static_cast<int>(activeVoices.size()) >= maxVoices) {
        evictOldestVoice();
    }

    auto voice = std::make_unique<SamplePlaybackBuffer>();
    startVoice(*voice, it->second, startMs, attackMs);
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
