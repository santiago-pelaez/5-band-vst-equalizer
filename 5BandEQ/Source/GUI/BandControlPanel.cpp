#include "BandControlPanel.h"

#include <algorithm>

BandControlPanel::BandControlPanel(juce::AudioProcessorValueTreeState &state,
                                   int index,
                                   BandPosition position,
                                   const juce::String &displayName)
    : parameterState(state),
      bandIndex(index),
      bandPosition(position),
      bandDisplayName(displayName)
{
    configureControls();
    configureFilterTypeChoices();

    const juce::String frequencyID = makeParameterID(bandIndex, "_freq");
    const juce::String gainID = makeParameterID(bandIndex, "_gain");
    const juce::String qID = makeParameterID(bandIndex, "_q");
    const juce::String typeID = makeParameterID(bandIndex, "_type");
    const juce::String enabledID = makeParameterID(bandIndex, "_enabled");

    frequencyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        parameterState, frequencyID, frequencySlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        parameterState, gainID, gainSlider);
    qAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        parameterState, qID, qSlider);
    filterTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        parameterState, typeID, filterTypeBox);
    enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        parameterState, enabledID, enabledButton);

    frequencySlider.onValueChange = [this]
    {
        notifyParameterChange();
    };
    gainSlider.onValueChange = [this]
    {
        notifyParameterChange();
    };
    qSlider.onValueChange = [this]
    {
        notifyParameterChange();
    };
    filterTypeBox.onChange = [this]
    {
        updateFilterDependentControls();
        notifyParameterChange();
    };
    enabledButton.onClick = [this]
    {
        notifyParameterChange();
    };

    addAndMakeVisible(bandNameLabel);
    addAndMakeVisible(enabledButton);
    addAndMakeVisible(frequencyLabel);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(qLabel);
    addAndMakeVisible(filterTypeLabel);
    addAndMakeVisible(frequencySlider);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(qSlider);
    addAndMakeVisible(filterTypeBox);

    updateFilterDependentControls();
    repaint();
}

BandControlPanel::~BandControlPanel() = default;

void BandControlPanel::paint(juce::Graphics &graphics)
{
    const auto panelBounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto fillColour = isSelected
                                ? juce::Colour::fromRGB(65, 76, 98)
                                : juce::Colour::fromRGB(48, 54, 68);
    const auto borderColour = isSelected
                                  ? juce::Colour::fromRGB(120, 205, 240)
                                  : juce::Colour::fromRGB(95, 105, 125);

    graphics.setColour(fillColour);
    graphics.fillRoundedRectangle(panelBounds, 6.0f);
    graphics.setColour(borderColour);
    graphics.drawRoundedRectangle(panelBounds, 6.0f, isSelected ? 2.0f : 1.0f);
}

void BandControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    auto headerArea = bounds.removeFromTop(26);
    bandNameLabel.setBounds(headerArea.removeFromLeft(headerArea.getWidth() - 78));
    enabledButton.setBounds(headerArea.removeFromRight(76));

    auto topControlArea = bounds.removeFromTop(106);
    auto frequencyArea = topControlArea.removeFromLeft(topControlArea.getWidth() / 2).reduced(3);
    auto qArea = topControlArea.reduced(3);

    frequencyLabel.setBounds(frequencyArea.removeFromTop(16));
    frequencySlider.setBounds(frequencyArea);
    qLabel.setBounds(qArea.removeFromTop(16));
    qSlider.setBounds(qArea);

    auto typeArea = bounds.removeFromTop(48).reduced(3, 0);
    filterTypeLabel.setBounds(typeArea.removeFromTop(16));
    filterTypeBox.setBounds(typeArea);

    gainLabel.setBounds(bounds.removeFromTop(16));
    gainSlider.setBounds(bounds.reduced(12, 0));
}

void BandControlPanel::setSelected(bool shouldBeSelected)
{
    if (isSelected == shouldBeSelected)
        return;

    isSelected = shouldBeSelected;
    repaint();
}

int BandControlPanel::getBandIndex() const
{
    return bandIndex;
}

float BandControlPanel::getFrequency() const
{
    return getRawParameterValue("_freq", 1000.0f);
}

float BandControlPanel::getGain() const
{
    return getRawParameterValue("_gain", 0.0f);
}

float BandControlPanel::getQ() const
{
    return getRawParameterValue("_q", 1.0f);
}

FilterType BandControlPanel::getFilterType() const
{
    const auto availableTypes = FilterTypeRestrictions::getAvailableTypes(bandPosition);
    const int selectedIndex = static_cast<int>(getRawParameterValue("_type", -1.0f));

    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(availableTypes.size()))
        return FilterType::Disabled;

    return availableTypes[static_cast<size_t>(selectedIndex)];
}

bool BandControlPanel::isBandEnabled() const
{
    return getRawParameterValue("_enabled", 0.0f) > 0.5f;
}

juce::String BandControlPanel::getFilterTypeName(FilterType type)
{
    switch (type)
    {
    case FilterType::Peak:
        return "Peak";
    case FilterType::LowCut:
        return "Low Cut";
    case FilterType::LowShelf:
        return "Low Shelf";
    case FilterType::HighCut:
        return "High Cut";
    case FilterType::HighShelf:
        return "High Shelf";
    case FilterType::Disabled:
        return "Disabled";
    }

    return "Disabled";
}

juce::String BandControlPanel::makeParameterID(int index, const juce::String &suffix)
{
    return "band" + juce::String(index + 1) + suffix;
}

void BandControlPanel::configureControls()
{
    bandNameLabel.setText(bandDisplayName, juce::dontSendNotification);
    bandNameLabel.setJustificationType(juce::Justification::centred);
    bandNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    bandNameLabel.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));

    enabledButton.setButtonText("On");
    enabledButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    enabledButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::lightgreen);

    const auto configureLabel = [](juce::Label &label, const juce::String &text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        label.setFont(juce::Font(juce::FontOptions(10.0f)));
    };

    configureLabel(frequencyLabel, "Freq");
    configureLabel(gainLabel, "Gain");
    configureLabel(qLabel, "Q");
    configureLabel(filterTypeLabel, "Type");

    frequencySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    frequencySlider.setRange(20.0, 20000.0, 1.0);
    frequencySlider.setSkewFactorFromMidPoint(1000.0);
    frequencySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    frequencySlider.setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
    frequencySlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);

    gainSlider.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider.setRange(-24.0, 24.0, 0.1);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    gainSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    gainSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);

    qSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qSlider.setRange(0.1, 10.0, 0.01);
    qSlider.setSkewFactorFromMidPoint(1.0);
    qSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    qSlider.setColour(juce::Slider::thumbColourId, juce::Colours::yellow);
    qSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::yellow);

    filterTypeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey);
    filterTypeBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    filterTypeBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::lightblue);
    filterTypeBox.setJustificationType(juce::Justification::centred);
}

void BandControlPanel::configureFilterTypeChoices()
{
    const auto availableTypes = FilterTypeRestrictions::getAvailableTypes(bandPosition);
    int itemID = 1;

    for (const auto type : availableTypes)
    {
        filterTypeBox.addItem(getFilterTypeName(type), itemID);
        ++itemID;
    }
}

void BandControlPanel::updateFilterDependentControls()
{
    const auto filterType = getFilterType();
    const bool gainIsMeaningful = filterType != FilterType::LowCut
                               && filterType != FilterType::HighCut;
    gainSlider.setEnabled(gainIsMeaningful);
    gainLabel.setEnabled(gainIsMeaningful);
}

float BandControlPanel::getRawParameterValue(const juce::String &suffix, float fallbackValue) const
{
    const auto *parameter = parameterState.getRawParameterValue(makeParameterID(bandIndex, suffix));

    if (parameter == nullptr)
        return fallbackValue;

    return parameter->load();
}

void BandControlPanel::notifyParameterChange()
{
    if (onParametersChanged)
        onParametersChanged(bandIndex);
}
