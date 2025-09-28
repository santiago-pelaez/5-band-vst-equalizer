#pragma once
#include <JuceHeader.h>
#include "DSP/EQBand.h"
#include "Utils/Constants.h"

/**
 * Parameter identifiers for automation
 */
namespace ParameterIDs
{
    // Band 1 (Low) - Restricted to cut/shelf
    static const juce::String band1Type = "band1_type";
    static const juce::String band1Freq = "band1_freq";
    static const juce::String band1Gain = "band1_gain";
    static const juce::String band1Q = "band1_q";
    static const juce::String band1Enabled = "band1_enabled";

    // Band 2 (Low-Mid) - Full parametric
    static const juce::String band2Type = "band2_type";
    static const juce::String band2Freq = "band2_freq";
    static const juce::String band2Gain = "band2_gain";
    static const juce::String band2Q = "band2_q";
    static const juce::String band2Enabled = "band2_enabled";

    // Band 3 (Mid) - Full parametric
    static const juce::String band3Type = "band3_type";
    static const juce::String band3Freq = "band3_freq";
    static const juce::String band3Gain = "band3_gain";
    static const juce::String band3Q = "band3_q";
    static const juce::String band3Enabled = "band3_enabled";

    // Band 4 (High-Mid) - Full parametric
    static const juce::String band4Type = "band4_type";
    static const juce::String band4Freq = "band4_freq";
    static const juce::String band4Gain = "band4_gain";
    static const juce::String band4Q = "band4_q";
    static const juce::String band4Enabled = "band4_enabled";

    // Band 5 (High) - Restricted to cut/shelf
    static const juce::String band5Type = "band5_type";
    static const juce::String band5Freq = "band5_freq";
    static const juce::String band5Gain = "band5_gain";
    static const juce::String band5Q = "band5_q";
    static const juce::String band5Enabled = "band5_enabled";

    // Global parameters
    static const juce::String outputGain = "output_gain";
    static const juce::String bypass = "bypass";
}

/**
 * Creates parameter layout with proper ranges and restrictions
 */
class ParameterHelper
{
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // Helper lambda to create band parameters
        auto createBandParams = [&](int bandNum, BandPosition position)
        {
            juce::String prefix = "band" + juce::String(bandNum + 1);

            // Filter type parameter (restricted based on band position)
            juce::StringArray filterChoices;
            auto availableTypes = FilterTypeRestrictions::getAvailableTypes(position);

            for (auto type : availableTypes)
            {
                switch (type)
                {
                case FilterType::Peak:
                    filterChoices.add("Peak");
                    break;
                case FilterType::LowCut:
                    filterChoices.add("Low Cut");
                    break;
                case FilterType::LowShelf:
                    filterChoices.add("Low Shelf");
                    break;
                case FilterType::HighCut:
                    filterChoices.add("High Cut");
                    break;
                case FilterType::HighShelf:
                    filterChoices.add("High Shelf");
                    break;
                case FilterType::Disabled:
                    filterChoices.add("Disabled");
                    break;
                }
            }

            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                prefix + "_type", prefix + " Type", filterChoices,
                position == BandPosition::Low ? 1 : (position == BandPosition::High ? 1 : 0)));

            // Frequency parameter
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                prefix + "_freq", prefix + " Frequency",
                juce::NormalisableRange<float>(EQConstants::FREQUENCY_MIN, EQConstants::FREQUENCY_MAX, 1.0f, 0.25f),
                EQConstants::BAND_FREQUENCIES[bandNum], "Hz"));

            // Gain parameter
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                prefix + "_gain", prefix + " Gain",
                juce::NormalisableRange<float>(EQConstants::GAIN_MIN, EQConstants::GAIN_MAX, 0.1f),
                0.0f, "dB"));

            // Q parameter
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                prefix + "_q", prefix + " Q",
                juce::NormalisableRange<float>(EQConstants::Q_MIN, EQConstants::Q_MAX, 0.01f, 0.3f),
                1.0f));

            // Enabled parameter
            params.push_back(std::make_unique<juce::AudioParameterBool>(
                prefix + "_enabled", prefix + " Enabled", true));
        };

        // Create parameters for all 5 bands
        createBandParams(0, BandPosition::Low);     // Band 1: Low cut/shelf only
        createBandParams(1, BandPosition::LowMid);  // Band 2: Full parametric
        createBandParams(2, BandPosition::Mid);     // Band 3: Full parametric
        createBandParams(3, BandPosition::HighMid); // Band 4: Full parametric
        createBandParams(4, BandPosition::High);    // Band 5: High cut/shelf only

        // Global parameters
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "output_gain", "Output Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "bypass", "Bypass", false));

        return {params.begin(), params.end()};
    }
};