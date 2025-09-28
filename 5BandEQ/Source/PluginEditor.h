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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FiveBandEQProcessorEditor)
};