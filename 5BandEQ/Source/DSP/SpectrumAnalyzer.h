#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cstdint>

/**
 * Real-time input/output spectrum analysis for the EQ display.
 *
 * Audio processing writes samples and publishes complete fixed-size frames.
 * The editor reads completed frames through a single-producer/single-consumer
 * FIFO. No GUI work, dynamic allocation, or locking occurs in the audio path.
 */
class SpectrumAnalyzer
{
public:
    static constexpr int FFT_ORDER = 11;
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int HOP_SIZE = FFT_SIZE / 2;
    static constexpr int SPECTRUM_BINS = FFT_SIZE / 2 + 1;
    static constexpr int FRAME_QUEUE_SIZE = 4;

    struct SpectrumFrame
    {
        double sampleRate = 44100.0;
        std::uint64_t sequenceNumber = 0;
        std::array<float, SPECTRUM_BINS> inputSpectrum{};
        std::array<float, SPECTRUM_BINS> outputSpectrum{};
    };

    SpectrumAnalyzer();

    void prepare(double newSampleRate, int maximumBlockSize);
    void reset();

    void pushInputSample(float leftSample, float rightSample);
    void pushOutputSample(float leftSample, float rightSample);

    bool readLatestFrame(SpectrumFrame &destination);

private:
    using HistoryBuffer = std::array<float, FFT_SIZE>;
    using FFTBuffer = std::array<float, FFT_SIZE * 2>;
    using MagnitudeBuffer = std::array<float, SPECTRUM_BINS>;

    static constexpr float MINIMUM_DISPLAY_DB = -96.0f;
    static constexpr float WINDOW_GAIN = 0.5f;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    juce::AbstractFifo frameFifo;
    std::array<SpectrumFrame, FRAME_QUEUE_SIZE> frameQueue;

    HistoryBuffer inputLeftHistory{};
    HistoryBuffer inputRightHistory{};
    HistoryBuffer outputLeftHistory{};
    HistoryBuffer outputRightHistory{};

    FFTBuffer inputLeftFFT{};
    FFTBuffer inputRightFFT{};
    FFTBuffer outputLeftFFT{};
    FFTBuffer outputRightFFT{};

    MagnitudeBuffer inputLeftMagnitude{};
    MagnitudeBuffer inputRightMagnitude{};
    MagnitudeBuffer outputLeftMagnitude{};
    MagnitudeBuffer outputRightMagnitude{};

    double sampleRate = 44100.0;
    int writeIndex = 0;
    int samplesWritten = 0;
    int samplesSinceLastFrame = 0;
    std::uint64_t nextSequenceNumber = 1;

    void calculateSpectrum(const HistoryBuffer &history,
                           FFTBuffer &fftData,
                           MagnitudeBuffer &magnitudes);
    void publishFrame();
    static float magnitudeToDecibels(float magnitude);
    static float decibelsToMagnitude(float decibels);
    static float averageMagnitudeDecibels(float firstDecibels, float secondDecibels);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};
