#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include "DSP/EQBand.h"
#include "Utils/Parameters.h"
#include "Utils/Constants.h"

/**
 * Main plugin processor handling 5-band EQ with restricted filter types
 */
class FiveBandEQProcessor : public juce::AudioProcessor,
                            public juce::AudioProcessorValueTreeState::Listener
{
public:
    FiveBandEQProcessor();
    ~FiveBandEQProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    //==============================================================================
    // AudioProcessorValueTreeState::Listener interface
    void parameterChanged(const juce::String &parameterID, float newValue) override;

    //==============================================================================
    // Parameter access for editor
    juce::AudioProcessorValueTreeState &getAPVTS() { return apvts; }

    // Get frequency response for spectrum display
    void getFrequencyResponse(std::vector<double> &frequencies,
                              std::vector<double> &magnitudes,
                              int numPoints = 512);

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    // EQ bands with position-based restrictions
    std::array<std::unique_ptr<EQBand>, EQConstants::NUM_BANDS> eqBands;

    // Output gain and bypass
    juce::SmoothedValue<float> outputGain;
    juce::SmoothedValue<float> bypassMix;
    std::atomic<bool> parametersNeedSynchronization{true};

    // Helper methods
    void updateBandFromParameters(int bandIndex);
    void synchronizeParametersFromHost();

    // Convert parameter choice index to FilterType
    FilterType getFilterTypeFromChoice(int choiceIndex, BandPosition position);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FiveBandEQProcessor)
};
