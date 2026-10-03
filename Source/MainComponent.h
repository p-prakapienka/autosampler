#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AutosamplerState.h"
#include "Sampling/MidiOutputManager.h"
#include "Sampling/RecordingManager.h"
#include "Sampling/SamplerEngine.h"
#include "Sampling/SampleEdit.h"
#include "Export/SFZExporter.h"
#include "Export/SF2Exporter.h"
#include "Export/DecentSamplerExporter.h"
#include "Preview/SamplePreviewManager.h"

class MainComponent : public juce::Component,
                      juce::AudioIODeviceCallback,
                      juce::MidiKeyboardState::Listener
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
    void refreshStartAutoLabel();

    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;

    juce::AudioDeviceManager audioDeviceManager;
    MidiOutputManager midiOutputManager;
    RecordingManager recordingManager;
    SamplerEngine samplerEngine;
    SampleEdit sampleEdit;

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

    // Sample start / attack. Raw buffers stay intact; SampleEdit is applied on preview and export.
    juce::Label sampleStartLabel;
    juce::Slider sampleStartSlider;
    juce::Label sampleStartAutoLabel;
    juce::TextButton autoStartButton { "Auto" };
    juce::Label attackLabel;
    juce::Slider attackSlider;
    double detectedStartMs = 0.0;

    // Sample preview
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent midiKeyboard { keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard };
    SamplePreviewManager previewManager;

    // Stored samples
    std::map<int, SampleData> capturedSamples;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
