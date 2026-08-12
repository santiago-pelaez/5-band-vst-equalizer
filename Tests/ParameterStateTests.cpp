#include "Utils/Parameters.h"

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

    class TestAudioProcessor final : public juce::AudioProcessor
    {
    public:
        TestAudioProcessor()
            : juce::AudioProcessor(BusesProperties()
                                       .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
              parameters(*this, nullptr, "Parameters", ParameterHelper::createParameterLayout())
        {
        }

        const juce::String getName() const override
        {
            return "Parameter Test Processor";
        }

        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {}

        bool hasEditor() const override
        {
            return false;
        }

        juce::AudioProcessorEditor *createEditor() override
        {
            return nullptr;
        }

        double getTailLengthSeconds() const override
        {
            return 0.0;
        }

        bool acceptsMidi() const override
        {
            return false;
        }

        bool producesMidi() const override
        {
            return false;
        }

        int getNumPrograms() override
        {
            return 1;
        }

        int getCurrentProgram() override
        {
            return 0;
        }

        void setCurrentProgram(int) override {}

        const juce::String getProgramName(int) override
        {
            return {};
        }

        void changeProgramName(int, const juce::String &) override {}
        void getStateInformation(juce::MemoryBlock &) override {}
        void setStateInformation(const void *, int) override {}

        juce::AudioProcessorValueTreeState parameters;
    };

    float readParameter(TestAudioProcessor &processor, const juce::String &parameterID)
    {
        const auto *value = processor.parameters.getRawParameterValue(parameterID);
        requireCondition(value != nullptr, "missing parameter: " + parameterID.toStdString());
        return value->load();
    }

    void setParameterValue(TestAudioProcessor &processor,
                           const juce::String &parameterID,
                           float value)
    {
        auto *parameter = processor.parameters.getParameter(parameterID);
        requireCondition(parameter != nullptr, "missing writable parameter: " + parameterID.toStdString());
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }

    void testDefaultBandStates()
    {
        TestAudioProcessor processor;
        const int expectedDefaultTypes[EQConstants::NUM_BANDS] = {1, 0, 0, 0, 1};

        for (int bandIndex = 0; bandIndex < EQConstants::NUM_BANDS; ++bandIndex)
        {
            const juce::String prefix = "band" + juce::String(bandIndex + 1);
            requireCondition(static_cast<int>(readParameter(processor, prefix + "_type"))
                                 == expectedDefaultTypes[bandIndex],
                             "default filter type is incorrect for " + prefix.toStdString());
            requireCondition(readParameter(processor, prefix + "_enabled") > 0.5f,
                             "default enabled state is false for " + prefix.toStdString());
        }
    }

    void testNodeParameterConversionPath()
    {
        TestAudioProcessor processor;

        for (int bandIndex = 0; bandIndex < EQConstants::NUM_BANDS; ++bandIndex)
        {
            const juce::String prefix = "band" + juce::String(bandIndex + 1);
            const float expectedFrequency = 100.0f + static_cast<float>(bandIndex) * 700.0f;
            const float expectedGain = -8.0f + static_cast<float>(bandIndex) * 3.0f;

            setParameterValue(processor, prefix + "_freq", expectedFrequency);
            setParameterValue(processor, prefix + "_gain", expectedGain);

            requireCondition(std::abs(readParameter(processor, prefix + "_freq") - expectedFrequency) < 0.01f,
                             "frequency conversion failed for " + prefix.toStdString());
            requireCondition(std::abs(readParameter(processor, prefix + "_gain") - expectedGain) < 0.01f,
                             "gain conversion failed for " + prefix.toStdString());
        }
    }

    void testStateRecall()
    {
        TestAudioProcessor sourceProcessor;
        setParameterValue(sourceProcessor, "band1_freq", 55.0f);
        setParameterValue(sourceProcessor, "band3_gain", 9.5f);
        setParameterValue(sourceProcessor, "band4_q", 2.25f);
        setParameterValue(sourceProcessor, "band5_enabled", 0.0f);
        setParameterValue(sourceProcessor, "output_gain", -3.5f);
        setParameterValue(sourceProcessor, "bypass", 1.0f);

        const juce::ValueTree savedState = sourceProcessor.parameters.copyState();

        TestAudioProcessor restoredProcessor;
        restoredProcessor.parameters.replaceState(savedState.createCopy());

        requireCondition(std::abs(readParameter(restoredProcessor, "band1_freq") - 55.0f) < 0.01f,
                         "band 1 frequency was not restored");
        requireCondition(std::abs(readParameter(restoredProcessor, "band3_gain") - 9.5f) < 0.01f,
                         "band 3 gain was not restored");
        requireCondition(std::abs(readParameter(restoredProcessor, "band4_q") - 2.25f) < 0.01f,
                         "band 4 Q was not restored");
        requireCondition(readParameter(restoredProcessor, "band5_enabled") < 0.5f,
                         "band 5 enabled state was not restored");
        requireCondition(std::abs(readParameter(restoredProcessor, "output_gain") + 3.5f) < 0.01f,
                         "output gain was not restored");
        requireCondition(readParameter(restoredProcessor, "bypass") > 0.5f,
                         "bypass state was not restored");
    }
}

int main()
{
    testDefaultBandStates();
    testNodeParameterConversionPath();
    testStateRecall();

    std::cout << "All parameter and state tests passed.\n";
    return EXIT_SUCCESS;
}
