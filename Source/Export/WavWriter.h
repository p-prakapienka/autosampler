#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "../AutosamplerState.h"

class WavWriter
{
public:
    static bool writeWavFile(const juce::File& outputFile, const SampleData& data, int bitDepth = 16)
    {
        juce::WavAudioFormat format;
        auto outputStream = outputFile.createOutputStream();
        if (outputStream == nullptr) {
            return false;
        }

        std::unique_ptr<juce::AudioFormatWriter> writer(
            format.createWriterFor(outputStream.release(),
                                   data.sampleRate,
                                   static_cast<unsigned int>(data.audioBuffer.getNumChannels()),
                                   bitDepth,
                                   {},
                                   0));

        if (writer == nullptr) {
            return false;
        }

        return writer->writeFromAudioSampleBuffer(data.audioBuffer, 0,
                                                   data.audioBuffer.getNumSamples());
    }

    static juce::String getWavFilename(const juce::String& packName, int midiNote)
    {
        return packName + "_" + getMidiNoteName(midiNote) + ".wav";
    }
};
