#include "FrequencyResponseDisplay.h"
#include "../DSP/FilterTypes.h"
#include "../DSP/BiquadFilter.h"
#include <cmath>
#include <complex>

FrequencyResponseDisplay::FrequencyResponseDisplay()
{
    // Initialize frequency points logarithmically
    calculateFrequencyPoints();

    // Initialize all response data to 0dB (no change)
    responseData.fill(0.0f);

    // Set up default band parameters
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        bandParams[i].frequency = 100.0f * std::pow(10.0f, i * 0.6f); // Spread across spectrum
        bandParams[i].gain = 0.0f;
        bandParams[i].q = 0.7f;
        bandParams[i].filterType = static_cast<int>(FilterType::Peak);
        bandParams[i].enabled = true;
    }
}

void FrequencyResponseDisplay::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate > 1.0 ? newSampleRate : 44100.0;
    updateResponse();
}

FrequencyResponseDisplay::~FrequencyResponseDisplay()
{
}

void FrequencyResponseDisplay::paint(juce::Graphics &g)
{
    // Professional dark background like Pro-Q
    g.fillAll(juce::Colour::fromRGB(25, 28, 35));

    // Draw grid and labels first (background elements)
    drawGrid(g);
    drawFrequencyLabels(g);
    drawGainLabels(g);

    // Draw the frequency response curve on top
    drawResponseCurve(g);
}

void FrequencyResponseDisplay::resized()
{
    // Recalculate curve when component size changes
    updateResponse();
}

void FrequencyResponseDisplay::updateResponse()
{
    calculateResponseCurve();
    repaint(); // Trigger redraw
}

void FrequencyResponseDisplay::setBandParameters(int bandIndex, float freq, float gain, float q, int filterType)
{
    if (bandIndex >= 0 && bandIndex < NUM_BANDS)
    {
        bandParams[bandIndex].frequency = freq;
        bandParams[bandIndex].gain = gain;
        bandParams[bandIndex].q = q;
        bandParams[bandIndex].filterType = filterType;

        // Update the display in real-time
        updateResponse();
    }
}

//==============================================================================
// Coordinate transformation functions (Critical for professional visualization)

float FrequencyResponseDisplay::frequencyToX(float freq, float width) const
{
    // Logarithmic scaling - industry standard for audio
    float logFreq = std::log10(freq);
    float logMin = std::log10(MIN_FREQUENCY);
    float logMax = std::log10(MAX_FREQUENCY);

    return (logFreq - logMin) / (logMax - logMin) * width;
}

float FrequencyResponseDisplay::gainToY(float gainDB, float height) const
{
    // Linear dB scaling with 0dB at center
    float normalizedGain = (gainDB - MIN_GAIN_DB) / (MAX_GAIN_DB - MIN_GAIN_DB);
    return height * (1.0f - normalizedGain); // Flip Y axis (0dB at center)
}

float FrequencyResponseDisplay::xToFrequency(float x, float width) const
{
    float logMin = std::log10(MIN_FREQUENCY);
    float logMax = std::log10(MAX_FREQUENCY);
    float logFreq = logMin + (x / width) * (logMax - logMin);

    return std::pow(10.0f, logFreq);
}

float FrequencyResponseDisplay::yToGain(float y, float height) const
{
    float normalizedY = 1.0f - (y / height);
    return MIN_GAIN_DB + normalizedY * (MAX_GAIN_DB - MIN_GAIN_DB);
}

//==============================================================================
// DSP Mathematics - The Heart of EQ Visualization

void FrequencyResponseDisplay::calculateFrequencyPoints()
{
    // Create logarithmically spaced frequency points
    float logMin = std::log10(MIN_FREQUENCY);
    float logMax = std::log10(MAX_FREQUENCY);

    for (int i = 0; i < RESPONSE_CURVE_POINTS; ++i)
    {
        float ratio = static_cast<float>(i) / (RESPONSE_CURVE_POINTS - 1);
        float logFreq = logMin + ratio * (logMax - logMin);
        frequencyPoints[i] = std::pow(10.0f, logFreq);
    }
}

void FrequencyResponseDisplay::calculateResponseCurve()
{
    // Calculate combined response from all active bands
    for (int i = 0; i < RESPONSE_CURVE_POINTS; ++i)
    {
        std::complex<double> totalResponse(1.0, 0.0); // Start with unity gain

        // Multiply responses from all bands (addition in log domain)
        for (int bandIndex = 0; bandIndex < NUM_BANDS; ++bandIndex)
        {
            if (bandParams[bandIndex].enabled
                && bandParams[bandIndex].filterType != static_cast<int>(FilterType::Disabled))
            {
                auto bandResponse = calculateBiquadResponse(bandParams[bandIndex], frequencyPoints[i]);
                totalResponse *= bandResponse;
            }
        }

        // Convert to dB
        double magnitude = std::abs(totalResponse);
        responseData[i] = static_cast<float>(20.0 * std::log10(std::max(magnitude, 1e-10))); // Avoid log(0)
    }
}

std::complex<double> FrequencyResponseDisplay::calculateBiquadResponse(const BandParams &params, float freq) const
{
    // This is where the real DSP magic happens!
    // We're calculating H(jω) = (b₀ + b₁z⁻¹ + b₂z⁻²) / (1 + a₁z⁻¹ + a₂z⁻²)

    // First, we need to create a biquad filter with these parameters
    // and get its coefficients
    BiquadFilter tempFilter;

    // Convert our filter type enum to the filter type the BiquadFilter expects
    FilterType filterType = static_cast<FilterType>(params.filterType);

    // Calculate the biquad coefficients for this band
    tempFilter.setSampleRate(sampleRate);
    tempFilter.setCoefficients(filterType, params.frequency, params.gain, params.q);

    // Get the coefficients
    auto coeffs = tempFilter.getCoefficients();

    // Calculate the complex frequency response
    double omega = juce::MathConstants<double>::twoPi * freq / sampleRate;
    std::complex<double> z = std::exp(std::complex<double>(0, -omega)); // e^(-jω)
    std::complex<double> z2 = z * z;                                    // e^(-j2ω)

    // Numerator: b₀ + b₁z⁻¹ + b₂z⁻²
    std::complex<double> numerator = std::complex<double>(coeffs.b0) + std::complex<double>(coeffs.b1) * z + std::complex<double>(coeffs.b2) * z2;

    // Denominator: 1 + a₁z⁻¹ + a₂z⁻²
    std::complex<double> denominator = std::complex<double>(1.0) + std::complex<double>(coeffs.a1) * z + std::complex<double>(coeffs.a2) * z2;

    return numerator / denominator;
}

//==============================================================================
// Professional Graphics Rendering

void FrequencyResponseDisplay::drawGrid(juce::Graphics &g)
{
    auto bounds = getLocalBounds();
    g.setColour(juce::Colour::fromRGB(45, 50, 60)); // Subtle grid lines

    // Vertical frequency grid lines (octaves)
    std::array<float, 10> freqLines = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};

    for (auto freq : freqLines)
    {
        if (freq >= MIN_FREQUENCY && freq <= MAX_FREQUENCY)
        {
            const float componentWidth = static_cast<float>(bounds.getWidth());
            float x = frequencyToX(freq, componentWidth);
            g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(bounds.getHeight()));
        }
    }

    // Horizontal gain grid lines
    std::array<float, 9> gainLines = {-20, -15, -10, -5, 0, 5, 10, 15, 20};

    for (auto gain : gainLines)
    {
        const float componentHeight = static_cast<float>(bounds.getHeight());
        float y = gainToY(gain, componentHeight);
        g.setColour(gain == 0.0f ? juce::Colour::fromRGB(70, 75, 85) : juce::Colour::fromRGB(45, 50, 60));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(bounds.getWidth()));
    }
}

void FrequencyResponseDisplay::drawFrequencyLabels(juce::Graphics &g)
{
    auto bounds = getLocalBounds();
    g.setColour(juce::Colours::lightgrey);
    g.setFont(10.0f);

    std::array<float, 6> labelFreqs = {20, 100, 1000, 5000, 10000, 20000};
    std::array<juce::String, 6> labels = {"20", "100", "1K", "5K", "10K", "20K"};

    for (size_t i = 0; i < labelFreqs.size() && i < labels.size(); ++i)
    {
        const float componentWidth = static_cast<float>(bounds.getWidth());
        float x = frequencyToX(labelFreqs[i], componentWidth);
        g.drawText(labels[i], static_cast<int>(x - 15), bounds.getHeight() - 15, 30, 12, juce::Justification::centred);
    }
}

void FrequencyResponseDisplay::drawGainLabels(juce::Graphics &g)
{
    auto bounds = getLocalBounds();
    g.setColour(juce::Colours::lightgrey);
    g.setFont(10.0f);

    std::array<float, 5> labelGains = {-20, -10, 0, 10, 20};

    for (auto gain : labelGains)
    {
        const float componentHeight = static_cast<float>(bounds.getHeight());
        float y = gainToY(gain, componentHeight);
        juce::String label = juce::String(static_cast<int>(gain)) + "dB";
        g.drawText(label, 5, static_cast<int>(y - 6), 40, 12, juce::Justification::left);
    }
}

void FrequencyResponseDisplay::drawResponseCurve(juce::Graphics &g)
{
    auto bounds = getLocalBounds();

    // Create the curve path
    juce::Path curvePath;
    bool firstPoint = true;

    for (int i = 0; i < RESPONSE_CURVE_POINTS; ++i)
    {
        const float componentWidth = static_cast<float>(bounds.getWidth());
        const float componentHeight = static_cast<float>(bounds.getHeight());
        float x = frequencyToX(frequencyPoints[i], componentWidth);
        float y = gainToY(responseData[i], componentHeight);

        if (firstPoint)
        {
            curvePath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            curvePath.lineTo(x, y);
        }
    }

    // Draw the curve with professional styling
    g.setColour(juce::Colour::fromRGB(255, 140, 0)); // Professional orange like Pro-Q
    g.strokePath(curvePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
}
