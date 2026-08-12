#include "PluginEditor.h"

#include <array>

namespace
{
    constexpr int DEFAULT_EDITOR_WIDTH = 1000;
    constexpr int DEFAULT_EDITOR_HEIGHT = 700;
    constexpr int MINIMUM_EDITOR_WIDTH = 900;
    constexpr int MINIMUM_EDITOR_HEIGHT = 620;
    constexpr int TITLE_BAR_HEIGHT = 72;
    constexpr int RESPONSE_DISPLAY_HEIGHT = 210;
}

FiveBandEQProcessorEditor::FiveBandEQProcessorEditor(FiveBandEQProcessor &processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setSize(DEFAULT_EDITOR_WIDTH, DEFAULT_EDITOR_HEIGHT);
    setResizable(true, true);
    setResizeLimits(MINIMUM_EDITOR_WIDTH, MINIMUM_EDITOR_HEIGHT, 1600, 1100);

    const std::array<BandPosition, EQConstants::NUM_BANDS> bandPositions = {
        BandPosition::Low,
        BandPosition::LowMid,
        BandPosition::Mid,
        BandPosition::HighMid,
        BandPosition::High};
    const std::array<juce::String, EQConstants::NUM_BANDS> bandNames = {
        "Low",
        "Low-Mid",
        "Mid",
        "High-Mid",
        "High"};

    for (int bandIndex = 0; bandIndex < EQConstants::NUM_BANDS; ++bandIndex)
    {
        bandControlPanels[bandIndex] = std::make_unique<BandControlPanel>(
            audioProcessor.getAPVTS(),
            bandIndex,
            bandPositions[bandIndex],
            bandNames[bandIndex]);

        bandControlPanels[bandIndex]->onParametersChanged = [this](int changedBandIndex)
        {
            juce::ignoreUnused(changedBandIndex);
            updateFrequencyResponse();
        };

        addAndMakeVisible(*bandControlPanels[bandIndex]);
    }

    frequencyResponseDisplay.onBandSelected = [this](int selectedBandIndex)
    {
        selectBand(selectedBandIndex);
    };
    frequencyResponseDisplay.onNodeEdit = [this](const FrequencyResponseDisplay::NodeEditEvent &event)
    {
        handleNodeEdit(event);
    };

    outputGainLabel.setText("Output Gain", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centredRight);
    outputGainLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(outputGainLabel);

    outputGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    outputGainSlider.setRange(-24.0, 24.0, 0.1);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 22);
    outputGainSlider.setTextValueSuffix(" dB");
    outputGainSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    outputGainSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    outputGainSlider.setColour(juce::Slider::backgroundColourId, juce::Colour::fromRGB(30, 32, 42));
    addAndMakeVisible(outputGainSlider);

    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "output_gain", outputGainSlider);

    bypassButton.setButtonText("Bypass");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkred);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    addAndMakeVisible(bypassButton);

    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "bypass", bypassButton);

    addAndMakeVisible(frequencyResponseDisplay);
    frequencyResponseDisplay.setSampleRate(audioProcessor.getSampleRate());
    updateFrequencyResponse();

    // setSize() may trigger resized() before child components exist. Perform
    // one explicit final layout after every panel and global control has been
    // created so the initial editor matches the layout used after resizing.
    resized();
}

FiveBandEQProcessorEditor::~FiveBandEQProcessorEditor() = default;

void FiveBandEQProcessorEditor::paint(juce::Graphics &graphics)
{
    const juce::ColourGradient backgroundGradient(
        juce::Colour::fromRGB(45, 48, 62),
        0.0f,
        0.0f,
        juce::Colour::fromRGB(30, 32, 42),
        0.0f,
        static_cast<float>(getHeight()),
        false);

    graphics.setGradientFill(backgroundGradient);
    graphics.fillAll();

    graphics.setColour(juce::Colour::fromRGB(25, 28, 35));
    graphics.fillRect(getLocalBounds().removeFromTop(TITLE_BAR_HEIGHT));

    graphics.setColour(juce::Colours::white);
    graphics.setFont(juce::Font(juce::FontOptions("Arial", 26.0f, juce::Font::bold)));
    graphics.drawFittedText("5-Band Parametric EQ",
                            juce::Rectangle<int>(18, 0, 350, TITLE_BAR_HEIGHT),
                            juce::Justification::centredLeft,
                            1);

    graphics.setColour(juce::Colour::fromRGB(145, 155, 175));
    graphics.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    graphics.drawText("BAND CONTROLS",
                      18,
                      TITLE_BAR_HEIGHT + RESPONSE_DISPLAY_HEIGHT + 2,
                      160,
                      20,
                      juce::Justification::left);
}

void FiveBandEQProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto headerArea = bounds.removeFromTop(TITLE_BAR_HEIGHT).reduced(18, 14);
    auto bypassArea = headerArea.removeFromRight(100);
    bypassButton.setBounds(bypassArea);

    auto outputGainArea = headerArea.removeFromRight(310);
    outputGainLabel.setBounds(outputGainArea.removeFromLeft(92));
    outputGainSlider.setBounds(outputGainArea.reduced(6, 2));

    const auto responseArea = bounds.removeFromTop(RESPONSE_DISPLAY_HEIGHT);
    frequencyResponseDisplay.setBounds(responseArea.reduced(18, 10));

    bounds.removeFromTop(28);
    auto bandArea = bounds.reduced(12, 6);
    const int panelWidth = bandArea.getWidth() / EQConstants::NUM_BANDS;

    for (auto &bandPanel : bandControlPanels)
    {
        if (bandPanel == nullptr)
            continue;

        bandPanel->setBounds(bandArea.removeFromLeft(panelWidth).reduced(5));
    }
}

void FiveBandEQProcessorEditor::updateFrequencyResponse()
{
    for (const auto &bandPanel : bandControlPanels)
    {
        if (bandPanel == nullptr)
            continue;

        frequencyResponseDisplay.setBandParameters(
            bandPanel->getBandIndex(),
            bandPanel->getFrequency(),
            bandPanel->getGain(),
            bandPanel->getQ(),
            static_cast<int>(bandPanel->getFilterType()),
            bandPanel->isBandEnabled());
    }
}

void FiveBandEQProcessorEditor::selectBand(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= EQConstants::NUM_BANDS)
        return;

    for (int currentBandIndex = 0; currentBandIndex < EQConstants::NUM_BANDS; ++currentBandIndex)
    {
        if (bandControlPanels[currentBandIndex] == nullptr)
            continue;

        bandControlPanels[currentBandIndex]->setSelected(currentBandIndex == bandIndex);
    }

    frequencyResponseDisplay.setSelectedBand(bandIndex);
}

void FiveBandEQProcessorEditor::handleNodeEdit(const FrequencyResponseDisplay::NodeEditEvent &event)
{
    if (event.bandIndex < 0 || event.bandIndex >= EQConstants::NUM_BANDS)
        return;

    selectBand(event.bandIndex);

    if (event.phase == FrequencyResponseDisplay::NodeEditPhase::Begin)
    {
        beginBandParameterGesture(event.bandIndex, "_freq");

        if (event.gainIsEditable)
            beginBandParameterGesture(event.bandIndex, "_gain");
    }

    setBandParameterValue(event.bandIndex, "_freq", event.frequency);

    if (event.gainIsEditable)
        setBandParameterValue(event.bandIndex, "_gain", event.gain);

    if (event.phase == FrequencyResponseDisplay::NodeEditPhase::End)
    {
        endBandParameterGesture(event.bandIndex, "_freq");

        if (event.gainIsEditable)
            endBandParameterGesture(event.bandIndex, "_gain");
    }

    updateFrequencyResponse();
}

juce::RangedAudioParameter *FiveBandEQProcessorEditor::getBandParameter(
    int bandIndex,
    const juce::String &suffix)
{
    if (bandIndex < 0 || bandIndex >= EQConstants::NUM_BANDS)
        return nullptr;

    const juce::String parameterID = "band" + juce::String(bandIndex + 1) + suffix;
    return audioProcessor.getAPVTS().getParameter(parameterID);
}

void FiveBandEQProcessorEditor::setBandParameterValue(int bandIndex,
                                                       const juce::String &suffix,
                                                       float value)
{
    auto *parameter = getBandParameter(bandIndex, suffix);

    if (parameter == nullptr)
        return;

    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void FiveBandEQProcessorEditor::beginBandParameterGesture(int bandIndex,
                                                           const juce::String &suffix)
{
    auto *parameter = getBandParameter(bandIndex, suffix);

    if (parameter == nullptr)
        return;

    parameter->beginChangeGesture();
}

void FiveBandEQProcessorEditor::endBandParameterGesture(int bandIndex,
                                                         const juce::String &suffix)
{
    auto *parameter = getBandParameter(bandIndex, suffix);

    if (parameter == nullptr)
        return;

    parameter->endChangeGesture();
}
