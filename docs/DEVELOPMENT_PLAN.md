# 5-Band JUCE EQ Development Plan

## Product direction

The final plugin will follow an Ableton-style EQ workflow:

- One top graph showing input spectrum, output spectrum, the combined EQ response, and draggable EQ nodes.
- Nodes edit frequency and gain directly.
- A detailed band-control panel below the graph edits frequency, gain, Q, filter type, and enable state.
- REAPER is the official validation host.
- Ableton Live is a secondary host for frequent testing.
- The supported release targets are Windows Standalone and VST3.

AU and AAX remain out of scope unless they are separately built and tested.

## Programming and engineering preferences

- Use verbose, descriptive C++ names and explicit control flow.
- Prefer a never-nest style with guard clauses, early returns, and shallow indentation.
- Keep audio processing, parameters, GUI, analysis, and tests in separate components.
- Avoid clever abstractions that make DSP or thread ownership difficult to audit.
- Explain important real-time and DSP decisions in comments and documentation.
- Do not claim functionality that has not been built and tested.

## Phase 1 — DSP and parameter foundation

The first implementation phase makes the existing EQ technically trustworthy and prepares it for future node and analyzer work.

### Audio-thread safety

- Cache APVTS parameter pointers during processor construction.
- Remove avoidable `juce::String` construction and temporary vectors from audio-thread synchronization.
- Replace dynamic filter-choice lookup with fixed, allocation-free mappings.
- Keep analyzer buffers preallocated during `prepareToPlay` when analyzer work begins.
- Keep locks, file I/O, logging, GUI calls, and repainting out of `processBlock`.

### DSP correctness

- Validate sample rates before coefficient calculation.
- Guard against invalid coefficient denominators and non-finite results.
- Preserve independent left/right filter state.
- Keep smoothing for frequency, gain, Q, output gain, and bypass.
- Ensure cut filters remain active at zero gain.
- Make output gain and bypass behavior explicit when used together.

### Shared response mathematics

The response graph must use the same coefficient model as the audio processor. The placeholder processor response method will be removed, and shared magnitude-response calculation will live with the biquad DSP code.

### Phase 1 acceptance criteria

- No avoidable audio-thread allocations or locks.
- Stable processing during normal parameter changes.
- Finite coefficients and output samples.
- Existing Release targets and DSP tests continue to pass.
- Parameter synchronization remains easy to inspect in Visual Studio or VS Code.

## Phase 2 — UI architecture and lower control panel

The current editor will be refactored into focused components:

- A reusable band-control component for one EQ band.
- A combined EQ response and node display.
- A future spectrum analyzer display.
- Shared theme/look-and-feel code.
- A top-level editor responsible primarily for layout and coordination.

The lower panel will contain five band sections. Each section will show the band name, enable state, frequency, gain, Q, and restricted filter-type selector. Global Bypass and Output Gain controls will be clearly visible.

Low-cut and high-cut bands will expose frequency and Q normally, while their gain control will be disabled or marked as non-applicable because those filters do not use gain meaningfully.

Clicking a node will highlight its detailed band panel. Editing either the node or the detailed controls will update the same APVTS parameter source.

The layout will have a larger default size, a sensible minimum size, consistent spacing, readable units, and safe resize behavior. Draggable nodes and analyzer rendering will not be implemented as large monolithic additions to `PluginEditor`.

## Phase 3 — Combined graph and EQ nodes

The top graph will contain four visual layers:

1. Input spectrum.
2. Output spectrum.
3. Combined EQ response.
4. Interactive EQ nodes.

The graph will use one logarithmic frequency axis. The EQ curve will use an EQ-gain scale of approximately -24 dB to +24 dB, while the analyzer will use a clearly labeled spectrum scale of approximately -96 dBFS to 0 dBFS. This preserves the shared frequency relationship without pretending that EQ gain and signal level are the same measurement.

Enabled bands will show nodes. Disabled bands will hide nodes. Low-cut and high-cut nodes will remain centered at 0 dB and primarily support horizontal frequency editing. Peak and shelf nodes will support horizontal frequency editing and vertical gain editing. Q will initially remain in the lower control panel.

Node drags will use host-notifying APVTS gestures so DAW automation, undo behavior, state recall, and host displays remain correct. Node code will never maintain a second parameter state or directly mutate audio-owned DSP state.

## Phase 4 — Practical real-time spectrum analyzer

The analyzer will capture input audio before EQ processing and output audio after the complete processor path. It will display both traces in different colors over the EQ graph.

Initial technical defaults are:

- 2048-point FFT.
- Hann window.
- Approximately 50% overlap.
- Approximately 30 Hz GUI refresh rate.
- A defined dBFS noise floor near -96 dBFS.
- Smoothed visual magnitudes.

The audio thread will use preallocated FFT state and publish fixed-size frames through a lock-free single-producer/single-consumer buffer. The GUI will poll the newest frame on a timer and will never block the audio thread.

Analyzer tests will cover sine-wave peaks, silence, finite values, sample-rate changes, snapshot exchange, and confirmation that analysis does not alter the audible buffer.

## Phase 5 — Automated verification

Tests will cover:

- All filter families and band restrictions.
- Boundary gains, Q values, frequencies, and sample rates.
- Finite coefficients and stable output.
- Neutral peak behavior.
- Low-cut DC attenuation and high-cut Nyquist attenuation.
- Shared response calculations.
- Mono and stereo processing.
- Parameter smoothing, output gain, and bypass transitions.
- Parameter identifiers, ranges, defaults, and state recall.
- Node-to-APVTS mapping and disabled/cut-node behavior.
- Analyzer peak detection and audio-buffer preservation.

Debug and Release builds will run the complete test target through CTest.

## Phase 6 — Host validation

REAPER is the formal validation target. The checklist will cover VST3 scanning, mono and stereo tracks, every filter type, all visible parameters, node editing, automation, bypass, output gain, state save/reload, analyzer playback, and ordinary parameter movement without audible glitches.

The exact REAPER version and test date will be recorded.

Ableton Live will be used for frequent secondary testing. It will be mentioned publicly only after its exact version, format, and validation date are recorded.

## Phase 7 — Documentation and portfolio release

After functionality is verified, update the README with:

- The exact feature list.
- The combined analyzer/EQ graph behavior.
- Draggable frequency/gain nodes.
- Lower-panel fine tuning and Q control.
- Windows Standalone and VST3 support.
- REAPER validation results.
- Ableton results when formally recorded.
- DSP and lock-free analyzer explanations.
- Screenshots and a short demo-video plan.
- Known limitations and deferred features.

Resume bullets will be written only from verified behavior.

## Deferred enhancements

- Factory presets and a preset browser.
- Optional modifier-based Q editing.
- Analyzer freeze, hold, and averaging modes.
- Installer and code signing.
- Additional platform formats.

## Final definition of done

- The top graph shows input spectrum, output spectrum, EQ response, and draggable nodes.
- Nodes edit frequency and gain through host-notifying APVTS gestures.
- The lower panel provides exact frequency, gain, Q, type, enable, bypass, and output controls.
- The analyzer uses a preallocated lock-free audio-to-GUI path.
- Automated DSP, parameter, state, node, and analyzer tests pass.
- Standalone testing passes.
- REAPER validation is documented.
- Ableton testing is documented separately when appropriate.
- README claims match demonstrated behavior.
- The repository remains reproducible and auditable in Visual Studio and VS Code.
