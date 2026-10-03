#include "SampleEdit.h"
#include <algorithm>
#include <cmath>

int SampleEdit::msToSamples(double ms, double sampleRate)
{
    if (ms <= 0.0 || sampleRate <= 0.0) {
        return 0;
    }
    return (int) std::llround(ms * sampleRate / 1000.0);
}

double SampleEdit::getShortestSampleMs(const std::map<int, SampleData>& samples)
{
    bool any = false;
    double shortest = 0.0;

    for (const auto& entry : samples) {
        const SampleData& data = entry.second;
        const int numSamples = data.audioBuffer.getNumSamples();
        if (data.sampleRate <= 0.0 || numSamples <= 0) {
            return 0.0;
        }

        const double ms = numSamples * 1000.0 / data.sampleRate;
        if (!any || ms < shortest) {
            shortest = ms;
        }
        any = true;
    }

    return any ? shortest : 0.0;
}

SampleData SampleEdit::render(const SampleData& source) const
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

    int startSample = msToSamples(startMs, source.sampleRate);
    startSample = juce::jlimit(0, numSamples - 1, startSample);

    const int outSamples = numSamples - startSample;
    out.numChannels = numChannels;
    out.audioBuffer.setSize(numChannels, outSamples);
    for (int ch = 0; ch < numChannels; ch++) {
        out.audioBuffer.copyFrom(ch, 0, source.audioBuffer, ch, startSample, outSamples);
    }

    if (source.sampleRate > 0.0) {
        juce::ADSR envelope;
        envelope.setSampleRate(source.sampleRate);
        envelope.setParameters(getAdsrParameters((float) (attackMs / 1000.0), 0.0f));
        envelope.noteOn();
        envelope.applyEnvelopeToBuffer(out.audioBuffer, 0, outSamples);
    }

    return out;
}
