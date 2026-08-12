#include "DSP/SpectrumAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
    void requireCondition(bool condition, const std::string &failureMessage)
    {
        if (condition)
            return;

        std::cerr << "Test failure: " << failureMessage << '\n';
        std::exit(EXIT_FAILURE);
    }

    void feedSamples(SpectrumAnalyzer &analyzer,
                     int sampleCount,
                     double sampleRate,
                     float frequency,
                     float inputAmplitude,
                     float outputAmplitude)
    {
        for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
        {
            const float phase = static_cast<float>(2.0 * juce::MathConstants<double>::pi
                                                   * frequency
                                                   * static_cast<double>(sampleIndex)
                                                   / sampleRate);
            const float inputSample = inputAmplitude * std::sin(phase);
            const float outputSample = outputAmplitude * std::sin(phase);
            analyzer.pushInputSample(inputSample, inputSample);
            analyzer.pushOutputSample(outputSample, outputSample);
        }
    }

    int findPeakBin(const std::array<float, SpectrumAnalyzer::SPECTRUM_BINS> &spectrum)
    {
        return static_cast<int>(std::distance(spectrum.begin(),
                                              std::max_element(spectrum.begin(), spectrum.end())));
    }

    void requireFiniteSpectrum(const SpectrumAnalyzer::SpectrumFrame &frame)
    {
        for (const float value : frame.inputSpectrum)
            requireCondition(std::isfinite(value), "input spectrum contains a non-finite value");

        for (const float value : frame.outputSpectrum)
            requireCondition(std::isfinite(value), "output spectrum contains a non-finite value");
    }

    void testCompleteFrameWarmupAndSinePeak()
    {
        constexpr double sampleRate = 48000.0;
        constexpr float testFrequency = 1000.0f;

        SpectrumAnalyzer analyzer;
        analyzer.prepare(sampleRate, 256);

        feedSamples(analyzer,
                    SpectrumAnalyzer::FFT_SIZE - 1,
                    sampleRate,
                    testFrequency,
                    0.5f,
                    0.25f);

        SpectrumAnalyzer::SpectrumFrame frame;
        requireCondition(! analyzer.readLatestFrame(frame),
                         "analyzer published a frame before the FFT history was full");

        feedSamples(analyzer, 1, sampleRate, testFrequency, 0.5f, 0.25f);
        requireCondition(analyzer.readLatestFrame(frame),
                         "analyzer did not publish the first complete frame");

        const int peakBin = findPeakBin(frame.inputSpectrum);
        const float peakFrequency = static_cast<float>(peakBin) * static_cast<float>(sampleRate)
                                  / static_cast<float>(SpectrumAnalyzer::FFT_SIZE);
        requireCondition(std::abs(peakFrequency - testFrequency) < 75.0f,
                         "sine peak was not located near the expected frequency");
        requireFiniteSpectrum(frame);
    }

    void testInputAndOutputLevelsRemainDistinct()
    {
        constexpr double sampleRate = 44100.0;

        SpectrumAnalyzer analyzer;
        analyzer.prepare(sampleRate, 512);
        feedSamples(analyzer,
                    SpectrumAnalyzer::FFT_SIZE,
                    sampleRate,
                    440.0f,
                    0.5f,
                    0.25f);

        SpectrumAnalyzer::SpectrumFrame frame;
        requireCondition(analyzer.readLatestFrame(frame),
                         "missing level comparison frame");

        const int peakBin = findPeakBin(frame.inputSpectrum);
        const float levelDifference = frame.inputSpectrum[static_cast<size_t>(peakBin)]
                                    - frame.outputSpectrum[static_cast<size_t>(peakBin)];
        requireCondition(levelDifference > 4.0f && levelDifference < 8.0f,
                         "input and output levels were not preserved as separate traces");
    }

    void testStereoMagnitudesAreAveraged()
    {
        constexpr double sampleRate = 48000.0;
        constexpr float leftFrequency = 500.0f;
        constexpr float rightFrequency = 2000.0f;

        SpectrumAnalyzer analyzer;
        analyzer.prepare(sampleRate, 256);

        for (int sampleIndex = 0; sampleIndex < SpectrumAnalyzer::FFT_SIZE; ++sampleIndex)
        {
            const double time = static_cast<double>(sampleIndex) / sampleRate;
            const float left = 0.5f * std::sin(static_cast<float>(2.0 * juce::MathConstants<double>::pi
                                                                   * leftFrequency * time));
            const float right = 0.5f * std::sin(static_cast<float>(2.0 * juce::MathConstants<double>::pi
                                                                    * rightFrequency * time));
            analyzer.pushInputSample(left, right);
            analyzer.pushOutputSample(left, right);
        }

        SpectrumAnalyzer::SpectrumFrame frame;
        requireCondition(analyzer.readLatestFrame(frame),
                         "missing stereo averaging frame");

        const int leftBin = static_cast<int>(std::round(leftFrequency * SpectrumAnalyzer::FFT_SIZE
                                                        / sampleRate));
        const int rightBin = static_cast<int>(std::round(rightFrequency * SpectrumAnalyzer::FFT_SIZE
                                                         / sampleRate));
        requireCondition(frame.inputSpectrum[static_cast<size_t>(leftBin)] > -30.0f,
                         "left-channel spectral content was lost");
        requireCondition(frame.inputSpectrum[static_cast<size_t>(rightBin)] > -30.0f,
                         "right-channel spectral content was lost");
    }

    void testSilenceAndReset()
    {
        SpectrumAnalyzer analyzer;
        analyzer.prepare(48000.0, 128);
        feedSamples(analyzer,
                    SpectrumAnalyzer::FFT_SIZE,
                    48000.0,
                    1000.0f,
                    0.0f,
                    0.0f);

        SpectrumAnalyzer::SpectrumFrame frame;
        requireCondition(analyzer.readLatestFrame(frame), "missing silence frame");
        requireFiniteSpectrum(frame);
        requireCondition(frame.inputSpectrum[10] <= -95.0f,
                         "silence did not reach the analyzer noise floor");

        analyzer.reset();
        requireCondition(! analyzer.readLatestFrame(frame),
                         "reset left an old frame available");
    }
}

int main()
{
    testCompleteFrameWarmupAndSinePeak();
    testInputAndOutputLevelsRemainDistinct();
    testStereoMagnitudesAreAveraged();
    testSilenceAndReset();

    std::cout << "All spectrum analyzer tests passed.\n";
    return EXIT_SUCCESS;
}
