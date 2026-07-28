# 5-Band JUCE EQ Development Plan

## Project objective

Transform the existing 5-Band VST Equalizer into a verified, maintainable, recruiter-ready C++/JUCE project demonstrating real-time DSP, safe audio-thread engineering, parameter automation, state management, reproducible Windows builds, and professional testing.

The first release target is Windows Standalone plus VST3, validated in REAPER. AU and AAX are out of scope unless separately built and tested.

## Programming preferences

- Use verbose, descriptive names and explicit code.
- Prefer small single-purpose functions over dense multipurpose functions.
- Use a never-nest style whenever practical: guard clauses, early returns, shallow indentation, and separate validation from processing.
- Explain important DSP and real-time decisions in comments and documentation.
- Keep audio processing, parameter management, UI, and tests clearly separated.
- Avoid clever abstractions that make the project harder to audit.
- Do not optimize for minimum line count.
- Do not claim functionality that has not been built and tested.

## Scope

### Included

- Source-controlled CMake build configuration for Visual Studio 2026 and VS Code.
- Windows Standalone and VST3 targets.
- Five-band biquad EQ processing with band-specific filter restrictions.
- Parameter smoothing, safe mono/stereo processing, safe bypass, APVTS automation, and state recall.
- Frequency-response visualization.
- DSP and parameter tests.
- REAPER validation.
- Honest README, technical documentation, MIT license, and publication-readiness audit.

### Deferred

AU, AAX, spectrum analysis, preset management, installers, code signing, draggable EQ nodes, undo/redo, and commercial-plugin feature parity.

## Implementation sequence

1. Preserve the baseline, inspect Git state, and scan for secrets and artifacts.
2. Add CMake presets and build targets using a configurable `JUCE_DIR`.
3. Normalize compilation so `.cpp` files are compiled normally rather than included directly.
4. Correct mono/stereo handling, parameter-to-DSP synchronization, coefficient validation, smoothing, and bypass transitions.
5. Add automated tests for filter math, restrictions, parameters, state, and audio behavior.
6. Build clean Debug and Release Standalone/VST3 targets and run tests.
7. Validate the plugin manually in the Standalone app and REAPER.
8. Rewrite documentation and publication metadata based only on verified behavior.

## Audit workflow

Visual Studio 2026 and VS Code will use the same CMake configuration. The repository will document configuring, building, testing, setting breakpoints, inspecting parameters and coefficients, reading call stacks, and using compiler diagnostics. Important audit points include `prepareToPlay`, `processBlock`, `EQBand::processStereo`, `BiquadFilter::processSample`, and the response-display calculation.

## Definition of done

- A clean checkout configures with `JUCE_DIR` and builds Standalone plus VST3.
- Automated tests pass.
- Mono and stereo processing are safe.
- Cut filters remain active at 0 dB gain.
- Parameter changes and bypass transitions are stable under normal use.
- REAPER validation is complete and documented.
- README claims match demonstrated behavior.
- No unsupported formats or unverified performance claims are published.
- No secrets or build artifacts are tracked.

No public release, push, or repository visibility change occurs without explicit approval.
