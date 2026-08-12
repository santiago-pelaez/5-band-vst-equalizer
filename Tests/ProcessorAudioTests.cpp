#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
    constexpr double TEST_SAMPLE_RATE = 48000.0;
    constexpr int TEST_BLOCK_SIZE = 256;

    void requireCondition(bool condition, const std::string &failureMessage)
    {
        if (condition)
            return;

        std::cerr << "Test failure: " << failureMessage << '\n';
        std::exit(EXIT_FAILURE);
    }

    void requireNear(float actual, float expected, float tolerance, const std::string &failureMessage)
    {
        requireCondition(std::abs(actual - expected) <= tolerance,
                         failureMessage + " (actual=" + std::to_string(actual)
                             + ", expected=" + std::to_string(expected) + ")");
    }

    void setParameter(FiveBandEQProcessor &processor,
                      const juce::String &parameterID,
                      float value)
    {
        auto *parameter = processor.getAPVTS().getParameter(parameterID);
        requireCondition(parameter != nullptr,
                         "missing processor parameter: " + parameterID.toStdString());
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }

    void fillStereoBuffer(juce::AudioBuffer<float> &buffer, float leftValue, float rightValue)
    {
        buffer.clear();
        buffer.applyGain(0.0f);

        for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            buffer.setSample(0, sampleIndex, leftValue);
            buffer.setSample(1, sampleIndex, rightValue);
        }
    }

    void processBlocks(FiveBandEQProcessor &processor,
                       juce::AudioBuffer<float> &buffer,
                       int blockCount)
    {
        juce::MidiBuffer midiMessages;

        for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex)
            processor.processBlock(buffer, midiMessages);
    }

    void testNeutralStereoProcessingAndAnalyzerObservation()
    {
        FiveBandEQProcessor processor;
        processor.prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);

        juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);
        fillStereoBuffer(buffer, 0.25f, -0.125f);
        processBlocks(processor, buffer, SpectrumAnalyzer::FFT_SIZE / TEST_BLOCK_SIZE + 1);

        requireNear(buffer.getSample(0, TEST_BLOCK_SIZE - 1), 0.25f, 0.0005f,
                    "neutral left-channel processing changed the signal");
        requireNear(buffer.getSample(1, TEST_BLOCK_SIZE - 1), -0.125f, 0.0005f,
                    "neutral right-channel processing changed the signal");

        SpectrumAnalyzer::SpectrumFrame frame;
        requireCondition(processor.copyLatestSpectrumFrame(frame),
                         "processor did not publish an analyzer frame");
        requireCondition(frame.sequenceNumber > 0,
                         "processor analyzer frame did not have a sequence number");
        requireCondition(std::isfinite(frame.sampleRate),
                         "processor analyzer frame sample rate was not finite");
    }

    void testStereoChannelsDoNotCrossTalk()
    {
        FiveBandEQProcessor processor;
        processor.prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);

        juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);
        fillStereoBuffer(buffer, 0.5f, 0.0f);
        processBlocks(processor, buffer, 4);

        requireNear(buffer.getSample(1, TEST_BLOCK_SIZE - 1), 0.0f, 0.00001f,
                    "right channel received signal from the left channel");
    }

    void testOutputGainSmoothingReachesTarget()
    {
        FiveBandEQProcessor processor;
        processor.prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        setParameter(processor, "output_gain", 6.0f);

        juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);
        juce::MidiBuffer midiMessages;
        float firstOutput = 0.0f;
        float finalOutput = 0.0f;

        for (int blockIndex = 0; blockIndex < 24; ++blockIndex)
        {
            fillStereoBuffer(buffer, 0.1f, 0.1f);
            processor.processBlock(buffer, midiMessages);

            if (blockIndex == 0)
                firstOutput = buffer.getSample(0, TEST_BLOCK_SIZE - 1);

            finalOutput = buffer.getSample(0, TEST_BLOCK_SIZE - 1);
        }

        requireCondition(finalOutput > firstOutput + 0.02f,
                         "output gain did not visibly smooth toward its target (first="
                             + std::to_string(firstOutput) + ", final="
                             + std::to_string(finalOutput) + ")");
        requireNear(finalOutput, juce::Decibels::decibelsToGain(6.0f) * 0.1f, 0.01f,
                    "output gain did not reach its target");
    }

    void testBypassReturnsToDrySignal()
    {
        FiveBandEQProcessor processor;
        setParameter(processor, "band1_type", 1.0f);
        setParameter(processor, "band1_freq", 100.0f);
        setParameter(processor, "band1_gain", 18.0f);
        processor.prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);

        juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);
        juce::MidiBuffer midiMessages;
        float processedOutput = 0.0f;

        for (int blockIndex = 0; blockIndex < 24; ++blockIndex)
        {
            fillStereoBuffer(buffer, 0.1f, 0.1f);
            processor.processBlock(buffer, midiMessages);
            processedOutput = buffer.getSample(0, TEST_BLOCK_SIZE - 1);
        }

        setParameter(processor, "bypass", 1.0f);

        float bypassedOutput = 0.0f;
        for (int blockIndex = 0; blockIndex < 24; ++blockIndex)
        {
            fillStereoBuffer(buffer, 0.1f, 0.1f);
            processor.processBlock(buffer, midiMessages);
            bypassedOutput = buffer.getSample(0, TEST_BLOCK_SIZE - 1);
        }

        requireCondition(std::abs(processedOutput - 0.1f) > 0.01f,
                         "test EQ configuration did not produce a processed signal");
        requireNear(bypassedOutput, 0.1f, 0.01f,
                    "bypass did not return the output to the dry signal");
    }

    void testAnalyzerDoesNotAlterAudioBuffer()
    {
        FiveBandEQProcessor processor;
        processor.prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);

        juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);
        juce::MidiBuffer midiMessages;
        std::array<float, TEST_BLOCK_SIZE> expectedLeft{};
        std::array<float, TEST_BLOCK_SIZE> expectedRight{};

        for (int sampleIndex = 0; sampleIndex < TEST_BLOCK_SIZE; ++sampleIndex)
        {
            const float phase = static_cast<float>(2.0 * juce::MathConstants<double>::pi
                                                   * 440.0 * sampleIndex / TEST_SAMPLE_RATE);
            expectedLeft[static_cast<size_t>(sampleIndex)] = 0.2f * std::sin(phase);
            expectedRight[static_cast<size_t>(sampleIndex)] = -0.1f * std::sin(phase);
            buffer.setSample(0, sampleIndex, expectedLeft[static_cast<size_t>(sampleIndex)]);
            buffer.setSample(1, sampleIndex, expectedRight[static_cast<size_t>(sampleIndex)]);
        }

        processor.processBlock(buffer, midiMessages);

        for (int sampleIndex = 0; sampleIndex < TEST_BLOCK_SIZE; ++sampleIndex)
        {
            requireCondition(std::isfinite(buffer.getSample(0, sampleIndex)),
                             "left processor output was not finite");
            requireCondition(std::isfinite(buffer.getSample(1, sampleIndex)),
                             "right processor output was not finite");
        }

        SpectrumAnalyzer::SpectrumFrame frame;
        requireCondition(! processor.copyLatestSpectrumFrame(frame),
                         "analyzer published a frame before its history was full");
    }
}

int main()
{
    testNeutralStereoProcessingAndAnalyzerObservation();
    testStereoChannelsDoNotCrossTalk();
    testOutputGainSmoothingReachesTarget();
    testBypassReturnsToDrySignal();
    testAnalyzerDoesNotAlterAudioBuffer();

    std::cout << "All processor audio tests passed.\n";
    return EXIT_SUCCESS;
}
