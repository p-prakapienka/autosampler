#include "StartDetector.h"
#include <algorithm>
#include <cmath>
#include <vector>

int StartDetector::findOnsetSample(const SampleData& data)
{
    const auto& buffer = data.audioBuffer;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0 || data.sampleRate <= 0.0) {
        return 0;
    }

    float peak = 0.0f;
    for (int ch = 0; ch < numChannels; ch++) {
        const auto range = juce::FloatVectorOperations::findMinAndMax(
            buffer.getReadPointer(ch), numSamples);
        peak = std::max(peak, std::max(std::abs(range.getStart()), std::abs(range.getEnd())));
    }

    if (peak < 1.0e-4f) {
        return 0;
    }

    const float threshold = peak * 0.01f; // -40 dB relative to this note's peak
    const int hold = std::max(1, (int) std::llround(data.sampleRate * 0.001)); // 1 ms

    std::vector<const float*> channels((size_t) numChannels);
    for (int ch = 0; ch < numChannels; ch++) {
        channels[(size_t) ch] = buffer.getReadPointer(ch);
    }

    int streak = 0;
    for (int i = 0; i < numSamples; i++) {
        bool above = false;
        for (int ch = 0; ch < numChannels; ch++) {
            if (std::abs(channels[(size_t) ch][i]) >= threshold) {
                above = true;
                break;
            }
        }

        if (above) {
            if (++streak >= hold) {
                return i - hold + 1;
            }
        } else {
            streak = 0;
        }
    }

    return 0;
}

double StartDetector::detectStartMs(const std::map<int, SampleData>& samples, double attackMs) const
{
    if (samples.empty()) {
        return 0.0;
    }

    std::vector<double> starts;
    starts.reserve(samples.size());

    for (const auto& entry : samples) {
        const SampleData& data = entry.second;
        const int onset = findOnsetSample(data);
        const double onsetMs = data.sampleRate > 0.0 ? onset * 1000.0 / data.sampleRate : 0.0;
        starts.push_back(std::max(0.0, onsetMs - attackMs));
    }

    std::sort(starts.begin(), starts.end());
    const size_t n = starts.size();
    const double median = (n % 2 == 1)
                              ? starts[n / 2]
                              : 0.5 * (starts[n / 2 - 1] + starts[n / 2]);
    return std::round(median);
}
