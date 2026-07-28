# 5-Band Parametric Equalizer

A five-band parametric equalizer plugin built with C++, JUCE, and CMake. The project demonstrates practical audio DSP, real-time-safe C++ programming, host parameter automation, and maintainable plugin architecture.

## Current release scope

The supported Windows targets are:

- Standalone application for local audio testing.
- VST3 plugin for DAW use.

AU and AAX are not part of the current release. They are not claimed or tested.

## Features

- Five serial biquad EQ bands.
- Gain, frequency, Q, and filter-type controls for every band.
- Band-specific filter restrictions:
  - Band 1: low cut and low shelf.
  - Bands 2-4: peak, low shelf, and high shelf.
  - Band 5: high cut and high shelf.
- Smoothed parameter changes for audible controls.
- APVTS-based host automation and state recall.
- Mono and stereo processing paths.
- Smoothed output gain.
- Click-resistant bypass transition.
- Logarithmic combined frequency-response display.
- Automated tests for filter restrictions and coefficient behavior.

## Project status

The source-controlled CMake build has been added and successfully compiled with the available Visual Studio 2022/MSVC toolchain and JUCE 8.0.10 installation. Release Standalone and VST3 targets build successfully, the VST3 bundle contains generated metadata, and the DSP test target passes under CTest. The Standalone application has also been manually inspected.

Formal DAW validation, screenshots, and final portfolio evidence remain pending until they are performed and recorded with a host version and test date.

## Repository structure

```text
5-band-vst-equalizer/
├── CMakeLists.txt
├── CMakePresets.json
├── LICENSE
├── README.md
├── IMPLEMENTATION.md
├── docs/
│   ├── BUILDING.md
│   ├── DSP.md
│   ├── TESTING.md
│   └── DEVELOPMENT_PLAN.md
├── Tests/
│   └── DspTests.cpp
└── 5BandEQ/Source/
    ├── PluginProcessor.h/.cpp
    ├── PluginEditor.h/.cpp
    ├── DSP/
    │   ├── BiquadFilter.h/.cpp
    │   ├── EQBand.h/.cpp
    │   └── FilterTypes.h
    ├── GUI/
    │   └── FrequencyResponseDisplay.h/.cpp
    └── Utils/
        ├── Constants.h
        └── Parameters.h
```

Generated build folders, JUCE wrappers, IDE state, binaries, and intermediate files are intentionally not tracked.

## Build requirements

- Windows 10 or later.
- Visual Studio 2022 with Desktop development with C++ or Visual Studio 2026 with the equivalent C++ workload.
- CMake 3.24 or later.
- JUCE 8.0.10 or a compatible JUCE 8 installation.
- Git.

The build expects JUCE at `C:/JUCE` by default. Override it when configuring if necessary.

## Build with Visual Studio

Open the repository folder in Visual Studio. Select the appropriate CMake preset and build one of these targets:

- `FiveBandEQ_Standalone`
- `FiveBandEQ_VST3`
- `FiveBandEQ_Tests`

The project includes presets for Visual Studio 2026 and a compatibility preset for the currently installed Visual Studio 2022 generator.

## Build from PowerShell

For Visual Studio 2022 compatibility:

```powershell
cmake --preset windows-vs2022
cmake --build build/windows-vs2022 --config Release --target FiveBandEQ_Standalone FiveBandEQ_VST3 FiveBandEQ_Tests
```

For Visual Studio 2026:

```powershell
cmake --preset windows-release
cmake --build build/windows-release --config Release --target FiveBandEQ_Standalone FiveBandEQ_VST3 FiveBandEQ_Tests
```

To use another JUCE location:

```powershell
cmake -S . -B build/custom -G "Visual Studio 18 2026" -A x64 -DJUCE_DIR="D:/libraries/JUCE"
```

## Output and installation

The generated Release artifacts are placed below:

```text
build/<preset>/FiveBandEQ_artefacts/Release/Standalone/5-Band EQ.exe
build/<preset>/FiveBandEQ_artefacts/Release/VST3/5-Band EQ.vst3
```

For local DAW testing, copy the VST3 bundle to:

```text
C:\Program Files\Common Files\VST3\
```

Do not commit built plugin bundles to Git.

## Testing

Run the focused DSP test executable through the selected build target. The tests currently cover coefficient finiteness, neutral peak behavior, and band filter restrictions.

The intended manual host-validation target is REAPER. The validation checklist is in [`docs/TESTING.md`](docs/TESTING.md). DAW compatibility is not claimed until that checklist has been completed.

## DSP overview

Each band uses a stereo pair of Direct Form II Transposed biquad filters. Parameter targets are smoothed over a short interval to reduce clicks during automation and manual control changes. Coefficients are updated from audio-owned state, while APVTS remains the host-facing parameter source.

The response display calculates the combined magnitude response using the same filter coefficient model. It runs on the message thread and is not part of the audio callback.

More detail is available in [`docs/DSP.md`](docs/DSP.md) and [`IMPLEMENTATION.md`](IMPLEMENTATION.md).

## Known limitations and next steps

- REAPER validation and screenshots are still pending.
- There is no spectrum analyzer.
- There is no preset browser or factory-preset system.
- There is no installer or code signing.
- AU and AAX are not supported by this Windows release.
- The project currently depends on a locally installed JUCE tree rather than vendoring JUCE.

The next high-value work is Release verification, host testing, screenshots, and measured documentation—not adding a large feature set.

## License

The project source is released under the MIT License. JUCE remains subject to its own license terms.
