#pragma once

#include "../AutosamplerState.h"
#include <map>

// Finds a shared start time from the recorded notes. Does not modify the buffers.
class StartDetector
{
public:
    // Median onset, pulled forward by attackMs so the fade ends as the transient begins.
    double detectStartMs(const std::map<int, SampleData>& samples, double attackMs) const;

private:
    static int findOnsetSample(const SampleData& data);
};
