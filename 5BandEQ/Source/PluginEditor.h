#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Main GUI editor for the 5-band EQ with restricted filter types
 */
class FiveBandEQProcessorEditor : public juce::AudioProcessorEditor
{
public:
    FiveBandEQProcessorEditor(FiveBandEQProcessor &);
    ~FiveBandEQProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

private:
    FiveBandEQProcessor &audioProcessor;

    // GUI Components
    juce::Slider gainSliders[5];
    juce::Slider freqSliders[5];
    juce::Slider qSliders[5];
    juce::Label bandLabels[5];
    juce::Label paramLabels[15]; // 3 labels per band (Gain, Freq, Q)
    juce::ToggleButton bypassButton;

    // Parameter attachments for automatic binding
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> gainAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> freqAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> qAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FiveBandEQProcessorEditor)
};