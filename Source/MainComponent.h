#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AutosamplerState.h"
#include "Sampling/MidiOutputManager.h"
#include "Sampling/RecordingManager.h"
#include "Sampling/SamplerEngine.h"
#include "Export/SFZExporter.h"
#include "Export/SF2Exporter.h"
#include "Export/DecentSamplerExporter.h"

class MainComponent : public juce::Component,
                      private juce::AudioIODeviceCallback
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    void refreshMidiDevices();
    void startSampling();
    void stopSampling();
    void showAudioSettings();
    void exportSamples();
    void updateControlsEnabled();

    juce::AudioDeviceManager audioDeviceManager;
    MidiOutputManager midiOutputManager;
    RecordingManager recordingManager;
    SamplerEngine samplerEngine;

    // UI Controls
    juce::Label titleLabel;

    juce::Label startNoteLabel;
    juce::Slider startNoteSlider;
    juce::Label startNoteNameLabel;

    juce::Label endNoteLabel;
    juce::Slider endNoteSlider;
    juce::Label endNoteNameLabel;

    juce::Label durationLabel;
    juce::Slider durationSlider;

    juce::Label velocityLabel;
    juce::Slider velocitySlider;

    juce::Label midiChannelLabel;
    juce::ComboBox midiChannelCombo;

    juce::Label midiOutputLabel;
    juce::ComboBox midiOutputCombo;
    juce::TextButton refreshMidiButton { "Refresh" };

    juce::Label samplePackLabel;
    juce::TextEditor samplePackNameEditor;

    juce::TextButton audioSettingsButton { "Audio Settings..." };
    juce::TextButton runButton { "RUN SAMPLING" };
    juce::TextButton stopButton { "STOP" };

    // Export controls
    juce::Label exportFormatLabel;
    juce::ComboBox exportFormatCombo;
    juce::TextButton exportButton { "EXPORT SAMPLES" };
    juce::Label exportStatusLabel;

    juce::Label statusLabel;
    double progress = 0.0;
    juce::ProgressBar progressBar { progress };

    // Stored samples
    std::map<int, SampleData> capturedSamples;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
