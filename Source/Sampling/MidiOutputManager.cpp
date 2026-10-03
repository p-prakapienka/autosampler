#include "MidiOutputManager.h"

juce::StringArray MidiOutputManager::getAvailableDevices() const
{
    juce::StringArray names;
    auto devices = juce::MidiOutput::getAvailableDevices();
    for (auto& device : devices) {
        names.add(device.name);
    }
    return names;
}

bool MidiOutputManager::openDevice(const juce::String& deviceIdentifier)
{
    closeDevice();

    auto devices = juce::MidiOutput::getAvailableDevices();
    for (auto& device : devices) {
        if (device.name == deviceIdentifier) {
            midiOutput = juce::MidiOutput::openDevice(device.identifier);
            if (midiOutput != nullptr) {
                currentDeviceName = device.name;
                return true;
            }
        }
    }
    return false;
}

void MidiOutputManager::closeDevice()
{
    if (midiOutput != nullptr) {
        midiOutput.reset();
        currentDeviceName.clear();
    }
}

void MidiOutputManager::sendNoteOn(int channel, int noteNumber, int velocity)
{
    if (midiOutput != nullptr) {
        auto msg = juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8) velocity);
        midiOutput->sendMessageNow(msg);
    }
}

void MidiOutputManager::sendNoteOff(int channel, int noteNumber)
{
    if (midiOutput != nullptr) {
        auto msg = juce::MidiMessage::noteOff(channel, noteNumber);
        midiOutput->sendMessageNow(msg);
    }
}

void MidiOutputManager::sendAllNotesOff(int channel)
{
    if (midiOutput != nullptr) {
        auto msg = juce::MidiMessage::allNotesOff(channel);
        midiOutput->sendMessageNow(msg);
    }
}
