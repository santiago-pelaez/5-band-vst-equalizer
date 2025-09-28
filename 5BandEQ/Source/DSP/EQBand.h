#pragma once
#include <JuceHeader.h>
#include "BiquadFilter.h"
#include "FilterTypes.h"
#include "../Utils/Constants.h"

/**
 * Individual EQ band with parameter smoothing and band restrictions
 */
class EQBand
{
public:
    EQBand(BandPosition position) : bandPosition(position)
    {
        // Set default filter type based on band position
        switch (position)
        {
        case BandPosition::Low:
            currentType = FilterType::LowShelf;
            break;
        case BandPosition::High:
            currentType = FilterType::HighShelf;
            break;
        default:
            currentType = FilterType::Peak;
            break;
        }

        // Initialize parameter smoothers
        frequencySmooth.setCurrentAndTargetValue(EQConstants::BAND_FREQUENCIES[static_cast<int>(position)]);
        gainSmooth.setCurrentAndTargetValue(0.0f);
        qSmooth.setCurrentAndTargetValue(1.0f);
    }

    void setSampleRate(double newSampleRate)
    {
        this->sampleRate = newSampleRate;
        leftFilter.setSampleRate(newSampleRate);
        rightFilter.setSampleRate(newSampleRate);

        // Set smoothing time (10ms for parameter changes)
        double smoothingTime = 0.01;
        frequencySmooth.reset(sampleRate, smoothingTime);
        gainSmooth.reset(sampleRate, smoothingTime);
        qSmooth.reset(sampleRate, smoothingTime);
    }

    void setFilterType(FilterType type)
    {
        if (FilterTypeRestrictions::isValidTypeForBand(type, bandPosition))
        {
            currentType = type;
            updateFilters();
        }
    }

    void setFrequency(float frequency)
    {
        frequencySmooth.setTargetValue(juce::jlimit(
            static_cast<float>(EQConstants::FREQUENCY_MIN),
            static_cast<float>(EQConstants::FREQUENCY_MAX),
            frequency));
    }

    void setGain(float gain)
    {
        gainSmooth.setTargetValue(juce::jlimit(
            static_cast<float>(EQConstants::GAIN_MIN),
            static_cast<float>(EQConstants::GAIN_MAX),
            gain));
    }

    void setQ(float q)
    {
        qSmooth.setTargetValue(juce::jlimit(
            static_cast<float>(EQConstants::Q_MIN),
            static_cast<float>(EQConstants::Q_MAX),
            q));
    }

    void setEnabled(bool isEnabled)
    {
        this->enabled = isEnabled;
    }

    void processStereo(float &left, float &right)
    {
        if (!enabled || currentType == FilterType::Disabled)
            return;

        // Update smoothed parameters
        bool paramsChanged = false;
        if (frequencySmooth.isSmoothing() || gainSmooth.isSmoothing() || qSmooth.isSmoothing())
        {
            frequencySmooth.getNextValue();
            gainSmooth.getNextValue();
            qSmooth.getNextValue();
            paramsChanged = true;
        }

        if (paramsChanged)
            updateFilters();

        left = leftFilter.processSample(left);
        right = rightFilter.processSample(right);
    }

    // Getters
    FilterType getFilterType() const { return currentType; }
    float getFrequency() const { return frequencySmooth.getCurrentValue(); }
    float getGain() const { return gainSmooth.getCurrentValue(); }
    float getQ() const { return qSmooth.getCurrentValue(); }
    bool isEnabled() const { return enabled; }
    BandPosition getBandPosition() const { return bandPosition; }

    std::vector<FilterType> getAvailableTypes() const
    {
        return FilterTypeRestrictions::getAvailableTypes(bandPosition);
    }

private:
    BandPosition bandPosition;
    FilterType currentType = FilterType::Peak;
    bool enabled = true;
    double sampleRate = 44100.0;

    BiquadFilter leftFilter, rightFilter;

    // Parameter smoothing
    juce::SmoothedValue<float> frequencySmooth;
    juce::SmoothedValue<float> gainSmooth;
    juce::SmoothedValue<float> qSmooth;

    void updateFilters()
    {
        leftFilter.setCoefficients(currentType, frequencySmooth.getCurrentValue(),
                                   gainSmooth.getCurrentValue(), qSmooth.getCurrentValue());
        rightFilter.setCoefficients(currentType, frequencySmooth.getCurrentValue(),
                                    gainSmooth.getCurrentValue(), qSmooth.getCurrentValue());
    }
};