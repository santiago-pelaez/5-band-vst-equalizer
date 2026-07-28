# Changelog

## Unreleased

- Added source-controlled CMake configuration for Standalone, VST3, and DSP tests.
- Removed dependence on the ignored generated `JuceHeader.h` wrapper.
- Compiled the frequency-response implementation as a normal source file.
- Added mono-safe processing and smoothed bypass blending.
- Removed the incorrect behavior that skipped cut filters at 0 dB gain.
- Added coefficient bounds and initial DSP tests.
- Added development, build, DSP, and testing documentation.
