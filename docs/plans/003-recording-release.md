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
   - Hold until the note length. The tick that reaches it sends Note Off and moves to the release phase. There is no separate Note Off phase.
   - The next tick is the release phase. It adds one timer interval, then stops the recorder and goes to Storing if the release time is already reached, including a release of 0.
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

The engine stays a 50 ms message-thread timer. One phase per tick. Recorded release is the slider value, within one timer interval, same accuracy as note length. A release of 0 stops on the tick after Note Off.

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
SendingNoteOn → Recording → Releasing → Storing → next note
```

- `SendingNoteOn` allocates `noteDuration + releaseDuration + 0.5` and sends Note On, as today.
- `Recording` waits for the note length. The tick that reaches it sends Note Off and moves to `Releasing`. It does not run the release phase on that same tick.
- `Releasing` adds one timer interval per tick. At or past the release time it stops the recorder and goes to `Storing`. A release of 0 takes that path on the tick after Note Off.
- `Storing` is unchanged.

`startSampling` takes `releaseDuration` after the note length. `AutosamplerState::releaseDuration` defaults to 1.0. `MainComponent` passes the slider value.

No change to `RecordingManager`, exporters, `StartDetector`, or preview.

---

## Testing criteria

- [ ] Release 0: Note Off, then the next tick stores. No separate Note Off phase and no zero-release branch.
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
- Note Off is sent from `Recording` when the note length is reached, and the phase changes to `Releasing` without running it on that tick. There is no `SendingNoteOff` phase and no fallthrough. The status line is posted only on the first release interval, and only if that interval has not already finished the release.
