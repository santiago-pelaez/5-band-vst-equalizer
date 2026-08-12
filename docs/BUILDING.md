# Building and Auditing

## Required tools

- Visual Studio 2026 or Visual Studio 2022 with Desktop development with C++.
- CMake 3.24 or later.
- A local JUCE installation.

## Configure

The default JUCE location is `C:/JUCE`.

```powershell
cmake --preset windows-vs2022
```

On Visual Studio 2026, use:

```powershell
cmake --preset windows-release
```

To override JUCE:

```powershell
cmake -S . -B build/custom -G "Visual Studio 18 2026" -A x64 -DJUCE_DIR="D:/libraries/JUCE"
```

## Build targets

```powershell
cmake --build build/windows-vs2022 --config Debug --target FiveBandEQ_Standalone
cmake --build build/windows-vs2022 --config Debug --target FiveBandEQ_VST3
cmake --build build/windows-vs2022 --config Debug --target FiveBandEQ_Tests FiveBandEQ_ParameterTests FiveBandEQ_SpectrumTests FiveBandEQ_ProcessorTests
```

## Debugging in Visual Studio

Open the repository folder. Select the CMake configuration and target, then set breakpoints in:

- `FiveBandEQProcessor::prepareToPlay`;
- `FiveBandEQProcessor::processBlock`;
- `FiveBandEQProcessor::synchronizeParametersFromHost`;
- `EQBand::prepareForProcessing`;
- `BiquadFilter::processSample`;
- `FrequencyResponseDisplay::calculateResponseCurve`.

Inspect channel counts, parameter values, smoother targets, coefficients, and the call stack. The first five locations are audio or DSP code; the last location is UI-thread code and should not appear in the audio callback stack.

## Debugging in VS Code

Install the CMake Tools and C/C++ extensions. Open the repository root, select a CMake preset, configure, select a target, and use the generated launch configuration. VS Code and Visual Studio use the same CMake source and therefore should produce the same targets.
