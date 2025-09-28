#pragma once
#include <JuceHeader.h>
#include "FilterTypes.h"

/**
 * Biquad filter implementation for all EQ filter types
 * Handles coefficient calculation and audio processing
 */
class BiquadFilter
{
public:
    BiquadFilter() = default;

    void setSampleRate(double sampleRate)
    {
        this->sampleRate = sampleRate;
        reset();
    }

    void setCoefficients(FilterType type, double frequency, double gain, double Q)
    {
        calculateCoefficients(type, frequency, gain, Q);
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
            double S = 1.0; // Shelf slope parameter
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
            double S = 1.0; // Shelf slope parameter
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

        // Normalize coefficients
        b0 = static_cast<float>(b0_temp / a0);
        b1 = static_cast<float>(b1_temp / a0);
        b2 = static_cast<float>(b2_temp / a0);
        a1 = static_cast<float>(a1_temp / a0);
        a2 = static_cast<float>(a2_temp / a0);
    }
};