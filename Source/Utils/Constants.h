#pragma once

namespace EQConstants
{
    // Band Configuration
    static constexpr int NUM_BANDS = 5;

    // Audio Processing
    static constexpr double SAMPLE_RATE_DEFAULT = 44100.0;
    static constexpr int BUFFER_SIZE_DEFAULT = 512;

    // Filter Parameters
    static constexpr double FREQUENCY_MIN = 20.0;
    static constexpr double FREQUENCY_MAX = 20000.0;
    static constexpr double GAIN_MIN = -24.0;
    static constexpr double GAIN_MAX = 24.0;
    static constexpr double Q_MIN = 0.1;
    static constexpr double Q_MAX = 10.0;

    // Default Band Frequencies
    static constexpr double BAND_FREQUENCIES[NUM_BANDS] = {
        80.0,   // Band 1: Low (cut/shelf only)
        300.0,  // Band 2: Low-mid (full parametric)
        1000.0, // Band 3: Mid (full parametric)
        3000.0, // Band 4: High-mid (full parametric)
        10000.0 // Band 5: High (cut/shelf only)
    };
}