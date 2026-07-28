# DSP Design

## Signal path

Input audio passes through five serial EQ bands and then the smoothed output-gain stage. The bypass control blends the processed signal with the original sample.

## Biquad processing

Each band uses a Direct Form II Transposed structure:

```text
y[n] = b0*x[n] + z1
z1   = b1*x[n] - a1*y[n] + z2
z2   = b2*x[n] - a2*y[n]
```

The implementation stores separate state for left and right channels. Mono input uses only the left filter state and does not pretend that a second channel exists.

## Parameter smoothing

Frequency, gain, and Q use short smoothing ramps. This prevents abrupt coefficient changes from producing clicks during normal control movement or host automation. Coefficients are recalculated from the audio-owned smoothed values.

## Frequency-response display

The display samples 512 logarithmically spaced frequencies between 20 Hz and 20 kHz. It calculates each active band's complex response and multiplies the band responses before converting the result to decibels. This work is performed outside the audio callback.

## Design limits

The current implementation is a focused parametric EQ rather than a full commercial analyzer. It does not include FFT spectrum analysis, dynamic EQ, linear-phase processing, oversampling, or preset management.
