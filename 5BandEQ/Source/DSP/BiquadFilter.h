#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include "FilterTypes.h"

/**
 * Biquad filter implementation for all EQ filter types
 * Handles coefficient calculation and audio processing
 */
class BiquadFilter
{
public:
    BiquadFilter() = default;

    void setSampleRate(double newSampleRate)
    {
        if (! std::isfinite(newSampleRate) || newSampleRate <= 1.0)
        {
            this->sampleRate = 44100.0;
            reset();
            return;
        }

        this->sampleRate = newSampleRate;
        reset();
    }

    void setCoefficients(FilterType type, double frequency, double gain, double Q)
    {
        const double finiteFrequency = std::isfinite(frequency) ? frequency : 1000.0;
        const double finiteGain = std::isfinite(gain) ? gain : 0.0;
        const double finiteQ = std::isfinite(Q) ? Q : 1.0;
        const double maximumFrequency = std::max(1.0, sampleRate * 0.49);
        const double safeFrequency = std::clamp(finiteFrequency, 1.0, maximumFrequency);
        const double safeGain = std::clamp(finiteGain, -60.0, 60.0);
        const double safeQ = std::clamp(finiteQ, 0.05, 100.0);

        calculateCoefficients(type, safeFrequency, safeGain, safeQ);
    }

    // Coefficient structure for frequency response analysis
    struct Coefficients
    {
        float b0, b1, b2, a1, a2;
    };

    // Get current filter coefficients for frequency response calculation
    Coefficients getCoefficients() const
    {
        return {b0, b1, b2, a1, a2};
    }

    double getMagnitudeResponseAtFrequency(double frequency) const
    {
        if (! std::isfinite(frequency) || frequency < 0.0)
            return 1.0;

        const double safeFrequency = std::clamp(frequency, 0.0, sampleRate * 0.5);
        const double angularFrequency = juce::MathConstants<double>::twoPi * safeFrequency / sampleRate;
        const std::complex<double> z = std::exp(std::complex<double>(0.0, -angularFrequency));
        const std::complex<double> zSquared = z * z;
        const std::complex<double> numerator = static_cast<double>(b0)
                                             + static_cast<double>(b1) * z
                                             + static_cast<double>(b2) * zSquared;
        const std::complex<double> denominator = 1.0
                                               + static_cast<double>(a1) * z
                                               + static_cast<double>(a2) * zSquared;
        const double denominatorMagnitude = std::abs(denominator);

        if (! std::isfinite(denominatorMagnitude) || denominatorMagnitude < 1.0e-12)
            return 0.0;

        const double magnitude = std::abs(numerator / denominator);
        return std::isfinite(magnitude) ? magnitude : 0.0;
    }

    float processSample(float input)
    {
        // Direct Form II Transposed implementation
        float output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;
        return output;
    }

    void reset()
    {
        z1 = z2 = 0.0f;
    }

private:
    void setPassThroughCoefficients()
    {
        b0 = 1.0f;
        b1 = 0.0f;
        b2 = 0.0f;
        a1 = 0.0f;
        a2 = 0.0f;
    }

    bool areFinite(double candidateB0, double candidateB1, double candidateB2,
                   double candidateA1, double candidateA2) const
    {
        return std::isfinite(candidateB0)
            && std::isfinite(candidateB1)
            && std::isfinite(candidateB2)
            && std::isfinite(candidateA1)
            && std::isfinite(candidateA2);
    }

    double sampleRate = 44100.0;

    // Filter coefficients
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    // State variables
    float z1 = 0.0f, z2 = 0.0f;

    void calculateCoefficients(FilterType type, double freq, double gain, double Q)
    {
        const double omega = juce::MathConstants<double>::twoPi * freq / sampleRate;
        const double sin_omega = std::sin(omega);
        const double cos_omega = std::cos(omega);
        const double alpha = sin_omega / (2.0 * Q);
        const double A = std::pow(10.0, gain / 40.0); // For shelving filters

        double b0_temp, b1_temp, b2_temp, a0, a1_temp, a2_temp;

        switch (type)
        {
        case FilterType::Peak:
            // Peaking EQ
            b0_temp = 1.0 + alpha * A;
            b1_temp = -2.0 * cos_omega;
            b2_temp = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1_temp = -2.0 * cos_omega;
            a2_temp = 1.0 - alpha / A;
            break;

        case FilterType::LowShelf:
        {
            double beta = std::sqrt(A) / Q;

            b0_temp = A * ((A + 1.0) - (A - 1.0) * cos_omega + beta * sin_omega);
            b1_temp = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_omega);
            b2_temp = A * ((A + 1.0) - (A - 1.0) * cos_omega - beta * sin_omega);
            a0 = (A + 1.0) + (A - 1.0) * cos_omega + beta * sin_omega;
            a1_temp = -2.0 * ((A - 1.0) + (A + 1.0) * cos_omega);
            a2_temp = (A + 1.0) + (A - 1.0) * cos_omega - beta * sin_omega;
        }
        break;

        case FilterType::HighShelf:
        {
            double beta = std::sqrt(A) / Q;

            b0_temp = A * ((A + 1.0) + (A - 1.0) * cos_omega + beta * sin_omega);
            b1_temp = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_omega);
            b2_temp = A * ((A + 1.0) + (A - 1.0) * cos_omega - beta * sin_omega);
            a0 = (A + 1.0) - (A - 1.0) * cos_omega + beta * sin_omega;
            a1_temp = 2.0 * ((A - 1.0) - (A + 1.0) * cos_omega);
            a2_temp = (A + 1.0) - (A - 1.0) * cos_omega - beta * sin_omega;
        }
        break;

        case FilterType::LowCut:
            // High-pass filter
            b0_temp = (1.0 + cos_omega) / 2.0;
            b1_temp = -(1.0 + cos_omega);
            b2_temp = (1.0 + cos_omega) / 2.0;
            a0 = 1.0 + alpha;
            a1_temp = -2.0 * cos_omega;
            a2_temp = 1.0 - alpha;
            break;

        case FilterType::HighCut:
            // Low-pass filter
            b0_temp = (1.0 - cos_omega) / 2.0;
            b1_temp = 1.0 - cos_omega;
            b2_temp = (1.0 - cos_omega) / 2.0;
            a0 = 1.0 + alpha;
            a1_temp = -2.0 * cos_omega;
            a2_temp = 1.0 - alpha;
            break;

        default:
            // Pass-through (disabled)
            b0_temp = 1.0;
            b1_temp = 0.0;
            b2_temp = 0.0;
            a0 = 1.0;
            a1_temp = 0.0;
            a2_temp = 0.0;
            break;
        }

        if (! std::isfinite(a0) || std::abs(a0) < 1.0e-12)
        {
            setPassThroughCoefficients();
            return;
        }

        // Normalize coefficients and reject invalid results before they reach
        // the audio state variables.
        const double normalizedB0 = b0_temp / a0;
        const double normalizedB1 = b1_temp / a0;
        const double normalizedB2 = b2_temp / a0;
        const double normalizedA1 = a1_temp / a0;
        const double normalizedA2 = a2_temp / a0;

        if (! areFinite(normalizedB0, normalizedB1, normalizedB2, normalizedA1, normalizedA2))
        {
            setPassThroughCoefficients();
            return;
        }

        b0 = static_cast<float>(normalizedB0);
        b1 = static_cast<float>(normalizedB1);
        b2 = static_cast<float>(normalizedB2);
        a1 = static_cast<float>(normalizedA1);
        a2 = static_cast<float>(normalizedA2);
    }
};
