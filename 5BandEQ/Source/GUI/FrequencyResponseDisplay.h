#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "../DSP/BiquadFilter.h"
#include "../DSP/SpectrumAnalyzer.h"

/**
 * Professional frequency response visualization component
 *
 * Displays real-time EQ curve showing combined response of all bands
 * Uses logarithmic frequency scaling and dB gain scaling
 * Industry-standard visualization like Pro-Q and EQ Eight
 */
class FrequencyResponseDisplay : public juce::Component
{
public:
    enum class NodeEditPhase
    {
        Begin,
        Update,
        End
    };

    struct NodeEditEvent
    {
        int bandIndex = -1;
        NodeEditPhase phase = NodeEditPhase::Update;
        float frequency = 1000.0f;
        float gain = 0.0f;
        bool gainIsEditable = true;
    };

    FrequencyResponseDisplay();
    ~FrequencyResponseDisplay() override;

    //==============================================================================
    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;

    /**
     * Update the frequency response curve
     * Call this whenever EQ parameters change
     */
    void updateResponse();

    /**
     * Set the current filter parameters for calculation
     * @param bandIndex Band number (0-4)
     * @param freq Frequency in Hz
     * @param gain Gain in dB
     * @param q Q factor
     * @param filterType Type of filter (Peak, Shelf, Cut, etc.)
     */
    void setBandParameters(int bandIndex, float freq, float gain, float q, int filterType, bool enabled);

    void setSelectedBand(int bandIndex);

    std::function<void(int)> onBandSelected;
    std::function<void(const NodeEditEvent &)> onNodeEdit;

    /**
     * Set sample rate for frequency response calculations
     * @param newSampleRate Sample rate in Hz
     */
    void setSampleRate(double newSampleRate);

    void setSpectrumFrame(const SpectrumAnalyzer::SpectrumFrame &frame);

private:
    //==============================================================================
    static constexpr int NUM_BANDS = 5;
    static constexpr int RESPONSE_CURVE_POINTS = 512;
    static constexpr float MIN_FREQUENCY = 20.0f;
    static constexpr float MAX_FREQUENCY = 20000.0f;
    static constexpr float MIN_GAIN_DB = -24.0f;
    static constexpr float MAX_GAIN_DB = 24.0f;
    static constexpr float MIN_SPECTRUM_DB = -96.0f;
    static constexpr float MAX_SPECTRUM_DB = 0.0f;

    // Band parameters for response calculation
    struct BandParams
    {
        float frequency = 1000.0f;
        float gain = 0.0f;
        float q = 0.7f;
        int filterType = 0; // Peak
        bool enabled = true;
    };

    BandParams bandParams[NUM_BANDS];

    // Pre-calculated frequency response curve
    std::array<float, RESPONSE_CURVE_POINTS> responseData;
    std::array<float, RESPONSE_CURVE_POINTS> frequencyPoints;
    std::array<float, SpectrumAnalyzer::SPECTRUM_BINS> inputSpectrumData;
    std::array<float, SpectrumAnalyzer::SPECTRUM_BINS> outputSpectrumData;
    bool hasSpectrumData = false;
    double spectrumSampleRate = 44100.0;

    // Sample rate for calculations (will be set from processor)
    double sampleRate = 44100.0;

    //==============================================================================
    // Helper functions for coordinate transformation
    juce::Rectangle<int> getPlotBounds() const;
    float frequencyToX(float freq, float width) const;
    float gainToY(float gainDB, float height) const;
    float xToFrequency(float x, float width) const;
    float yToGain(float y, float height) const;
    float spectrumDBToY(float decibels, float height) const;

    // DSP calculation functions
    void calculateFrequencyPoints();
    void calculateResponseCurve();
    double calculateBiquadResponse(const BandParams &params, float freq) const;

    // Drawing functions
    void drawGrid(juce::Graphics &g);
    void drawFrequencyLabels(juce::Graphics &g);
    void drawGainLabels(juce::Graphics &g);
    void drawSpectrum(juce::Graphics &g);
    void drawSpectrumTrace(juce::Graphics &g,
                           const std::array<float, SpectrumAnalyzer::SPECTRUM_BINS> &spectrum,
                           juce::Colour colour);
    void drawSpectrumLegend(juce::Graphics &g);
    void drawResponseCurve(juce::Graphics &g);
    void drawNodes(juce::Graphics &g);

    int findNodeAtPosition(juce::Point<float> position) const;
    juce::Point<float> getNodePosition(int bandIndex) const;
    void updateDraggedNode(juce::Point<float> position, NodeEditPhase phase);

    int selectedBandIndex = -1;
    int draggedBandIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyResponseDisplay)
};
