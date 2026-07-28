#include "FrequencyResponseDisplay.h"
#include "../DSP/FilterTypes.h"

#include <algorithm>
#include <array>
#include <cmath>

FrequencyResponseDisplay::FrequencyResponseDisplay()
{
    calculateFrequencyPoints();
    responseData.fill(0.0f);

    for (int bandIndex = 0; bandIndex < NUM_BANDS; ++bandIndex)
    {
        bandParams[bandIndex].frequency = 100.0f * std::pow(10.0f, bandIndex * 0.6f);
        bandParams[bandIndex].gain = 0.0f;
        bandParams[bandIndex].q = 0.7f;
        bandParams[bandIndex].filterType = static_cast<int>(FilterType::Peak);
        bandParams[bandIndex].enabled = true;
    }
}

FrequencyResponseDisplay::~FrequencyResponseDisplay() = default;

void FrequencyResponseDisplay::setSampleRate(double newSampleRate)
{
    sampleRate = std::isfinite(newSampleRate) && newSampleRate > 1.0 ? newSampleRate : 44100.0;
    updateResponse();
}

void FrequencyResponseDisplay::paint(juce::Graphics &graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(25, 28, 35));
    drawGrid(graphics);
    drawFrequencyLabels(graphics);
    drawGainLabels(graphics);
    drawResponseCurve(graphics);
}

void FrequencyResponseDisplay::resized()
{
    updateResponse();
}

void FrequencyResponseDisplay::updateResponse()
{
    calculateResponseCurve();
    repaint();
}

void FrequencyResponseDisplay::setBandParameters(int bandIndex,
                                                 float frequency,
                                                 float gain,
                                                 float q,
                                                 int filterType,
                                                 bool enabled)
{
    if (bandIndex < 0 || bandIndex >= NUM_BANDS)
        return;

    bandParams[bandIndex].frequency = frequency;
    bandParams[bandIndex].gain = gain;
    bandParams[bandIndex].q = q;
    bandParams[bandIndex].filterType = filterType;
    bandParams[bandIndex].enabled = enabled;
    updateResponse();
}

float FrequencyResponseDisplay::frequencyToX(float frequency, float width) const
{
    const float logFrequency = std::log10(frequency);
    const float logMinimum = std::log10(MIN_FREQUENCY);
    const float logMaximum = std::log10(MAX_FREQUENCY);
    return (logFrequency - logMinimum) / (logMaximum - logMinimum) * width;
}

float FrequencyResponseDisplay::gainToY(float gainDB, float height) const
{
    const float normalizedGain = (gainDB - MIN_GAIN_DB) / (MAX_GAIN_DB - MIN_GAIN_DB);
    return height * (1.0f - normalizedGain);
}

float FrequencyResponseDisplay::xToFrequency(float x, float width) const
{
    if (width <= 0.0f)
        return MIN_FREQUENCY;

    const float logMinimum = std::log10(MIN_FREQUENCY);
    const float logMaximum = std::log10(MAX_FREQUENCY);
    const float logFrequency = logMinimum + (x / width) * (logMaximum - logMinimum);
    return std::pow(10.0f, logFrequency);
}

float FrequencyResponseDisplay::yToGain(float y, float height) const
{
    if (height <= 0.0f)
        return 0.0f;

    const float normalizedY = 1.0f - (y / height);
    return MIN_GAIN_DB + normalizedY * (MAX_GAIN_DB - MIN_GAIN_DB);
}

void FrequencyResponseDisplay::calculateFrequencyPoints()
{
    const float logMinimum = std::log10(MIN_FREQUENCY);
    const float logMaximum = std::log10(MAX_FREQUENCY);

    for (int pointIndex = 0; pointIndex < RESPONSE_CURVE_POINTS; ++pointIndex)
    {
        const float ratio = static_cast<float>(pointIndex) / static_cast<float>(RESPONSE_CURVE_POINTS - 1);
        const float logFrequency = logMinimum + ratio * (logMaximum - logMinimum);
        frequencyPoints[pointIndex] = std::pow(10.0f, logFrequency);
    }
}

void FrequencyResponseDisplay::calculateResponseCurve()
{
    for (int pointIndex = 0; pointIndex < RESPONSE_CURVE_POINTS; ++pointIndex)
    {
        double totalMagnitude = 1.0;

        for (int bandIndex = 0; bandIndex < NUM_BANDS; ++bandIndex)
        {
            const auto &band = bandParams[bandIndex];

            if (! band.enabled)
                continue;

            if (band.filterType == static_cast<int>(FilterType::Disabled))
                continue;

            const double bandMagnitude = calculateBiquadResponse(band, frequencyPoints[pointIndex]);
            totalMagnitude *= bandMagnitude;
        }

        responseData[pointIndex] = static_cast<float>(
            20.0 * std::log10(std::max(totalMagnitude, 1.0e-10)));
    }
}

double FrequencyResponseDisplay::calculateBiquadResponse(const BandParams &parameters, float frequency) const
{
    BiquadFilter temporaryFilter;
    temporaryFilter.setSampleRate(sampleRate);
    temporaryFilter.setCoefficients(static_cast<FilterType>(parameters.filterType),
                                     parameters.frequency,
                                     parameters.gain,
                                     parameters.q);
    return temporaryFilter.getMagnitudeResponseAtFrequency(frequency);
}

void FrequencyResponseDisplay::drawGrid(juce::Graphics &graphics)
{
    const auto bounds = getLocalBounds();
    const float width = static_cast<float>(bounds.getWidth());
    const float height = static_cast<float>(bounds.getHeight());

    graphics.setColour(juce::Colour::fromRGB(45, 50, 60));

    const std::array<float, 10> frequencyLines = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (const float frequency : frequencyLines)
    {
        const int x = static_cast<int>(frequencyToX(frequency, width));
        graphics.drawVerticalLine(x, 0.0f, height);
    }

    const std::array<float, 9> gainLines = {-20, -15, -10, -5, 0, 5, 10, 15, 20};
    for (const float gain : gainLines)
    {
        graphics.setColour(gain == 0.0f
                               ? juce::Colour::fromRGB(70, 75, 85)
                               : juce::Colour::fromRGB(45, 50, 60));
        graphics.drawHorizontalLine(static_cast<int>(gainToY(gain, height)), 0.0f, width);
    }
}

void FrequencyResponseDisplay::drawFrequencyLabels(juce::Graphics &graphics)
{
    const auto bounds = getLocalBounds();
    const float width = static_cast<float>(bounds.getWidth());
    const std::array<float, 6> frequencies = {20, 100, 1000, 5000, 10000, 20000};
    const std::array<juce::String, 6> labels = {"20", "100", "1K", "5K", "10K", "20K"};

    graphics.setColour(juce::Colours::lightgrey);
    graphics.setFont(10.0f);

    for (size_t index = 0; index < frequencies.size(); ++index)
    {
        const int x = static_cast<int>(frequencyToX(frequencies[index], width));
        graphics.drawText(labels[index], x - 15, bounds.getHeight() - 15, 30, 12,
                          juce::Justification::centred);
    }
}

void FrequencyResponseDisplay::drawGainLabels(juce::Graphics &graphics)
{
    const auto bounds = getLocalBounds();
    const float height = static_cast<float>(bounds.getHeight());
    const std::array<float, 5> gains = {-20, -10, 0, 10, 20};

    graphics.setColour(juce::Colours::lightgrey);
    graphics.setFont(10.0f);

    for (const float gain : gains)
    {
        const int y = static_cast<int>(gainToY(gain, height));
        graphics.drawText(juce::String(static_cast<int>(gain)) + "dB", 5, y - 6, 40, 12,
                          juce::Justification::left);
    }
}

void FrequencyResponseDisplay::drawResponseCurve(juce::Graphics &graphics)
{
    const auto bounds = getLocalBounds();
    const float width = static_cast<float>(bounds.getWidth());
    const float height = static_cast<float>(bounds.getHeight());
    juce::Path curvePath;

    for (int pointIndex = 0; pointIndex < RESPONSE_CURVE_POINTS; ++pointIndex)
    {
        const float x = frequencyToX(frequencyPoints[pointIndex], width);
        const float y = gainToY(responseData[pointIndex], height);

        if (pointIndex == 0)
        {
            curvePath.startNewSubPath(x, y);
            continue;
        }

        curvePath.lineTo(x, y);
    }

    graphics.setColour(juce::Colour::fromRGB(255, 140, 0));
    graphics.strokePath(curvePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
}
