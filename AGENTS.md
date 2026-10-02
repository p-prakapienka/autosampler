# AGENTS.md

Instructions for coding agents working on Autosampler.

The codebase is the source of truth. Product decisions and conventions are in [docs/reader.md](docs/reader.md) — treat them as fixed unless a feature plan changes them. [docs/original-development-plan.md](docs/original-development-plan.md) is historical reference only. It uses plugin-era names (`PluginProcessor`, `processBlock`, `prepareToPlay`) that may not match the code.

## Feature work

A feature is one implementation plan in [docs/plans/](docs/plans/). The checklist lives in that file. There are no status folders and no separate review document.

Before writing code:

1. Read the plan and the existing source. Map any generic names onto the actual classes (audio callback, sample storage, main UI). Do not assume names from the original development plan.
2. If the plan conflicts with the code, or is ambiguous, ask before proceeding.

Then implement the plan and work through its checklist. Do not add behaviour the plan does not ask for; propose that separately. If the plan is wrong, edit the plan first.

## Conventions that must not change unless a plan says so

Full list: [docs/reader.md](docs/reader.md). In short:

- Standalone desktop app only. No AU/VST plugin unless a plan says so.
- Note names use sharps with `#`: C, C#, D, D#, E, F, F#, G, G#, A, A#, B.
- Middle C = C4 = MIDI 60. `octave = note / 12 - 1`.
- WAV names: `{samplePackName}_{NoteName}.wav` (for example `MySamples_C3.wav` for MIDI 48).
- The sample pack name is the default export library/instrument name.

## Audio thread

No allocation, no locking, and no UI calls on the audio thread.
