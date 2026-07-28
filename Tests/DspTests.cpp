#include "DSP/BiquadFilter.h"
#include "DSP/FilterTypes.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{
    void requireCondition(bool condition, const std::string &failureMessage)
    {
        if (condition)
            return;

        std::cerr << "Test failure: " << failureMessage << '\n';
        std::exit(EXIT_FAILURE);
    }

    void requireFiniteCoefficients(const BiquadFilter::Coefficients &coefficients)
    {
        requireCondition(std::isfinite(coefficients.b0), "b0 is not finite");
        requireCondition(std::isfinite(coefficients.b1), "b1 is not finite");
        requireCondition(std::isfinite(coefficients.b2), "b2 is not finite");
        requireCondition(std::isfinite(coefficients.a1), "a1 is not finite");
        requireCondition(std::isfinite(coefficients.a2), "a2 is not finite");
    }

    void testFilterRestrictions()
    {
        auto lowBandTypes = FilterTypeRestrictions::getAvailableTypes(BandPosition::Low);
        auto middleBandTypes = FilterTypeRestrictions::getAvailableTypes(BandPosition::Mid);
        auto highBandTypes = FilterTypeRestrictions::getAvailableTypes(BandPosition::High);

        requireCondition(FilterTypeRestrictions::isValidTypeForBand(FilterType::LowCut, BandPosition::Low),
                         "Band 1 should accept low cut");
        requireCondition(!FilterTypeRestrictions::isValidTypeForBand(FilterType::Peak, BandPosition::Low),
                         "Band 1 should reject peak");
        requireCondition(FilterTypeRestrictions::isValidTypeForBand(FilterType::Peak, BandPosition::Mid),
                         "middle bands should accept peak");
        requireCondition(FilterTypeRestrictions::isValidTypeForBand(FilterType::HighCut, BandPosition::High),
                         "Band 5 should accept high cut");
        requireCondition(!FilterTypeRestrictions::isValidTypeForBand(FilterType::LowShelf, BandPosition::High),
                         "Band 5 should reject low shelf");
        requireCondition(lowBandTypes.size() == 3, "Band 1 should expose three choices");
        requireCondition(middleBandTypes.size() == 4, "middle bands should expose four choices");
        requireCondition(highBandTypes.size() == 3, "Band 5 should expose three choices");
    }

    void testBiquadCoefficients()
    {
        BiquadFilter filter;
        filter.setSampleRate(48000.0);

        const FilterType filterTypes[] = {
            FilterType::Peak,
            FilterType::LowShelf,
            FilterType::HighShelf,
            FilterType::LowCut,
            FilterType::HighCut};

        for (const auto filterType : filterTypes)
        {
            filter.setCoefficients(filterType, 1000.0, 6.0, 0.707);
            requireFiniteCoefficients(filter.getCoefficients());
        }
    }

    void testNeutralPeakIsUnity()
    {
        BiquadFilter filter;
        filter.setSampleRate(48000.0);
        filter.setCoefficients(FilterType::Peak, 1000.0, 0.0, 0.707);

        const auto coefficients = filter.getCoefficients();
        const float input = 0.25f;
        const float output = filter.processSample(input);

        requireCondition(std::abs(output - input) < 0.0001f,
                         "neutral peak filter should initially pass a sample unchanged");
        requireFiniteCoefficients(coefficients);
    }

    void testCutFiltersRemainActiveAtZeroGain()
    {
        BiquadFilter filter;
        filter.setSampleRate(48000.0);

        filter.setCoefficients(FilterType::LowCut, 1000.0, 0.0, 0.707);
        const double lowCutAtDc = filter.getMagnitudeResponseAtFrequency(0.0);

        filter.setCoefficients(FilterType::HighCut, 1000.0, 0.0, 0.707);
        const double highCutAtNyquist = filter.getMagnitudeResponseAtFrequency(24000.0);

        requireCondition(lowCutAtDc < 0.01f, "low cut should attenuate DC at zero gain");
        requireCondition(highCutAtNyquist < 0.01f, "high cut should attenuate Nyquist at zero gain");
    }

    void testBoundaryInputsRemainFinite()
    {
        BiquadFilter filter;
        filter.setSampleRate(std::numeric_limits<double>::quiet_NaN());
        filter.setCoefficients(FilterType::Peak,
                                std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::infinity(),
                                0.0);

        requireFiniteCoefficients(filter.getCoefficients());
        requireCondition(std::isfinite(filter.getMagnitudeResponseAtFrequency(1000.0)),
                         "boundary response should remain finite");
        requireCondition(std::isfinite(filter.processSample(0.25f)),
                         "boundary processing should remain finite");
    }

    void testSharedResponseMathShowsPeakGain()
    {
        BiquadFilter filter;
        filter.setSampleRate(48000.0);
        filter.setCoefficients(FilterType::Peak, 1000.0, 6.0, 0.707);

        const double magnitudeAtCenter = filter.getMagnitudeResponseAtFrequency(1000.0);
        requireCondition(magnitudeAtCenter > 1.5,
                         "positive peak gain should appear in the shared response calculation");
    }
}

int main()
{
    testFilterRestrictions();
    testBiquadCoefficients();
    testNeutralPeakIsUnity();
    testCutFiltersRemainActiveAtZeroGain();
    testBoundaryInputsRemainFinite();
    testSharedResponseMathShowsPeakGain();

    std::cout << "All DSP tests passed.\n";
    return EXIT_SUCCESS;
}
