#pragma once

#include <juce_core/juce_core.h>
#include "../AutosamplerState.h"
#include "WavWriter.h"
#include <map>

/**
 * Decent Sampler Exporter
 *
 * Exports a .dspreset XML file alongside WAV samples.
 * The .dspreset format is used by the free Decent Sampler plugin.
 *
 * Structure:
 *   <DecentSampler>
 *     <groups>
 *       <group>
 *         <sample path="..." rootNote="..." loNote="..." hiNote="..." ... />
 *         ...
 *       </group>
 *     </groups>
 *   </DecentSampler>
 *
 * Reference: https://www.decentsamples.com/docs/format-documentation/
 */
class DecentSamplerExporter
{
public:
    static bool exportSamples(const std::map<int, SampleData>& samples,
                              const juce::String& packName,
                              const juce::File& outputDirectory)
    {
        if (samples.empty()) {
            return false;
        }

        if (!outputDirectory.exists()) {
            outputDirectory.createDirectory();
        }

        // Create a Samples subdirectory for WAV files
        auto samplesDir = outputDirectory.getChildFile("Samples");
        if (!samplesDir.exists()) {
            samplesDir.createDirectory();
        }

        // Write WAV files
        for (auto& [midiNote, data] : samples) {
            auto wavFilename = WavWriter::getWavFilename(packName, midiNote);
            auto wavFile = samplesDir.getChildFile(wavFilename);

            if (!WavWriter::writeWavFile(wavFile, data)) {
                return false;
            }
        }

        // Build XML
        auto xmlContent = generateXML(samples, packName);

        // Write .dspreset file
        auto presetFile = outputDirectory.getChildFile(packName + ".dspreset");
        return presetFile.replaceWithText(xmlContent);
    }

private:
    static juce::String generateXML(const std::map<int, SampleData>& samples,
                                     const juce::String& packName)
    {
        // Collect notes into a vector for lokey/hikey calculation
        std::vector<int> midiNotes;
        for (auto& [note, data] : samples) {
            midiNotes.push_back(note);
        }

        int numSamples = (int) midiNotes.size();

        juce::XmlElement root("DecentSampler");
        root.setAttribute("pluginVersion", "1");

        // <ui> element with basic knobs
        auto* ui = root.createNewChildElement("ui");
        auto* tab = ui->createNewChildElement("tab");
        tab->setAttribute("name", "main");

        auto* volumeKnob = tab->createNewChildElement("labeled-knob");
        volumeKnob->setAttribute("x", 180);
        volumeKnob->setAttribute("y", 40);
        volumeKnob->setAttribute("width", 90);
        volumeKnob->setAttribute("label", "Volume");
        volumeKnob->setAttribute("type", "float");
        volumeKnob->setAttribute("minValue", "0");
        volumeKnob->setAttribute("maxValue", "2");
        volumeKnob->setAttribute("value", "1");

        auto* volumeBinding = volumeKnob->createNewChildElement("binding");
        volumeBinding->setAttribute("type", "amp");
        volumeBinding->setAttribute("level", "instrument");
        volumeBinding->setAttribute("position", 0);
        volumeBinding->setAttribute("parameter", "AMP_VOLUME");

        // <groups> / <group>
        auto* groups = root.createNewChildElement("groups");
        auto* group = groups->createNewChildElement("group");
        group->setAttribute("ampVelTrack", 1.0);
        group->setAttribute("name", packName);

        for (int i = 0; i < numSamples; i++) {
            int midiNote = midiNotes[(size_t) i];

            // Calculate key range
            int lokey, hikey;
            if (i == 0) {
                lokey = midiNote;
            } else {
                lokey = midiNotes[(size_t) i - 1] + (midiNote - midiNotes[(size_t) i - 1]) / 2 + 1;
            }

            if (i == numSamples - 1) {
                hikey = midiNote;
            } else {
                hikey = midiNote + (midiNotes[(size_t) i + 1] - midiNote) / 2;
            }

            auto wavFilename = WavWriter::getWavFilename(packName, midiNote);

            auto* sample = group->createNewChildElement("sample");
            sample->setAttribute("path", "Samples/" + wavFilename);
            sample->setAttribute("rootNote", midiNote);
            sample->setAttribute("loNote", lokey);
            sample->setAttribute("hiNote", hikey);
            sample->setAttribute("loVel", 0);
            sample->setAttribute("hiVel", 127);
        }

        return root.toString();
    }
};
