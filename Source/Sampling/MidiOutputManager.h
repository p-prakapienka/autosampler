#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

class MidiOutputManager
{
public:
    MidiOutputManager() = default;
    ~MidiOutputManager() = default;

    juce::StringArray getAvailableDevices() const;
    bool openDevice(const juce::String& deviceIdentifier);
    void closeDevice();
    void sendNoteOn(int channel, int noteNumber, int velocity);
    void sendNoteOff(int channel, int noteNumber);
    void sendAllNotesOff(int channel);

    bool isDeviceOpen() const { return midiOutput != nullptr; }
    juce::String getCurrentDeviceName() const { return currentDeviceName; }

private:
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::String currentDeviceName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiOutputManager)
};
