#include "PluginEditor.h"

//==============================================================================
FiveBandEQProcessorEditor::FiveBandEQProcessorEditor(FiveBandEQProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set editor size - increased height for freq/Q controls
    setSize(800, 400);

    // Setup band controls - each band gets gain, frequency, and Q controls
    const juce::String bandNames[5] = {"Low", "Low-Mid", "Mid", "High-Mid", "High"};
    const juce::String paramNames[3] = {"Gain", "Freq", "Q"};

    for (int i = 0; i < 5; ++i)
    {
        // Setup band labels
        bandLabels[i].setText(bandNames[i], juce::dontSendNotification);
        bandLabels[i].setJustificationType(juce::Justification::centred);
        bandLabels[i].setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(bandLabels[i]);

        // Setup parameter labels (Gain, Freq, Q for each band)
        for (int j = 0; j < 3; ++j)
        {
            int labelIndex = i * 3 + j;
            paramLabels[labelIndex].setText(paramNames[j], juce::dontSendNotification);
            paramLabels[labelIndex].setJustificationType(juce::Justification::centred);
            paramLabels[labelIndex].setColour(juce::Label::textColourId, juce::Colours::lightgrey);
            paramLabels[labelIndex].setFont(juce::Font(10.0f));
            addAndMakeVisible(paramLabels[labelIndex]);
        }

        // Setup gain sliders (vertical)
        gainSliders[i].setSliderStyle(juce::Slider::LinearVertical);
        gainSliders[i].setRange(-20.0, 20.0, 0.1);
        gainSliders[i].setValue(0.0);
        gainSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
        gainSliders[i].setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        gainSliders[i].setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
        addAndMakeVisible(gainSliders[i]);

        // Setup frequency sliders (rotary)
        freqSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        freqSliders[i].setRange(20.0, 20000.0, 1.0);
        freqSliders[i].setSkewFactorFromMidPoint(1000.0); // Logarithmic scaling
        freqSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        freqSliders[i].setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
        freqSliders[i].setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
        addAndMakeVisible(freqSliders[i]);

        // Setup Q sliders (rotary)
        qSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        qSliders[i].setRange(0.1, 20.0, 0.01);
        qSliders[i].setSkewFactorFromMidPoint(1.0);
        qSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
        qSliders[i].setColour(juce::Slider::thumbColourId, juce::Colours::yellow);
        qSliders[i].setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::yellow);
        addAndMakeVisible(qSliders[i]);

        // Create parameter attachments
        juce::String bandPrefix = "band" + juce::String(i + 1);
        gainAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), bandPrefix + "_gain", gainSliders[i]);
        freqAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), bandPrefix + "_freq", freqSliders[i]);
        qAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), bandPrefix + "_q", qSliders[i]);
    }

    // Setup bypass button (toggle style)
    bypassButton.setButtonText("Bypass");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkred);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    addAndMakeVisible(bypassButton);

    // Create bypass parameter attachment
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getAPVTS(), "bypass", bypassButton);
}

FiveBandEQProcessorEditor::~FiveBandEQProcessorEditor()
{
}

//==============================================================================
void FiveBandEQProcessorEditor::paint(juce::Graphics &g)
{
    // Modern gradient background
    juce::ColourGradient gradient(juce::Colour::fromRGB(45, 48, 62), 0, 0,
                                  juce::Colour::fromRGB(30, 32, 42), 0, getHeight(),
                                  false);
    g.setGradientFill(gradient);
    g.fillAll();

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font("Arial", 28.0f, juce::Font::bold));
    g.drawFittedText("5-Band Parametric EQ", getLocalBounds().removeFromTop(60),
                     juce::Justification::centred, 1);

    // Draw subtle separators between bands
    g.setColour(juce::Colour::fromRGB(70, 75, 90));
    auto bandArea = getLocalBounds().removeFromTop(60).reduced(10, 0);
    bandArea.removeFromTop(70); // Skip title

    for (int i = 1; i < 5; ++i)
    {
        int x = bandArea.getX() + (i * bandArea.getWidth() / 5);
        g.drawVerticalLine(x, bandArea.getY(), bandArea.getBottom());
    }
}

void FiveBandEQProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Reserve space for title
    auto titleArea = bounds.removeFromTop(60);

    // Reserve space for bypass button at bottom
    auto bottomArea = bounds.removeFromBottom(50);
    bypassButton.setBounds(bottomArea.removeFromRight(100).reduced(10));

    // Divide remaining space for 5 bands
    auto bandArea = bounds.reduced(10);
    int bandWidth = bandArea.getWidth() / 5;

    for (int i = 0; i < 5; ++i)
    {
        auto currentBandArea = bandArea.removeFromLeft(bandWidth).reduced(5);

        // Band label at top
        bandLabels[i].setBounds(currentBandArea.removeFromTop(25));

        // Create areas for the three controls per band
        auto controlArea = currentBandArea;

        // Top row: Frequency and Q controls (rotary knobs)
        auto topRow = controlArea.removeFromTop(120);
        auto freqArea = topRow.removeFromLeft(topRow.getWidth() / 2).reduced(5);
        auto qArea = topRow.reduced(5);

        // Labels for freq and Q
        int freqLabelIndex = i * 3 + 1; // Freq is second param
        int qLabelIndex = i * 3 + 2;    // Q is third param

        paramLabels[freqLabelIndex].setBounds(freqArea.removeFromTop(15));
        freqSliders[i].setBounds(freqArea.removeFromTop(80));

        paramLabels[qLabelIndex].setBounds(qArea.removeFromTop(15));
        qSliders[i].setBounds(qArea.removeFromTop(80));

        // Bottom: Gain slider (vertical)
        auto gainArea = controlArea.reduced(15, 5);
        int gainLabelIndex = i * 3; // Gain is first param

        paramLabels[gainLabelIndex].setBounds(gainArea.removeFromTop(15));
        gainSliders[i].setBounds(gainArea);
    }
}