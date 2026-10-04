# Autosampler — reader notes

Durable context for anyone continuing this project. How an agent should work is in [AGENTS.md](../AGENTS.md).

## What the app is

A standalone JUCE desktop application that automatically samples a hardware or software MIDI instrument:

1. Sends MIDI Note On for a range of notes, holds each note for a fixed length, sends Note Off, and keeps recording for a release time before the next note. Channel and velocity are chosen once for the run.
2. Records audio from the input configured in the standard JUCE audio settings (`AudioDeviceManager`).
3. Stores each note's recording separately in memory.
4. Exports the recordings as an instrument in an open format: SFZ, SF2, or Decent Sampler.

## Current status

- v1 is built and working. It was implemented from [original-development-plan.md](original-development-plan.md).
- The codebase is the source of truth. That plan is historical and contains some plugin-era naming that may not match the code.
- Sample preview is in the code (keyboard, 8 voices). Its plan checklist is still open: [plans/001-sample-preview.md](plans/001-sample-preview.md).
- Adjustable sample start and attack is in the code. Plan: [plans/002-sample-start-and-attack.md](plans/002-sample-start-and-attack.md).
- Recording release time is in the code. After the note length, Note Off is sent and recording continues for the release time, so the previous note's tail is not written into the next take. Plan: [plans/003-recording-release.md](plans/003-recording-release.md).

## v1 UI controls

- Sample pack name (text input)
- Start note (0–127)
- End note (0–127)
- Note length in seconds (double)
- Release time in seconds (0–10, default 1). Recording continues this long after Note Off.
- Velocity (0–127, default 100)
- MIDI channel (dropdown 1–16)
- Run button
- Export format (dropdown: SFZ / SF2 / Decent Sampler)
- Export button

## Decisions and conventions

Do not change these unless a feature plan says to.

- Standalone desktop app only. No AU/VST plugin for now.
- The app has no relation to Claude or any product called "Opus". Claude Opus is only the AI model used to build it.
- Note naming uses sharps with "#": C, C#, D, D#, E, F, F#, G, G#, A, A#, B.
- Octave convention: middle C = C4 = MIDI 60. Formula: `octave = note / 12 - 1`. So MIDI 48 = C3, MIDI 36 = C2, MIDI 84 = C6.
- Exported WAV naming: `{samplePackName}_{NoteName}.wav`, e.g. `MySamples_C3.wav` for MIDI 48, `MySamples_C#3.wav` for MIDI 49.
- The sample pack name is the default export library/instrument name (e.g. `MySamples.sfz`).

## Feature plans

A new feature is one markdown file in [plans/](plans/): what to build, how it fits the existing code, and a checklist in the same file. No status folders, no separate template, no review document.

## Known considerations

"#" in WAV filenames is fine on macOS, Windows, and Linux, and in most SFZ / Decent Sampler players, but worth verifying with each target sampler. Any tool that treats paths as URLs will need "#" escaped.

## Where things live

- [plans/](plans/) — feature implementation plans
- [original-development-plan.md](original-development-plan.md) — historical plan v1 was built from
