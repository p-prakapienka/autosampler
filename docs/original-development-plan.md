# JUCE Autosampler App — Original Development Plan

> Historical document. v1 was built from this plan and is working. The code is the source of truth. Some snippets use plugin-style naming (PluginProcessor, processBlock, prepareToPlay) from an early draft; the app is a standalone desktop app.

## Project Overview
A JUCE-based desktop application that automatically samples instruments across a range of MIDI notes, with support for multiple export formats (SF2, SFZ, Decent Sampler). Useful for generating sample libraries from any MIDI instrument connected to the system.

---

## Phase 1: Project Setup & Architecture

### 1.1 JUCE Project Configuration
- Type: Desktop Application (standalone)
- Target platforms: macOS, Windows, Linux
- JUCE version: latest stable
- Build: CMake (recommended over Projucer)
- Audio/MIDI: system audio input via JUCE AudioDeviceManager

### 1.2 Core Data Structures
```
SampleData (per-note storage)
├── midiNote (uint8: 0-127)
├── audioBuffer (AudioBuffer<float>)
├── sampleRate (double)
├── numChannels (int)
└── startTime (Time)

AutosamplerState
├── startNote (0-127)
├── endNote (0-127)
├── noteDuration (double: seconds)
├── midiChannel (1-16)
├── velocity (0-127, default 100)
├── samplePackName (String)
├── exportFormat (enum: SF2, SFZ, DecentSampler)
├── samples (map: note -> SampleData)
└── isRecording (bool)
```

### 1.3 Project Structure
```
juce-autosampler/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.h/cpp
│   ├── PluginEditor.h/cpp
│   ├── Components/
│   │   ├── ControlPanel.h/cpp
│   │   └── RecordingIndicator.h/cpp
│   ├── Sampling/
│   │   ├── SamplerEngine.h/cpp
│   │   ├── SampleBuffer.h/cpp
│   │   └── RecordingManager.h/cpp
│   ├── Export/
│   │   ├── ExportFormat.h
│   │   ├── SF2Exporter.h/cpp
│   │   ├── SFZExporter.h/cpp
│   │   └── DecentSamplerExporter.h/cpp
│   └── Utilities/
│       └── MidiHelper.h/cpp
└── Resources/
```

---

## Phase 2: Core Functionality

### 2.1 MIDI Sequencing (SamplerEngine)
For each note from startNote to endNote:
1. Send Note On (selected velocity, selected channel)
2. Record audio for noteDuration seconds
3. Send Note Off
4. Store the recording for that note
5. Move to the next note

```cpp
void SamplerEngine::startSampling() {
  currentNoteIndex = startNote;
  recordingBuffer.clear();
  sendMidiNoteOn(currentNoteIndex, midiChannel, velocity);
  isRecording = true;
}

// called per audio block
void SamplerEngine::process(const AudioBuffer<float>& input) {
  recordAudioBlock(input);
  if (recordingTime >= noteDuration) {
    storeSample(currentNoteIndex, recordingBuffer);
    sendMidiNoteOff(currentNoteIndex, midiChannel);
    if (currentNoteIndex < endNote) {
      currentNoteIndex++;
      recordingBuffer.clear();
      sendMidiNoteOn(currentNoteIndex, midiChannel, velocity);
    } else {
      isRecording = false;
      triggerCallback(SamplingComplete);
    }
  }
}
```

### 2.2 Audio Input Recording (RecordingManager)
- Store samples in AudioBuffer<float> per MIDI note
- Handle sample rates 44.1–192 kHz
- Memory estimate: notes × duration × sampleRate × channels × 4 bytes
  (e.g. 128 × 3 s × 44.1 kHz × 2 ch × 4 B ≈ 135 MB)
- Warn in UI for high-memory configurations

```cpp
void RecordingManager::recordBlock(const AudioBuffer<float>& in) {
  for (int ch = 0; ch < in.getNumChannels(); ++ch)
    currentSampleBuffer.copyFrom(ch, writePosition, in, ch, 0, in.getNumSamples());
  writePosition += in.getNumSamples();
}
```

### 2.3 MIDI Channel
- Dropdown 1–16, passed to MidiMessage::noteOn(channel, note, velocity)

---

## Phase 3: User Interface

### 3.1 Layout
```
┌─────────────────────────────────┐
│  JUCE AUTOSAMPLER               │
├─────────────────────────────────┤
│ Sample Pack Name: [MyInstrument]│
│ Start Note:   [0  ]  C-1        │
│ End Note:     [127]  G9         │
│ Duration:     [3.0] seconds     │
│ Velocity:     [100] (0-127)     │
│ MIDI Channel: [1  ▼]            │
├─────────────────────────────────┤
│ [RUN SAMPLING]                  │
│ Status: Idle / Recording...     │
├─────────────────────────────────┤
│ Export Format: [SFZ ▼]          │
│ [EXPORT SAMPLES]                │
└─────────────────────────────────┘
```

### 3.2 Components
```cpp
// Sample pack name
TextEditor samplePackNameEditor;
samplePackNameEditor.setText("MySamples", dontSendNotification);
samplePackNameEditor.setMultiLine(false);

// Start/end note
Slider startNoteSlider, endNoteSlider;
startNoteSlider.setRange(0, 127, 1);

// Duration
Slider durationSlider;
durationSlider.setRange(0.1, 10.0, 0.1);

// Velocity
Slider velocitySlider;
velocitySlider.setRange(0, 127, 1);
velocitySlider.setValue(100);

// MIDI channel
ComboBox midiChannelCombo;
for (int i = 1; i <= 16; ++i) midiChannelCombo.addItem("Ch " + String(i), i);
midiChannelCombo.setSelectedId(1);

// Run
TextButton runButton("RUN SAMPLING");
runButton.onClick = [this] { startSampling(); };

// Export format
ComboBox exportFormatCombo;
exportFormatCombo.addItem("SF2", (int)ExportFormat::SF2);
exportFormatCombo.addItem("SFZ", (int)ExportFormat::SFZ);
exportFormatCombo.addItem("Decent Sampler", (int)ExportFormat::DecentSampler);

// Export
TextButton exportButton("EXPORT SAMPLES");
exportButton.onClick = [this] { exportSamples(); };
exportButton.setEnabled(hasSamples());
```

### 3.3 Status Display
- "Idle" / "Recording Note C4 (2.5s)" / "Complete"
- Optional progress bar for note count

---

## Phase 4: Export

### 4.1 Exporter Interface
```cpp
class AudioExporter {
public:
  virtual ~AudioExporter() = default;
  virtual bool exportSamples(const SampleMap& samples,
                             const String& samplePackName,
                             const File& outputDirectory) = 0;
};
```

### 4.2 Naming
- Library/instrument file named after the sample pack: `{packName}.sfz` etc.
- WAV files: `{packName}_{NoteName}.wav`, e.g. `MySamples_C3.wav` for MIDI 48

```cpp
String getMidiNoteName(uint8_t noteNumber) {
  static const char* noteNames[] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
  };
  int octave = (noteNumber / 12) - 1;
  int semitone = noteNumber % 12;
  return String(noteNames[semitone]) + String(octave);
}

String getWavFilename(const String& packName, uint8_t noteNumber) {
  return packName + "_" + getMidiNoteName(noteNumber) + ".wav";
}
```

### 4.3 SFZ Exporter
```cpp
String generateSFZContent(const SampleMap& samples, const String& packName) {
  String content = "<global>\nlovel=0\nhivel=127\n\n";
  for (auto& [note, data] : samples) {
    content += "<region>\n";
    content += "sample=" + getWavFilename(packName, note) + "\n";
    content += "key=" + String(note) + "\n";
    content += "pitch_keycenter=" + String(note) + "\n\n";
  }
  return content;
}
```

### 4.4 SF2 Exporter
- Binary RIFF: INFO (name, copyright), sdta (sample data), pdta (presets/instruments)
- Most complex format; manual writer or third-party library

### 4.5 Decent Sampler Exporter
- XML-based; one sample element per note with path, note and name attributes
- Output file named after the sample pack

### 4.6 WAV Writing
```cpp
void writeWAVFile(const File& file, const SampleData& data) {
  WavAudioFormat format;
  std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(
      new FileOutputStream(file), data.sampleRate, data.numChannels,
      16, {}, 0));
  writer->writeFromAudioSampleBuffer(data.audioBuffer, 0,
                                     data.audioBuffer.getNumSamples());
}
```

---

## Phase 5: Implementation Priorities (as originally planned)
1. Core: CMake app setup, AudioDeviceManager, UI scaffold, MIDI output, recording, per-note storage
2. UI refinement: all controls, status, note names, validation, memory warning
3. Export: SFZ, WAV writing, export directory dialog, progress
4. Polish: SF2, Decent Sampler, settings persistence, error handling

---

## Phase 6: Technical Considerations
- MIDI output: system MIDI output device via MidiOutput; open once, send from non-audio thread; account for MIDI latency
- Audio input: AudioDeviceManager + AudioIODeviceCallback
- Memory: pre-allocate per-note buffers; clear previous samples on new run
- Sample rate: read from device; store with each recording
- Thread safety: lock-free communication (AbstractFifo / atomics) between audio and UI threads
- Export validation: at least one sample, disk space, overwrite prompt, valid pack name

---

## Phase 7: Testing Checklist
- MIDI notes sent in order with selected channel and velocity
- Each note recorded for the exact duration, mapped to the right note
- Full range 0–127 works
- Exported WAV names follow `{packName}_{NoteName}.wav`
- SFZ / SF2 / Decent Sampler instruments load in compatible players
- Edge cases: start = end, very short/long durations, 192 kHz, stop mid-run, special characters in pack name

---

## Dependencies
- JUCE modules: juce_core, juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_gui_basics
- Optional: libsndfile, an SF2 writer library

## Success Criteria
1. App launches without crashes
2. Notes sent sequentially via system MIDI output with chosen channel and velocity
3. Each note recorded cleanly and independently
4. Pack name applied to exported files and library
5. Exports with correct naming in at least SFZ
6. Exported instruments playable in any DAW/sampler
7. Clear UI with status feedback
8. Full MIDI range handled without memory issues
