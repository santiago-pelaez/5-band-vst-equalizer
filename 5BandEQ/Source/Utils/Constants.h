#pragma once

namespace EQConstants
{
    // Band Configuration
    static constexpr int NUM_BANDS = 5;

    // Audio Processing
    static constexpr double SAMPLE_RATE_DEFAULT = 44100.0;
    static constexpr int BUFFER_SIZE_DEFAULT = 512;

    // Filter Parameters
    static constexpr float FREQUENCY_MIN = 20.0f;
    static constexpr float FREQUENCY_MAX = 20000.0f;
    static constexpr float GAIN_MIN = -24.0f;
    static constexpr float GAIN_MAX = 24.0f;
    static constexpr float Q_MIN = 0.1f;
    static constexpr float Q_MAX = 10.0f;

    // Default Band Frequencies
    static constexpr float BAND_FREQUENCIES[NUM_BANDS] = {
        80.0f,   // Band 1: Low (cut/shelf only)
        300.0f,  // Band 2: Low-mid (full parametric)
        1000.0f, // Band 3: Mid (full parametric)
        3000.0f, // Band 4: High-mid (full parametric)
        10000.0f // Band 5: High (cut/shelf only)
    };
}
