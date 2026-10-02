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
   - Convert each onset to milliseconds. Automatic start = median of `max(0, onsetMs - defaultAttackMs)`.
   - Runs on the message thread when sampling completes. Not on the audio thread.

5. **Attack**
   - A single Attack slider, in milliseconds, shared by every note.
   - Range: 0 to 50 ms. Step: 1 ms. Default: 5 ms.
   - From the start point, gain ramps linearly from 0 to 1 over the attack length.
   - 0 ms means no ramp (a click is possible if the start sample is not near zero; that is the user's choice).
   - If the remaining audio is shorter than the attack, the ramp is shortened to fit. It always reaches 1 at the end of the ramp.

6. **Preview**
   - Playback starts at the start point, not at sample 0 of the raw buffer.
   - The same linear ramp is applied.
   - Moving a slider does not restart a note that is already playing. The new values apply on the next note-on.
   - No allocation and no locking on the audio thread for these parameters. Publish start and attack sample counts with atomics; the voice reads them when a note starts.

7. **Export**
   - On export, build a processed copy of each `SampleData`: drop samples before the start point, apply the attack ramp to the head, leave the rest unchanged.
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

### New: onset detection

A free function or small helper, message thread only:

```cpp
// Returns the automatic start in milliseconds for this set of takes.
double detectStartMs(const std::map<int, SampleData>& samples, double attackMs);
```

Called from the sampling-complete path in `MainComponent`, after `capturedSamples` is filled and before preview is enabled.

### New: render helper

Message thread only. Used by export, and by any non-realtime check. Preview does not use this copy; it reads the raw buffer at an offset.

```cpp
SampleData applyStartAndAttack(const SampleData& source, double startMs, double attackMs);
```

One shared gain function so preview and export match:

```cpp
// posFromStart is 0 at the first exported/played sample.
inline float attackGain(int posFromStart, int attackSamples)
{
    if (attackSamples <= 0 || posFromStart >= attackSamples)
        return 1.0f;
    return (float) (posFromStart + 1) / (float) attackSamples;
}
```

### Preview

`SamplePlaybackBuffer::startPlayback` takes a start sample index and an attack length in samples. `readPosition` begins at the start index. Each output sample is multiplied by `attackGain` until the ramp ends, then by the existing release fade.

`SamplePreviewManager` holds `std::atomic<int>` start and attack counts, in the recorded sample rate. `MainComponent` writes them when a slider moves or when the device sample rate is known. `noteOn` copies them into the voice. Do not allocate a processed buffer per note-on.

This plan does not redo plan 001's thread model. It only requires that these two new parameters are safe to read on the audio thread.

### Export call site

In `MainComponent::exportSamples`, build a temporary `std::map<int, SampleData>` by running `applyStartAndAttack` on each captured note, and pass that map to `SFZExporter`, `SF2Exporter`, and `DecentSamplerExporter`. Those classes stay unaware of the sliders.

### State to keep on the UI side

- `detectedStartMs` — result of the last detection. 0 if there are no samples.
- `startMs` — slider value, initialized from `detectedStartMs`.
- `attackMs` — slider value, default 5.

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
- [ ] Moving a slider during a held note does not glitch that note; the next press uses the new value.
- [ ] Exported SFZ, SF2, and Decent Sampler start at the same point and with the same attack as preview.
- [ ] Export does not change what a later preview plays if the sliders are untouched (raw buffer still has the leading silence; playback offset still skips it).
- [ ] Stereo WAVs stay stereo. SF2 remains mono from channel 0, as today.

### Edges
- [ ] Start at 0 and Attack at 0 reproduces today's export.
- [ ] Start near the end of the buffer does not read past the buffer and does not crash.
- [ ] Attack longer than the remaining audio ramps across what is left and does not exceed 1.

---

## Constraints and out of scope

- No per-note start or attack.
- No end trim, tail fade, or release envelope. A click at the end of a file is a separate plan.
- No user-facing threshold control. -40 dB and 1 ms hold are fixed.
- No persistence of the sliders across launches.
- No resampling. Times convert at the recorded sample rate, same rule as preview today.
- No pitch, loop, or preview-volume work (still listed as future ideas on plan 001).

## Implementation phases

1. Detection helper and the three pieces of state, set when sampling completes.
2. Preview start offset and attack ramp.
3. Start slider, Attack slider, Auto button.
4. Export copy wired into the existing three exporters.

## Deviations

(fill in after implementation)
