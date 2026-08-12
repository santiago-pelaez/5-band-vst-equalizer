# Changelog

## Unreleased

- Saved the expanded roadmap for the combined EQ-curve, analyzer, and node workflow.
- Cached APVTS parameter pointers to keep audio-thread synchronization allocation-free.
- Added defensive sample-rate, coefficient, and response handling.
- Centralized biquad magnitude-response calculation for the response display and tests.
- Added regression coverage for invalid inputs and positive peak response.
- Added parameter conversion, default-band-state, and APVTS state-recall tests.
- Added source-controlled CMake configuration for Standalone, VST3, and DSP tests.
- Removed dependence on the ignored generated `JuceHeader.h` wrapper.
- Compiled the frequency-response implementation as a normal source file.
- Added mono-safe processing and smoothed bypass blending.
- Removed the incorrect behavior that skipped cut filters at 0 dB gain.
- Added coefficient bounds and initial DSP tests.
- Added development, build, DSP, and testing documentation.
