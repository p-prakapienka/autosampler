#include "SamplerEngine.h"

SamplerEngine::SamplerEngine(MidiOutputManager& midiOut, RecordingManager& recorder)
    : midiOutputManager(midiOut), recordingManager(recorder)
{
}

SamplerEngine::~SamplerEngine()
{
    stopSampling();
}

void SamplerEngine::startSampling(int start, int end, double duration,
                                   int channel, int vel)
{
    if (sampling)
        stopSampling();

    startNote = start;
    endNote = end;
    noteDuration = duration;
    midiChannel = channel;
    velocity = vel;
    currentNote = startNote;
    currentPhase = NotePhase::SendingNoteOn;
    recordingElapsed = 0.0;
    collectedSamples.clear();
    sampling = true;

    startTimer(timerIntervalMs);
}

void SamplerEngine::stopSampling()
{
    if (!sampling)
        return;

    stopTimer();
    recordingManager.stopRecording();
    midiOutputManager.sendAllNotesOff(midiChannel);
    sampling = false;

    if (statusCallback)
        statusCallback("Stopped", currentNote, getTotalNotes());
}

void SamplerEngine::timerCallback()
{
    if (!sampling)
        return;

    switch (currentPhase)
    {
        case NotePhase::SendingNoteOn:
        {
            recordingManager.prepareToRecord(
                recordingManager.getCurrentSampleRate(),
                recordingManager.getCurrentNumChannels(),
                noteDuration + 0.5);
            recordingManager.startRecording();
            midiOutputManager.sendNoteOn(midiChannel, currentNote, velocity);
            recordingElapsed = 0.0;
            currentPhase = NotePhase::Recording;

            if (statusCallback)
            {
                auto noteName = getMidiNoteDisplayName(currentNote);
                auto status = "Recording " + noteName + " (" +
                              juce::String(currentNote) + ")";
                statusCallback(status, getCompletedNotes(), getTotalNotes());
            }
            break;
        }

        case NotePhase::Recording:
        {
            recordingElapsed += timerIntervalMs / 1000.0;
            if (recordingElapsed >= noteDuration)
            {
                currentPhase = NotePhase::SendingNoteOff;
            }
            break;
        }

        case NotePhase::SendingNoteOff:
        {
            midiOutputManager.sendNoteOff(midiChannel, currentNote);
            recordingManager.stopRecording();
            currentPhase = NotePhase::Storing;
            break;
        }

        case NotePhase::Storing:
        {
            storeCurrentSample();
            advanceToNextNote();
            break;
        }
    }
}

void SamplerEngine::storeCurrentSample()
{
    SampleData data;
    data.midiNote = static_cast<juce::uint8>(currentNote);
    data.audioBuffer = recordingManager.getRecordedBuffer();
    data.sampleRate = recordingManager.getCurrentSampleRate();
    data.numChannels = recordingManager.getCurrentNumChannels();
    collectedSamples[currentNote] = std::move(data);
}

void SamplerEngine::advanceToNextNote()
{
    if (currentNote < endNote)
    {
        currentNote++;
        currentPhase = NotePhase::SendingNoteOn;
    }
    else
    {
        finishSampling();
    }
}

void SamplerEngine::finishSampling()
{
    stopTimer();
    sampling = false;

    if (statusCallback)
        statusCallback("Complete! " + juce::String(collectedSamples.size()) +
                       " samples recorded.", endNote - startNote + 1, getTotalNotes());

    if (completionCallback)
        completionCallback(collectedSamples);
}
