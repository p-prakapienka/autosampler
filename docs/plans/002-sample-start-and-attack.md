# Implementation plan 002: Adjustable sample start and attack

## Overview
Leading silence in a take is mostly MIDI and audio latency, not part of the instrument. Do not hard-trim it. Detect a start time, let the user move it, and apply a short attack fade so the new start does not click. Both controls are reversible until the next sampling run. Raw recordings stay intact in memory.

This replaces a destructive "trim silence" step.

---

## Requirements

1. **Raw audio is kept**
   - `SampleData::audioBuffer` remains the full recording from `RecordingManager`.
   - Start and attack are edit parameters. They are applied when previewing and when exporting, never by rewriting the stored take.
   - A new sampling run replaces the takes and re-runs detection. The previous slider values are not reused.

2. **One start time for the whole run**
   - A single Start slider, in milliseconds, shared by every note.
   - Latency is common to the run, so per-note start editors are out of scope.
   - Range: 0 to the recorded note length. Step: 1 ms.
   - Applied per note as `startSample = min(ms * sampleRate / 1000, numSamples - 1)`.

3. **Automatic start**
   - After a sampling run finishes, detect an onset per note, then set Start to the median.
   - Place that automatic start `attack` milliseconds *before* the onset (clamped at 0), so the default fade ends as the transient begins instead of ducking it.
   - An **Auto** button puts the slider back on the detected value.
   - The control shows both numbers when they differ, e.g. `Start 50 ms (auto 38 ms)`.

4. **Detection**
   - Per note, peak = max absolute sample across channels.
   - If peak < 1.0e-4, onset is sample 0 (nothing useful in the take).
   - Otherwise threshold = peak * 0.01 (-40 dB relative to that note's peak).
   - Onset = first sample where any channel stays at or above the threshold for 1 ms. This ignores a single spike.
   - Convert each onset to milliseconds. Automatic start = median of `max(0, onsetMs - attackMs)`.
   - Runs on the message thread when sampling completes. Not on the audio thread.

5. **Attack**
   - A single Attack slider, in milliseconds, shared by every note.
   - Range: 0 to 50 ms. Step: 1 ms. Default: 5 ms.
   - Use `juce::ADSR`. Attack is the slider. Decay is 0. Sustain is 1.
   - 0 ms skips the attack. JUCE jumps straight to sustain.
   - If the remaining audio is shorter than the attack, playback ends while the envelope is still rising. The attack is not compressed to fit, and the gain does not exceed 1.

6. **Preview**
   - Playback starts at the start point, not at sample 0 of the raw buffer.
   - The same `juce::ADSR` attack is applied. Note-off is the ADSR release, 50 ms, which replaces the old hand-rolled fade.
   - Moving a slider does not restart a note that is already playing. The new values apply on the next note-on.
   - The playback voice stores the sample counts. The preview manager does not.

7. **Export**
   - On export, build a processed copy of each `SampleData`: drop samples before the start point, run `juce::ADSR` over the copy with the same attack and with release 0, so the file tail is not faded.
   - Pass that copy to the existing SFZ, SF2, and Decent Sampler exporters. Do not modify `capturedSamples`.
   - Do not rely on format-specific offset or envelope opcodes (`offset` / `ampeg_attack`, Decent Sampler `start` / `attack`, SF2 `startAddrsOffset` / `attackVolEnv`). Baked audio is the only way preview, SFZ, SF2, and Decent Sampler stay the same. SF2 already takes channel 0 of the buffer; the copy keeps the original channel count so SFZ and Decent Sampler stay stereo.
   - Exporting again after moving a slider rewrites the files. No re-record.

---

## UI

Enabled only when at least one sample exists (same idea as the preview keyboard).

```
Start   [========●------]  38 ms (auto 38 ms)   [Auto]
Attack  [==●----------]   5 ms
```

- Start and Attack sit with the existing sampling controls, above the preview keyboard.
- **Auto** sets Start back to the detected value. It does not change Attack.
- Changing either slider updates the next preview note and the next export immediately.

---

## Implementation details

Map names onto the current desktop app (`SamplerEngine`, `SampleData`, `SamplePlaybackBuffer`, `MainComponent`). This is not a plugin.

### `SamplePlaybackBuffer`

The voice plays one recorded note. It stores a start sample and an attack length, and applies `juce::ADSR` (decay 0, sustain 1). There is no separate edit object.

`startPlayback` reads those values, calls `noteOn` on the envelope, and `readBlock` multiplies by `getNextSample`. `stopPlayback` calls `noteOff`. Preview release is 50 ms.

`render` is the same voice used offline for export: trim to the start sample, then `setParameters` with release 0, `noteOn`, and `applyEnvelopeToBuffer`. It does not modify the source buffer.

`msToSamples` converts the slider times. The setters store sample counts only.

### `StartDetector`

```cpp
class StartDetector {
public:
    double detectStartMs(const std::map<int, SampleData>& samples, double attackMs) const;
};
```

`MainComponent` owns the two slider values in milliseconds and one `StartDetector`. Detection runs with the current attack. Called from the sampling-complete path, after `capturedSamples` is filled and before preview is enabled.

### Preview

`SamplePreviewManager` does not store start or attack. On note-on it converts the millisecond arguments with `msToSamples`, calls the two setters on the voice, then `startPlayback`. Do not allocate a processed buffer per note-on.

This plan does not redo plan 001's thread model. Do not change ADSR parameters during a note. `reset` before the next `noteOn`.

### Export call site

In `MainComponent::exportSamples`, build a temporary `std::map<int, SampleData>` by setting start and attack on a `SamplePlaybackBuffer` and calling `render` for each captured note. Pass that map to `SFZExporter`, `SF2Exporter`, and `DecentSamplerExporter`. Those classes stay unaware of the sliders.

### State to keep on the UI side

- `startMs` and `attackMs` — the slider values.
- `detectedStartMs` — result of the last detection, for the Auto button and the "(auto N ms)" label. 0 if there are no samples.

No new fields on `SampleData`. No change to note naming, pack naming, or the recording path.

---

## Testing criteria

### Detection and sliders
- [ ] A take with ~30 ms of leading silence gets an automatic start near that silence, a few ms before the transient (not on top of it).
- [ ] A take that is already loud at sample 0 gets automatic start 0.
- [ ] A near-silent take does not crash and gets start 0.
- [ ] Median: one note with a much later onset does not drag every note to that late start.
- [ ] Moving Start and pressing Auto restores the detected value. Attack is unchanged.
- [ ] A new sampling run replaces the detected value and resets Start to it.

### Sound
- [ ] Preview of a key starts at the slider position, not at the beginning of the raw take.
- [ ] With Attack at 5 ms there is no click at the start. With Attack at 0, the file and the preview start immediately (a click is acceptable).
- [ ] Releasing a key fades out over about 50 ms.
- [ ] Moving a slider during a held note does not glitch that note; the next press uses the new value.
- [ ] Exported SFZ, SF2, and Decent Sampler start at the same point and with the same attack as preview. The exported file has no release fade.
- [ ] Export does not change what a later preview plays if the sliders are untouched (raw buffer still has the leading silence; playback offset still skips it).
- [ ] Stereo WAVs stay stereo. SF2 remains mono from channel 0, as today.

### Edges
- [ ] Start at 0 and Attack at 0 reproduces today's export.
- [ ] Start near the end of the buffer does not read past the buffer and does not crash.
- [ ] Attack longer than the remaining audio does not exceed 1 and does not read past the buffer.

---

## Constraints and out of scope

- No per-note start or attack.
- No user-facing decay, sustain, or release slider. Decay is 0 and sustain is 1. Preview release stays 50 ms. Export release is 0. A click at the end of a file is a separate plan.
- No user-facing threshold control. -40 dB and 1 ms hold are fixed.
- No persistence of the sliders across launches.
- No resampling. Times convert at the recorded sample rate, same rule as preview today.
- No pitch, loop, or preview-volume work (still listed as future ideas on plan 001).

## Implementation phases

1. `StartDetector` and `SamplePlaybackBuffer`: detection, playback, and the export copy.
2. Preview: the voice starts at `startSample` with `juce::ADSR`.
3. Start slider, Attack slider, Auto button.
4. Export copy wired into the existing three exporters.

## Deviations

- Onset detection is `StartDetector`. Playback and the export copy are the same voice, `SamplePlaybackBuffer`. There is no `SampleEdit`.
- Start and attack sample counts live on `SamplePlaybackBuffer` (`setStartSample` / `getStartSample`, `setAttackSamples` / `getAttackSamples`). They are not arguments of `startPlayback`, and `SamplePreviewManager` does not store them.
- Accessors are getters and setters. No coined names. They only read or write the stored value, and they are declared last. Milliseconds become samples in `msToSamples`, not in a getter.
- The attack is `juce::ADSR`, not a custom gain ramp. Preview note-off is that envelope's 50 ms release. Export uses release 0.
