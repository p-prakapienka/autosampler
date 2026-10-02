# Autosampler

Standalone JUCE desktop app that samples a hardware or software MIDI instrument and exports the recordings as SFZ, SF2, or Decent Sampler.

## What it does

1. Sends MIDI Note On/Off for a range of notes (fixed length, chosen channel and velocity).
2. Records audio from the input selected in the JUCE audio settings.
3. Keeps each note's recording in memory.
4. Exports an instrument in SFZ, SF2, or Decent Sampler.

## Build

CMake 3.22+ and a C++17 compiler. JUCE 8.0.12 is fetched during configure.

```bash
cmake -B build
cmake --build build
```

The target is `JuceAutosampler` (product name "JUCE Autosampler").

## Download a build

Pushes to `main` (and manual runs) build a Windows x64 app. Open the latest green run of [Build](https://github.com/p-prakapienka/autosampler/actions/workflows/build.yml) and download `autosampler-windows-x64`.

GitHub wraps the artifact in a zip. Unzip that, then unpack the `.tar.gz` inside:

```bash
tar -xzf autosampler-windows-x64.tar.gz
```

Run `JUCE Autosampler.exe`. If Windows reports a missing DLL, install the latest Microsoft Visual C++ Redistributable (x64).

## Status

v1 works. Decisions, UI, and naming conventions: [docs/reader.md](docs/reader.md).

Next feature: [docs/plans/001-sample-preview.md](docs/plans/001-sample-preview.md) (sample preview).

## Docs

All project docs live in [docs/](docs/):

| File | What it is |
| --- | --- |
| [docs/reader.md](docs/reader.md) | Product context, status, conventions |
| [docs/plans/](docs/plans/) | Feature implementation plans, checklist inside each plan |
| [docs/original-development-plan.md](docs/original-development-plan.md) | Historical plan v1 was built from |

Coding agents: start at [AGENTS.md](AGENTS.md). The code is the source of truth; the original development plan is reference only.
