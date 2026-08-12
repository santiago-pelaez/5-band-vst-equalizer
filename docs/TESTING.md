# Testing Checklist

## Automated tests

Build and run `FiveBandEQ_Tests`, `FiveBandEQ_ParameterTests`, `FiveBandEQ_SpectrumTests`, and `FiveBandEQ_ProcessorTests`. Current checks include finite coefficients for all supported filter families, neutral peaking behavior, valid and invalid band filter restrictions, state/parameter conversion, FFT peak detection, stereo spectrum averaging, silence handling, analyzer reset behavior, actual processor mono/stereo behavior, channel isolation, output-gain smoothing, bypass, and analyzer integration.

## Standalone test

- Launch the Release Standalone application.
- Confirm the application opens and closes cleanly.
- Select an audio device and verify input/output routing.
- Test mono input and stereo input separately.
- Confirm the input and output spectrum traces appear in different colors during playback.
- Confirm the output trace responds to EQ changes while the input trace remains representative of the input.
- Move gain, frequency, Q, and filter-type controls.
- Confirm the response curve follows the controls.
- Toggle bypass during playback.

## REAPER test

Detected local installation: REAPER 7.38 at `C:\Program Files\REAPER (x64)\reaper.exe`. The checklist below remains pending until manually performed.

- Copy the VST3 bundle to the configured VST3 directory.
- Rescan plugins in REAPER.
- Load the plugin on a stereo track.
- Load the plugin on a mono track.
- Confirm audio passes through at neutral settings.
- Test every supported filter type.
- Automate gain, frequency, Q, filter type, output gain, and bypass.
- Save and reload the project.
- Confirm parameter state is restored.
- Listen for clicks during ordinary parameter movement and bypass automation.
- Record the REAPER version and date in the README after completion.
