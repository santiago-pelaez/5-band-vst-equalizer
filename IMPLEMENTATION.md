# Implementation Notes

## Architecture

The plugin is split into host integration, DSP, parameter definitions, and UI components.

`PluginProcessor` owns the `AudioProcessorValueTreeState`, audio-owned EQ bands, output-gain smoothing, bypass smoothing, and state serialization. `PluginEditor` owns controls and APVTS attachments. `FrequencyResponseDisplay` is a message-thread GUI component and does not participate in audio processing.

## Parameter flow

1. The host or UI changes an APVTS parameter.
2. The APVTS listener marks synchronization as pending.
3. At construction time, the processor caches raw parameter pointers. At the start of the next audio block, it reads those atomic values without constructing parameter-ID strings.
4. Audio-owned band targets and smoothers are updated.
5. The audio callback processes samples without taking a lock or allocating memory.

This separates host/UI parameter ownership from filter state used by the audio callback.

## Filter model

The EQ uses Direct Form II Transposed biquads. Each band has independent left and right filter state. Supported coefficient families are peaking EQ, low shelf, high shelf, high-pass exposed as Low Cut, and low-pass exposed as High Cut.

Frequency, gain, Q, and coefficient inputs are bounded before coefficient calculation. The active frequency is kept below Nyquist to avoid invalid coefficient values.

## Real-time constraints

The audio callback is designed to avoid dynamic allocation, mutexes, file I/O, logging, GUI calls, and response-curve calculations. The response display performs its calculations on the message thread using the shared `BiquadFilter` response model.

## Bypass behavior

The processor keeps a dry sample and blends it with the processed sample using a smoothed bypass mix. This avoids an abrupt discontinuity when bypass is automated or toggled during playback.

## Build architecture

`CMakeLists.txt` is the source-controlled build definition. It creates Standalone, VST3, and test targets. Visual Studio and VS Code are front ends for the same CMake project. Generated JUCE wrappers and IDE project files are not required in the repository.

## Current verification state

- CMake configuration succeeds with JUCE 8.0.10 and the installed Visual Studio 2022 toolchain.
- Debug Standalone and VST3 targets compile.
- DSP tests compile and pass.
- Release target compilation has been exercised, but final Release acceptance and REAPER validation remain pending.
