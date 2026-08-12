#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <functional>
#include <memory>

#include "../DSP/FilterTypes.h"

/**
 * Owns the controls and APVTS attachments for one EQ band.
 *
 * The panel deliberately owns one band's complete UI state instead of making
 * the editor maintain parallel arrays for five separate bands. This keeps
 * filter restrictions, parameter IDs, and control layout auditable in one
 * place while preserving APVTS as the source of truth.
 */
class BandControlPanel : public juce::Component
{
public:
    BandControlPanel(juce::AudioProcessorValueTreeState &state,
                     int bandIndex,
                     BandPosition position,
                     const juce::String &displayName);
    ~BandControlPanel() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    void setSelected(bool shouldBeSelected);

    int getBandIndex() const;
    float getFrequency() const;
    float getGain() const;
    float getQ() const;
    FilterType getFilterType() const;
    bool isBandEnabled() const;

    std::function<void(int)> onParametersChanged;

private:
    static juce::String getFilterTypeName(FilterType type);
    static juce::String makeParameterID(int bandIndex, const juce::String &suffix);

    void configureControls();
    void configureFilterTypeChoices();
    void updateFilterDependentControls();
    void notifyParameterChange();
    float getRawParameterValue(const juce::String &suffix, float fallbackValue) const;

    juce::AudioProcessorValueTreeState &parameterState;
    const int bandIndex;
    const BandPosition bandPosition;
    const juce::String bandDisplayName;

    bool isSelected = false;

    juce::Label bandNameLabel;
    juce::ToggleButton enabledButton;
    juce::Label frequencyLabel;
    juce::Label gainLabel;
    juce::Label qLabel;
    juce::Label filterTypeLabel;
    juce::Slider frequencySlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;
    juce::ComboBox filterTypeBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandControlPanel)
};
