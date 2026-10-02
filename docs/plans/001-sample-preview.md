# Implementation plan 001: Sample Preview with Virtual MIDI Keyboard

## Overview
Let the user audition recorded samples with a 49-key JUCE virtual MIDI keyboard at the bottom of the window. Pressing a key looks up the sample recorded for that MIDI note and plays it through the app's audio output.

---

## Requirements

1. **Keyboard display**
   - A JUCE `MidiKeyboardComponent` (driven by a `MidiKeyboardState`) sits at the bottom of the main window, full width.
   - Range: 49 keys, C2–C6 (MIDI 36–84), using the app's octave convention (C4 = MIDI 60). Set with `setAvailableRange(36, 84)`.
   - A small label shows the range: "C2 – C6".

2. **Enabled state**
   - Disabled (greyed, non-interactive) when no samples exist.
   - Enabled when at least one sample exists.
   - Disabled while a sampling run is in progress (preview output could bleed into the recording).

3. **Note On**
   - Pressing a key looks up the sample for that MIDI note number.
   - If found: playback starts from the beginning of the sample.
   - If not found: nothing happens (no sound, no error).
   - Pressing a key that is already playing restarts it from the beginning (no layering of the same note).

4. **Note Off**
   - Releasing a key fades the sample out over 50 ms (no clicks).
   - If the sample ends before release, playback simply stops at the end of the buffer.

5. **Polyphony**
   - Different keys can play simultaneously, up to 8 voices.
   - When all 8 are busy, the oldest voice is stolen.

6. **Output**
   - Preview audio is mixed into the device output buffer, which is cleared first each callback.
   - The audio input is not passed through to the output.
   - Mono samples are sent to both output channels; stereo samples map L/R.

---

## UI Changes

```
┌─────────────────────────────────────┐
│  [Existing controls]                │
│  Sample pack, start/end note, etc.  │
├─────────────────────────────────────┤
│  [RUN] [EXPORT]   Status display    │
├─────────────────────────────────────┤
│  Preview keyboard          C2 – C6  │
│  ┌───────────────────────────────┐  │
│  │ 49-key MidiKeyboardComponent  │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```
- Keyboard height: ~100–120 px. No scrolling, no octave shift controls.
- Minimum window height increases to accommodate the keyboard.

---

## Implementation Details

> Integration point names below are generic. Map them to the actual classes in the codebase (the app is a desktop app using an `AudioIODeviceCallback` via `AudioDeviceManager`, not a plugin).

### Event path (important for thread safety)
Do not trigger playback from UI-thread listener callbacks. Instead:
1. The UI owns no playback logic; the `MidiKeyboardComponent` only updates a shared `MidiKeyboardState`.
2. In the audio callback, call `keyboardState.processNextMidiBuffer(midiBuffer, 0, numSamples, true)` to collect note events for that block.
3. Iterate `midiBuffer` on the audio thread and start/stop voices in `SamplePreviewManager`.

This keeps all voice state on the audio thread.

### New class: `PreviewVoice`
```cpp
class PreviewVoice {
public:
  void start(const juce::AudioBuffer<float>* source, int midiNote);
  void release(int fadeSamples);      // begin 50 ms fade-out
  void render(juce::AudioBuffer<float>& out, int startSample, int numSamples);
  bool isActive() const;
  int  getNote() const;
private:
  const juce::AudioBuffer<float>* source = nullptr;
  int note = -1, readPos = 0;
  int fadeRemaining = -1, fadeLength = 0; // -1 = not fading
  uint64_t startOrder = 0;                // for oldest-voice stealing
};
```

### New class: `SamplePreviewManager`
```cpp
class SamplePreviewManager {
public:
  void prepare(double deviceSampleRate);       // fadeSamples = 0.05 * rate
  void setSamples(std::shared_ptr<const SampleSet> samples); // message thread
  void handleMidi(const juce::MidiBuffer& midi); // audio thread
  void render(juce::AudioBuffer<float>& out, int numSamples); // audio thread
  void allNotesOff();
private:
  std::array<PreviewVoice, 8> voices;
  // current sample set, swapped safely (see Thread Safety)
};
```
`SampleSet` = whatever the existing per-note storage is (MIDI note → `SampleData` with its `AudioBuffer<float>`).

### Modifications to existing code
- **Audio callback** (the existing `AudioIODeviceCallback`):
  1. Record input as today (unchanged).
  2. Clear the output buffer.
  3. `keyboardState.processNextMidiBuffer(...)` → `previewManager.handleMidi(...)`.
  4. `previewManager.render(output, numSamples)`.
- **Device start**: call `previewManager.prepare(sampleRate)`.
- **Sampling complete**: on the message thread, hand the new sample set to `previewManager.setSamples(...)` and enable the keyboard.
- **Sampling start**: disable the keyboard, call `keyboardState.allNotesOff(0)` and stop all voices.
- **Main UI component**: add `MidiKeyboardComponent` + range label; lay out at the bottom in `resized()`.

### Thread safety
- Sample buffers are immutable once a sampling run completes.
- The sample set the audio thread reads must never be modified or freed while being read. Publish it as an immutable snapshot (e.g. `std::shared_ptr<const SampleSet>` swapped under a short `juce::SpinLock`, or an atomic pointer with deferred release on the message thread). No allocation, no blocking locks, and no freeing of sample memory on the audio thread.
- Since sampling and preview are mutually exclusive (Requirement 2), stopping all voices before swapping the set is sufficient to avoid stale-voice reads.

### Data flow
```
Key press (UI) → MidiKeyboardState
  → audio callback: processNextMidiBuffer → handleMidi
  → look up sample for note → start/steal voice
  → render → device output
Key release → Note Off → 50 ms fade → voice idle
```

---

## Testing Criteria

### Functional
- [ ] Keyboard shows 49 keys, C2–C6 (MIDI 36–84), label reads "C2 – C6"
- [ ] Disabled with no samples; enabled after a completed run; disabled during a run
- [ ] Pressing a key with a sample plays it from the start
- [ ] Pressing a key without a sample: no sound, no crash
- [ ] Re-pressing a playing key restarts it (no layering)
- [ ] Several different keys play together; 9th key steals the oldest voice
- [ ] Release fades out with no audible click
- [ ] Sample shorter than hold time stops cleanly at its end
- [ ] Mono samples are heard on both output channels

### Edge cases
- [ ] Start a new sampling run while notes are held: preview stops, keyboard disables
- [ ] Rapid repeated key presses: no stutter, no crash
- [ ] Export while previewing: no audio artefacts, export correct
- [ ] Notes recorded outside 36–84 are simply not reachable from the keyboard (no errors)

### Performance
- [ ] Key-press-to-sound latency is bounded by the device buffer size (no extra delay)
- [ ] 8 simultaneous voices: no dropouts at a typical buffer size (256–512)

---

## Constraints & Out of Scope (v1)
- No pitch-shifting, resampling or time-stretching. Samples were recorded on the same device, so playback uses the device sample rate as-is.
- No preview volume control, looping, octave shift, computer-keyboard mapping, or MIDI-input preview.
- Fade-out fixed at 50 ms.
- Max 8 voices.

## Implementation phases
1. Core playback: `PreviewVoice`, `SamplePreviewManager`, audio-callback integration (test with a hardcoded note).
2. UI: keyboard component, range label, layout, enabled/disabled states.
3. Polish: fade-out, voice stealing, sampling-start interlock, thread-safe sample-set swap.

## Future ideas (separate plans)
- Octave shift buttons, preview volume, computer-keyboard mapping, loop playback, MIDI-input preview.

## Deviations
(fill in after implementation)
