#include "PluginEditor.h"

//==============================================================================
FiveBandEQProcessorEditor::FiveBandEQProcessorEditor (FiveBandEQProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set editor size
    setSize (800, 600);
}

FiveBandEQProcessorEditor::~FiveBandEQProcessorEditor()
{
}

//==============================================================================
void FiveBandEQProcessorEditor::paint (juce::Graphics& g)
{
    // Fill the whole window with a gradient
    g.fillAll (juce::Colour::fromRGB(40, 42, 54));
    
    // Draw title
    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText ("5-Band Parametric EQ", getLocalBounds().removeFromTop(50), 
                      juce::Justification::centred, 1);
    
    // Draw placeholder for band restrictions
    g.setFont (14.0f);
    g.setColour (juce::Colours::lightgrey);
    
    juce::Rectangle<int> area = getLocalBounds().reduced(20).removeFromTop(100);
    area.removeFromTop(50); // Skip title area
    
    g.drawText ("Band 1 (Low): Cut/Shelf Only", area.removeFromLeft(150), juce::Justification::left, false);
    g.drawText ("Bands 2-4 (Mid): Full Parametric", area.removeFromLeft(200), juce::Justification::left, false);
    g.drawText ("Band 5 (High): Cut/Shelf Only", area, juce::Justification::left, false);
}

void FiveBandEQProcessorEditor::resized()
{
    // Layout will be implemented with actual GUI components
}