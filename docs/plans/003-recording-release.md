# Implementation plan 003: Recording release time

## Overview

Note Off and the recorder stop on the same timer tick. The instrument is still decaying when the next note opens the recorder, so that tail is written into the next sample.

After the existing note length, send Note Off and keep recording for a Release time. The take is note length plus release. The next note starts only after that.

---

## Requirements

1. **Release control**
   - One slider, seconds, shared by every note in the run.
   - Sits under Duration. Label: `Release (s):`.
   - Range: 0.0 to 10.0. Step: 0.1. Default: 1.0.
   - Read once when the run starts, same as Duration. Moving it during a run does not change that run.

2. **Timeline per note**
   - Note On and start recording.
   - Hold until the note length. Note On to Note Off does not move.
   - Note Off. If release is 0, stop the recorder on that same tick (today's behavior).
   - Otherwise keep recording until the release time has elapsed, then stop.
   - Store the buffer and advance. The next Note On happens only after the recorder has stopped.

3. **What is stored**
   - The raw buffer is the whole take, decay included.
   - Start, attack, preview, and export are unchanged. Start still trims the front. Attack still fades the new start. Export release stays 0, so the captured tail is not faded again.
   - Preview key-release stays the existing 50 ms fade. Holding a preview key plays the recorded tail because it is in the buffer.

4. **Status**
   - While the release is recording, the status reads `Releasing C4 (60)` (note name and MIDI number), so a long tail does not look stuck on Recording.

5. **Stop**
   - Stop still sends all-notes-off and stops the recorder immediately, including during the release.

6. **Allocation**
   - The recorder is sized for note length + release + 0.5 s. The extra 0.5 s is the existing timer slack. It is not extra recorded audio.

Anything still sounding after the release time can still leak into the next note. There is no extra gap and no auto-stop on silence. Set Release at least as long as the source decay.

The engine stays a 50 ms message-thread timer. One phase per tick. Recorded release is the slider value, within one timer interval, same accuracy as note length.

---

## UI

```
Duration (s): [========●------]  3.0
Release (s):  [====●----------]  1.0
```

- Release stays enabled during a run, same as Duration and the other sampling controls.
- Window grows by one row.

---

## Implementation details

`SamplerEngine` gains a `Releasing` phase.

```text
SendingNoteOn → Recording → SendingNoteOff → Releasing → Storing → next note
                                      └─ release 0 → Storing
```

- `SendingNoteOn` allocates `noteDuration + releaseDuration + 0.5` and sends Note On, as today.
- `Recording` waits for the note length, then moves to `SendingNoteOff`. Note Off is still the next tick.
- `SendingNoteOff` sends Note Off. Release 0 stops the recorder and goes to `Storing`. Otherwise it zeros the release clock, reports Releasing, and goes to `Releasing`.
- `Releasing` adds one timer interval per tick. At or past the release time it stops the recorder and goes to `Storing`.
- `Storing` is unchanged.

`startSampling` takes `releaseDuration` after the note length. `AutosamplerState::releaseDuration` defaults to 1.0. `MainComponent` passes the slider value.

No change to `RecordingManager`, exporters, `StartDetector`, or preview.

---

## Testing criteria

- [ ] Release 0: Note Off and stop on the same tick. Take length matches a build without this control.
- [ ] Release 1 s, duration 3 s: Note Off around 3 s, recorder stops around 4 s, next Note On only after that.
- [ ] The stored buffer contains the decay. The next note does not, if the source has gone quiet.
- [ ] Status shows Releasing, then Recording for the next note.
- [ ] Stop during release cuts the note and the recorder immediately.
- [ ] Start detection, attack, preview, and all three exporters still use the raw buffer. The tail is kept. Export does not add its own fade.
- [ ] Changing Release during a run does not affect that run.

---

## Constraints and out of scope

- No per-note release.
- No silence detection and no pause after the tail.
- No preview or export release slider. Preview release stays 50 ms. Export release stays 0.
- No velocity layers.
- No persistence of the slider across launches.

## Deviations

- Duration and the other sampling controls stay enabled during a run. Release matches them. The value is copied in `startSampling` and not read again.
- Window height goes from 920 to 958.
