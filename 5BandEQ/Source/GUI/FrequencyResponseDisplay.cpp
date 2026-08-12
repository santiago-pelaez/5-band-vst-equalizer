#include "FrequencyResponseDisplay.h"
#include "../DSP/FilterTypes.h"

#include <algorithm>
#include <array>
#include <cmath>

FrequencyResponseDisplay::FrequencyResponseDisplay()
{
    calculateFrequencyPoints();
    responseData.fill(0.0f);
    inputSpectrumData.fill(MIN_SPECTRUM_DB);
    outputSpectrumData.fill(MIN_SPECTRUM_DB);

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
    spectrumSampleRate = sampleRate;
    updateResponse();
}

void FrequencyResponseDisplay::setSpectrumFrame(const SpectrumAnalyzer::SpectrumFrame &frame)
{
    constexpr float spectrumSmoothingAmount = 0.35f;
    const bool frameHasValidSampleRate = std::isfinite(frame.sampleRate) && frame.sampleRate > 1.0;
    const bool sampleRateChanged = frameHasValidSampleRate && std::abs(sampleRate - frame.sampleRate) > 0.5;

    if (frameHasValidSampleRate)
    {
        sampleRate = frame.sampleRate;
        spectrumSampleRate = frame.sampleRate;
    }

    for (size_t binIndex = 0; binIndex < inputSpectrumData.size(); ++binIndex)
    {
        inputSpectrumData[binIndex] = juce::jmap(spectrumSmoothingAmount,
                                                inputSpectrumData[binIndex],
                                                frame.inputSpectrum[binIndex]);
        outputSpectrumData[binIndex] = juce::jmap(spectrumSmoothingAmount,
                                                 outputSpectrumData[binIndex],
                                                 frame.outputSpectrum[binIndex]);
    }
    hasSpectrumData = true;

    if (sampleRateChanged)
        calculateResponseCurve();

    repaint();
}

void FrequencyResponseDisplay::paint(juce::Graphics &graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(25, 28, 35));

    const auto plotBounds = getPlotBounds();
    graphics.setColour(juce::Colour::fromRGB(34, 38, 48));
    graphics.fillRect(plotBounds);
    graphics.setColour(juce::Colour::fromRGB(80, 86, 102));
    graphics.drawRect(plotBounds, 1);

    graphics.setColour(juce::Colours::lightgrey);
    graphics.setFont(11.0f);
    graphics.drawText("SPECTRUM + EQ RESPONSE", plotBounds.getX() + 8, plotBounds.getY() + 4, 180, 16,
                      juce::Justification::left);

    drawGrid(graphics);
    drawFrequencyLabels(graphics);
    drawGainLabels(graphics);
    drawSpectrum(graphics);
    drawSpectrumLegend(graphics);
    drawResponseCurve(graphics);
    drawNodes(graphics);
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

juce::Rectangle<int> FrequencyResponseDisplay::getPlotBounds() const
{
    auto plotBounds = getLocalBounds().reduced(42, 8);
    plotBounds.removeFromTop(20);
    plotBounds.removeFromBottom(22);
    return plotBounds;
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

void FrequencyResponseDisplay::setSelectedBand(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= NUM_BANDS)
    {
        selectedBandIndex = -1;
        repaint();
        return;
    }

    selectedBandIndex = bandIndex;
    repaint();
}

void FrequencyResponseDisplay::mouseDown(const juce::MouseEvent &event)
{
    const int hitBandIndex = findNodeAtPosition(event.position);

    if (hitBandIndex < 0)
        return;

    draggedBandIndex = hitBandIndex;
    selectedBandIndex = hitBandIndex;

    if (onBandSelected)
        onBandSelected(hitBandIndex);

    updateDraggedNode(event.position, NodeEditPhase::Begin);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void FrequencyResponseDisplay::mouseDrag(const juce::MouseEvent &event)
{
    if (draggedBandIndex < 0)
        return;

    updateDraggedNode(event.position, NodeEditPhase::Update);
}

void FrequencyResponseDisplay::mouseUp(const juce::MouseEvent &event)
{
    if (draggedBandIndex < 0)
        return;

    updateDraggedNode(event.position, NodeEditPhase::End);
    draggedBandIndex = -1;
    setMouseCursor(juce::MouseCursor::NormalCursor);
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
    const float visibleGain = juce::jlimit(MIN_GAIN_DB, MAX_GAIN_DB, gainDB);
    const float normalizedGain = (visibleGain - MIN_GAIN_DB) / (MAX_GAIN_DB - MIN_GAIN_DB);
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

    const float normalizedY = juce::jlimit(0.0f, 1.0f, 1.0f - (y / height));
    return MIN_GAIN_DB + normalizedY * (MAX_GAIN_DB - MIN_GAIN_DB);
}

float FrequencyResponseDisplay::spectrumDBToY(float decibels, float height) const
{
    if (height <= 0.0f)
        return 0.0f;

    const float safeDecibels = std::isfinite(decibels) ? decibels : MIN_SPECTRUM_DB;
    const float normalizedMagnitude = juce::jmap(
        juce::jlimit(MIN_SPECTRUM_DB, MAX_SPECTRUM_DB, safeDecibels),
        MIN_SPECTRUM_DB,
        MAX_SPECTRUM_DB,
        0.0f,
        1.0f);

    return height * (1.0f - normalizedMagnitude);
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
    const auto plotBounds = getPlotBounds();
    const float width = static_cast<float>(plotBounds.getWidth());
    const float height = static_cast<float>(plotBounds.getHeight());
    const int plotX = plotBounds.getX();
    const int plotY = plotBounds.getY();

    graphics.setColour(juce::Colour::fromRGB(45, 50, 60));

    const std::array<float, 10> frequencyLines = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (const float frequency : frequencyLines)
    {
        const int x = plotX + static_cast<int>(frequencyToX(frequency, width));
        graphics.drawVerticalLine(x, static_cast<float>(plotY), static_cast<float>(plotBounds.getBottom()));
    }

    const std::array<float, 9> gainLines = {-24, -18, -12, -6, 0, 6, 12, 18, 24};
    for (const float gain : gainLines)
    {
        graphics.setColour(gain == 0.0f
                               ? juce::Colour::fromRGB(70, 75, 85)
                               : juce::Colour::fromRGB(45, 50, 60));
        graphics.drawHorizontalLine(plotY + static_cast<int>(gainToY(gain, height)),
                                    static_cast<float>(plotX),
                                    static_cast<float>(plotBounds.getRight()));
    }
}

void FrequencyResponseDisplay::drawFrequencyLabels(juce::Graphics &graphics)
{
    const auto plotBounds = getPlotBounds();
    const float width = static_cast<float>(plotBounds.getWidth());
    const std::array<float, 6> frequencies = {20, 100, 1000, 5000, 10000, 20000};
    const std::array<juce::String, 6> labels = {"20", "100", "1K", "5K", "10K", "20K"};

    graphics.setColour(juce::Colours::lightgrey);
    graphics.setFont(10.0f);

    for (size_t index = 0; index < frequencies.size(); ++index)
    {
        const int x = plotBounds.getX() + static_cast<int>(frequencyToX(frequencies[index], width));
        graphics.drawText(labels[index], x - 18, plotBounds.getBottom() + 4, 36, 12,
                          juce::Justification::centred);
    }
}

void FrequencyResponseDisplay::drawGainLabels(juce::Graphics &graphics)
{
    const auto plotBounds = getPlotBounds();
    const float height = static_cast<float>(plotBounds.getHeight());
    const std::array<float, 5> gains = {-24, -12, 0, 12, 24};

    graphics.setColour(juce::Colours::lightgrey);
    graphics.setFont(10.0f);

    for (const float gain : gains)
    {
        const int y = plotBounds.getY() + static_cast<int>(gainToY(gain, height));
        graphics.drawText(juce::String(static_cast<int>(gain)) + "dB", 2, y - 6, 36, 12,
                          juce::Justification::left);
    }
}

void FrequencyResponseDisplay::drawSpectrum(juce::Graphics &graphics)
{
    if (! hasSpectrumData)
        return;

    drawSpectrumTrace(graphics,
                      inputSpectrumData,
                      juce::Colour::fromRGB(80, 190, 240).withAlpha(0.62f));
    drawSpectrumTrace(graphics,
                      outputSpectrumData,
                      juce::Colour::fromRGB(120, 220, 170).withAlpha(0.72f));
}

void FrequencyResponseDisplay::drawSpectrumTrace(
    juce::Graphics &graphics,
    const std::array<float, SpectrumAnalyzer::SPECTRUM_BINS> &spectrum,
    juce::Colour colour)
{
    const auto plotBounds = getPlotBounds();
    const float width = static_cast<float>(plotBounds.getWidth());
    const float height = static_cast<float>(plotBounds.getHeight());
    juce::Path spectrumPath;
    bool pathHasStarted = false;

    for (int binIndex = 1; binIndex < SpectrumAnalyzer::SPECTRUM_BINS; ++binIndex)
    {
        const float frequency = static_cast<float>(binIndex) * static_cast<float>(spectrumSampleRate)
                              / static_cast<float>(SpectrumAnalyzer::FFT_SIZE);

        if (frequency < MIN_FREQUENCY)
            continue;

        if (frequency > MAX_FREQUENCY)
            break;

        const float x = static_cast<float>(plotBounds.getX()) + frequencyToX(frequency, width);
        const float y = static_cast<float>(plotBounds.getY())
                      + spectrumDBToY(spectrum[static_cast<size_t>(binIndex)], height);

        if (! pathHasStarted)
        {
            spectrumPath.startNewSubPath(x, y);
            pathHasStarted = true;
            continue;
        }

        spectrumPath.lineTo(x, y);
    }

    if (! pathHasStarted)
        return;

    graphics.setColour(colour);
    graphics.strokePath(spectrumPath,
                        juce::PathStrokeType(1.25f, juce::PathStrokeType::curved));
}

void FrequencyResponseDisplay::drawSpectrumLegend(juce::Graphics &graphics)
{
    const auto plotBounds = getPlotBounds();
    const int legendWidth = 250;
    const int legendX = plotBounds.getRight() - legendWidth - 8;
    const int legendY = plotBounds.getY() + 5;

    graphics.setColour(juce::Colour::fromRGB(20, 23, 30).withAlpha(0.82f));
    graphics.fillRoundedRectangle(static_cast<float>(legendX),
                                  static_cast<float>(legendY),
                                  static_cast<float>(legendWidth),
                                  22.0f,
                                  4.0f);

    graphics.setFont(10.0f);
    graphics.setColour(juce::Colour::fromRGB(80, 190, 240));
    graphics.drawText("INPUT", legendX + 8, legendY + 5, 42, 12, juce::Justification::left);
    graphics.setColour(juce::Colour::fromRGB(120, 220, 170));
    graphics.drawText("OUTPUT", legendX + 57, legendY + 5, 52, 12, juce::Justification::left);
    graphics.setColour(juce::Colour::fromRGB(255, 140, 0));
    graphics.drawText("EQ", legendX + 117, legendY + 5, 24, 12, juce::Justification::left);
    graphics.setColour(juce::Colours::lightgrey);
    graphics.drawText("spectrum -96 to 0 dBFS", legendX + 146, legendY + 5, 98, 12,
                      juce::Justification::left);
}

void FrequencyResponseDisplay::drawResponseCurve(juce::Graphics &graphics)
{
    const auto plotBounds = getPlotBounds();
    const float width = static_cast<float>(plotBounds.getWidth());
    const float height = static_cast<float>(plotBounds.getHeight());
    juce::Path curvePath;

    for (int pointIndex = 0; pointIndex < RESPONSE_CURVE_POINTS; ++pointIndex)
    {
        const float x = static_cast<float>(plotBounds.getX())
                      + frequencyToX(frequencyPoints[pointIndex], width);
        const float y = static_cast<float>(plotBounds.getY())
                      + gainToY(responseData[pointIndex], height);

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

void FrequencyResponseDisplay::drawNodes(juce::Graphics &graphics)
{
    const std::array<juce::Colour, NUM_BANDS> nodeColours = {
        juce::Colour::fromRGB(80, 190, 240),
        juce::Colour::fromRGB(120, 220, 170),
        juce::Colour::fromRGB(255, 210, 90),
        juce::Colour::fromRGB(255, 155, 90),
        juce::Colour::fromRGB(225, 115, 220)};

    for (int bandIndex = 0; bandIndex < NUM_BANDS; ++bandIndex)
    {
        const auto &band = bandParams[bandIndex];

        if (! band.enabled || band.filterType == static_cast<int>(FilterType::Disabled))
            continue;

        const auto nodePosition = getNodePosition(bandIndex);
        const bool isSelected = bandIndex == selectedBandIndex;
        const float outerRadius = isSelected ? 9.0f : 7.0f;

        graphics.setColour(juce::Colours::black.withAlpha(0.65f));
        graphics.fillEllipse(nodePosition.x - outerRadius - 2.0f,
                             nodePosition.y - outerRadius - 2.0f,
                             (outerRadius + 2.0f) * 2.0f,
                             (outerRadius + 2.0f) * 2.0f);
        graphics.setColour(nodeColours[static_cast<size_t>(bandIndex)]);
        graphics.fillEllipse(nodePosition.x - outerRadius,
                             nodePosition.y - outerRadius,
                             outerRadius * 2.0f,
                             outerRadius * 2.0f);
        graphics.setColour(juce::Colours::white);
        graphics.drawEllipse(nodePosition.x - outerRadius,
                             nodePosition.y - outerRadius,
                             outerRadius * 2.0f,
                             outerRadius * 2.0f,
                             isSelected ? 2.0f : 1.0f);
    }
}

int FrequencyResponseDisplay::findNodeAtPosition(juce::Point<float> position) const
{
    constexpr float NODE_HIT_RADIUS = 13.0f;
    int closestBandIndex = -1;
    float closestDistanceSquared = NODE_HIT_RADIUS * NODE_HIT_RADIUS;

    for (int bandIndex = 0; bandIndex < NUM_BANDS; ++bandIndex)
    {
        const auto &band = bandParams[bandIndex];

        if (! band.enabled || band.filterType == static_cast<int>(FilterType::Disabled))
            continue;

        const auto distance = position.getDistanceSquaredFrom(getNodePosition(bandIndex));
        if (distance > closestDistanceSquared)
            continue;

        closestDistanceSquared = distance;
        closestBandIndex = bandIndex;
    }

    return closestBandIndex;
}

juce::Point<float> FrequencyResponseDisplay::getNodePosition(int bandIndex) const
{
    const auto plotBounds = getPlotBounds();
    const auto &band = bandParams[bandIndex];
    const float width = static_cast<float>(plotBounds.getWidth());
    const float height = static_cast<float>(plotBounds.getHeight());
    const float nodeGain = band.filterType == static_cast<int>(FilterType::LowCut)
                               || band.filterType == static_cast<int>(FilterType::HighCut)
                               ? 0.0f
                               : band.gain;

    return {
        static_cast<float>(plotBounds.getX()) + frequencyToX(band.frequency, width),
        static_cast<float>(plotBounds.getY()) + gainToY(nodeGain, height)};
}

void FrequencyResponseDisplay::updateDraggedNode(juce::Point<float> position, NodeEditPhase phase)
{
    if (draggedBandIndex < 0 || draggedBandIndex >= NUM_BANDS)
        return;

    const auto plotBounds = getPlotBounds();
    const float width = static_cast<float>(plotBounds.getWidth());
    const float height = static_cast<float>(plotBounds.getHeight());
    const float localX = juce::jlimit(0.0f, width, position.x - static_cast<float>(plotBounds.getX()));
    const float localY = juce::jlimit(0.0f, height, position.y - static_cast<float>(plotBounds.getY()));
    auto &band = bandParams[draggedBandIndex];

    band.frequency = juce::jlimit(MIN_FREQUENCY, MAX_FREQUENCY, xToFrequency(localX, width));

    const bool gainIsEditable = band.filterType != static_cast<int>(FilterType::LowCut)
                             && band.filterType != static_cast<int>(FilterType::HighCut);
    if (gainIsEditable)
        band.gain = juce::jlimit(MIN_GAIN_DB, MAX_GAIN_DB, yToGain(localY, height));
    else
        band.gain = 0.0f;

    updateResponse();

    if (! onNodeEdit)
        return;

    onNodeEdit({draggedBandIndex, phase, band.frequency, band.gain, gainIsEditable});
}
