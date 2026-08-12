#include "SpectrumAnalyzer.h"

#include <algorithm>
#include <cmath>

SpectrumAnalyzer::SpectrumAnalyzer()
    : fft(FFT_ORDER),
      window(FFT_SIZE, juce::dsp::WindowingFunction<float>::hann, false),
      frameFifo(FRAME_QUEUE_SIZE)
{
    reset();
}

void SpectrumAnalyzer::prepare(double newSampleRate, int maximumBlockSize)
{
    juce::ignoreUnused(maximumBlockSize);

    sampleRate = std::isfinite(newSampleRate) && newSampleRate > 1.0
                   ? newSampleRate
                   : 44100.0;

    reset();
}

void SpectrumAnalyzer::reset()
{
    frameFifo.reset();

    inputLeftHistory.fill(0.0f);
    inputRightHistory.fill(0.0f);
    outputLeftHistory.fill(0.0f);
    outputRightHistory.fill(0.0f);

    inputLeftFFT.fill(0.0f);
    inputRightFFT.fill(0.0f);
    outputLeftFFT.fill(0.0f);
    outputRightFFT.fill(0.0f);

    inputLeftMagnitude.fill(MINIMUM_DISPLAY_DB);
    inputRightMagnitude.fill(MINIMUM_DISPLAY_DB);
    outputLeftMagnitude.fill(MINIMUM_DISPLAY_DB);
    outputRightMagnitude.fill(MINIMUM_DISPLAY_DB);

    for (auto &frame : frameQueue)
    {
        frame.sampleRate = sampleRate;
        frame.sequenceNumber = 0;
        frame.inputSpectrum.fill(MINIMUM_DISPLAY_DB);
        frame.outputSpectrum.fill(MINIMUM_DISPLAY_DB);
    }

    writeIndex = 0;
    samplesWritten = 0;
    samplesSinceLastFrame = 0;
    nextSequenceNumber = 1;
}

void SpectrumAnalyzer::pushInputSample(float leftSample, float rightSample)
{
    inputLeftHistory[static_cast<size_t>(writeIndex)] = std::isfinite(leftSample) ? leftSample : 0.0f;
    inputRightHistory[static_cast<size_t>(writeIndex)] = std::isfinite(rightSample) ? rightSample : 0.0f;
}

void SpectrumAnalyzer::pushOutputSample(float leftSample, float rightSample)
{
    outputLeftHistory[static_cast<size_t>(writeIndex)] = std::isfinite(leftSample) ? leftSample : 0.0f;
    outputRightHistory[static_cast<size_t>(writeIndex)] = std::isfinite(rightSample) ? rightSample : 0.0f;

    ++samplesWritten;

    ++writeIndex;
    if (writeIndex >= FFT_SIZE)
        writeIndex = 0;

    bool frameIsReady = samplesWritten == FFT_SIZE;
    if (samplesWritten > FFT_SIZE)
    {
        ++samplesSinceLastFrame;
        frameIsReady = samplesSinceLastFrame >= HOP_SIZE;

        if (frameIsReady)
            samplesSinceLastFrame -= HOP_SIZE;
    }

    if (frameIsReady)
        publishFrame();
}

bool SpectrumAnalyzer::readLatestFrame(SpectrumFrame &destination)
{
    const int availableFrames = frameFifo.getNumReady();
    if (availableFrames <= 0)
        return false;

    int firstStartIndex = 0;
    int firstFrameCount = 0;
    int secondStartIndex = 0;
    int secondFrameCount = 0;
    frameFifo.prepareToRead(availableFrames,
                            firstStartIndex,
                            firstFrameCount,
                            secondStartIndex,
                            secondFrameCount);

    const int frameCount = firstFrameCount + secondFrameCount;
    if (frameCount <= 0)
        return false;

    for (int frameOffset = 0; frameOffset < firstFrameCount; ++frameOffset)
    {
        const int frameIndex = (firstStartIndex + frameOffset) % FRAME_QUEUE_SIZE;
        destination = frameQueue[static_cast<size_t>(frameIndex)];
    }

    for (int frameOffset = 0; frameOffset < secondFrameCount; ++frameOffset)
    {
        const int frameIndex = (secondStartIndex + frameOffset) % FRAME_QUEUE_SIZE;
        destination = frameQueue[static_cast<size_t>(frameIndex)];
    }

    frameFifo.finishedRead(frameCount);
    return true;
}

void SpectrumAnalyzer::calculateSpectrum(const HistoryBuffer &history,
                                         FFTBuffer &fftData,
                                         MagnitudeBuffer &magnitudes)
{
    for (int sampleOffset = 0; sampleOffset < FFT_SIZE; ++sampleOffset)
    {
        int historyIndex = writeIndex + sampleOffset;
        if (historyIndex >= FFT_SIZE)
            historyIndex -= FFT_SIZE;

        fftData[static_cast<size_t>(sampleOffset)] = history[static_cast<size_t>(historyIndex)];
    }

    std::fill(fftData.begin() + FFT_SIZE, fftData.end(), 0.0f);
    window.multiplyWithWindowingTable(fftData.data(), FFT_SIZE);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    for (int binIndex = 0; binIndex < SPECTRUM_BINS; ++binIndex)
    {
        const float binScale = binIndex == 0 || binIndex == FFT_SIZE / 2
                                 ? 2.0f / static_cast<float>(FFT_SIZE)
                                 : 2.0f / (static_cast<float>(FFT_SIZE) * WINDOW_GAIN);
        const float magnitude = std::max(0.0f, fftData[static_cast<size_t>(binIndex)]) * binScale;
        magnitudes[static_cast<size_t>(binIndex)] = magnitudeToDecibels(magnitude);
    }
}

void SpectrumAnalyzer::publishFrame()
{
    int firstStartIndex = 0;
    int firstFrameCount = 0;
    int secondStartIndex = 0;
    int secondFrameCount = 0;
    frameFifo.prepareToWrite(1,
                             firstStartIndex,
                             firstFrameCount,
                             secondStartIndex,
                             secondFrameCount);

    if (firstFrameCount + secondFrameCount <= 0)
        return;

    calculateSpectrum(inputLeftHistory, inputLeftFFT, inputLeftMagnitude);
    calculateSpectrum(inputRightHistory, inputRightFFT, inputRightMagnitude);
    calculateSpectrum(outputLeftHistory, outputLeftFFT, outputLeftMagnitude);
    calculateSpectrum(outputRightHistory, outputRightFFT, outputRightMagnitude);

    const int frameIndex = firstFrameCount > 0 ? firstStartIndex : secondStartIndex;
    auto &frame = frameQueue[static_cast<size_t>(frameIndex)];
    frame.sampleRate = sampleRate;
    frame.sequenceNumber = nextSequenceNumber++;

    for (int binIndex = 0; binIndex < SPECTRUM_BINS; ++binIndex)
    {
        const size_t index = static_cast<size_t>(binIndex);
        frame.inputSpectrum[index] = averageMagnitudeDecibels(inputLeftMagnitude[index],
                                                              inputRightMagnitude[index]);
        frame.outputSpectrum[index] = averageMagnitudeDecibels(outputLeftMagnitude[index],
                                                               outputRightMagnitude[index]);
    }

    frameFifo.finishedWrite(1);
}

float SpectrumAnalyzer::magnitudeToDecibels(float magnitude)
{
    if (! std::isfinite(magnitude) || magnitude <= 0.0f)
        return MINIMUM_DISPLAY_DB;

    const float decibels = 20.0f * std::log10(magnitude);
    if (! std::isfinite(decibels))
        return MINIMUM_DISPLAY_DB;

    return juce::jlimit(MINIMUM_DISPLAY_DB, 0.0f, decibels);
}

float SpectrumAnalyzer::decibelsToMagnitude(float decibels)
{
    if (! std::isfinite(decibels) || decibels <= MINIMUM_DISPLAY_DB)
        return 0.0f;

    const float magnitude = std::pow(10.0f, decibels / 20.0f);
    return std::isfinite(magnitude) ? magnitude : 0.0f;
}

float SpectrumAnalyzer::averageMagnitudeDecibels(float firstDecibels, float secondDecibels)
{
    const float averageMagnitude = 0.5f * (decibelsToMagnitude(firstDecibels)
                                           + decibelsToMagnitude(secondDecibels));
    return magnitudeToDecibels(averageMagnitude);
}
