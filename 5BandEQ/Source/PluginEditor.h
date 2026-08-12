#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "GUI/BandControlPanel.h"
#include "GUI/FrequencyResponseDisplay.h"

/**
 * Main GUI editor for the 5-band EQ with restricted filter types and frequency response visualization
 */
class FiveBandEQProcessorEditor : public juce::AudioProcessorEditor
{
public:
    FiveBandEQProcessorEditor(FiveBandEQProcessor &);
    ~FiveBandEQProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    // Update frequency response visualization
    void updateFrequencyResponse();

private:
    void selectBand(int bandIndex);
    void handleNodeEdit(const FrequencyResponseDisplay::NodeEditEvent &event);
    juce::RangedAudioParameter *getBandParameter(int bandIndex, const juce::String &suffix);
    void setBandParameterValue(int bandIndex, const juce::String &suffix, float value);
    void beginBandParameterGesture(int bandIndex, const juce::String &suffix);
    void endBandParameterGesture(int bandIndex, const juce::String &suffix);

    FiveBandEQProcessor &audioProcessor;

    // GUI components. Each band owns its controls and APVTS attachments.
    std::array<std::unique_ptr<BandControlPanel>, EQConstants::NUM_BANDS> bandControlPanels;
    FrequencyResponseDisplay frequencyResponseDisplay;
    juce::Label outputGainLabel;
    juce::Slider outputGainSlider;
    juce::ToggleButton bypassButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FiveBandEQProcessorEditor)
};
