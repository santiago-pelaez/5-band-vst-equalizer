#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FiveBandEQProcessor::FiveBandEQProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts(*this, nullptr, "Parameters", ParameterHelper::createParameterLayout())
{
    // Initialize EQ bands with their respective positions
    for (int i = 0; i < EQConstants::NUM_BANDS; ++i)
    {
        eqBands[i] = std::make_unique<EQBand>(static_cast<BandPosition>(i));
    }
    
    // Set up parameter listeners
    for (int band = 0; band < EQConstants::NUM_BANDS; ++band)
    {
        juce::String prefix = "band" + juce::String(band + 1);
        apvts.addParameterListener(prefix + "_type", this);
        apvts.addParameterListener(prefix + "_freq", this);
        apvts.addParameterListener(prefix + "_gain", this);
        apvts.addParameterListener(prefix + "_q", this);
        apvts.addParameterListener(prefix + "_enabled", this);
    }
    
    apvts.addParameterListener("output_gain", this);
    apvts.addParameterListener("bypass", this);
}

FiveBandEQProcessor::~FiveBandEQProcessor()
{
}

//==============================================================================
const juce::String FiveBandEQProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FiveBandEQProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FiveBandEQProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FiveBandEQProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FiveBandEQProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FiveBandEQProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FiveBandEQProcessor::getCurrentProgram()
{
    return 0;
}

void FiveBandEQProcessor::setCurrentProgram (int index)
{
}

const juce::String FiveBandEQProcessor::getProgramName (int index)
{
    return {};
}

void FiveBandEQProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FiveBandEQProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialize all EQ bands
    for (auto& band : eqBands)
    {
        band->setSampleRate(sampleRate);
    }
    
    // Setup output gain smoothing
    outputGain.reset(sampleRate, 0.01); // 10ms smoothing
    outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(
        *apvts.getRawParameterValue("output_gain")));
    
    // Initialize all bands from current parameter values
    for (int i = 0; i < EQConstants::NUM_BANDS; ++i)
    {
        updateBandFromParameters(i);
    }
}

void FiveBandEQProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FiveBandEQProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // This is the place where you check if the layout is supported.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
}
#endif

void FiveBandEQProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // Check bypass
    if (bypassed.load())
        return;
        
    // Process each sample
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float left = leftChannel[sample];
        float right = rightChannel[sample];
        
        // Process through all 5 EQ bands in series
        for (auto& band : eqBands)
        {
            band->processStereo(left, right);
        }
        
        // Apply output gain
        float gain = outputGain.getNextValue();
        left *= gain;
        right *= gain;
        
        leftChannel[sample] = left;
        rightChannel[sample] = right;
    }
}

//==============================================================================
bool FiveBandEQProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FiveBandEQProcessor::createEditor()
{
    return new FiveBandEQProcessorEditor (*this);
}

//==============================================================================
void FiveBandEQProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FiveBandEQProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
void FiveBandEQProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "output_gain")
    {
        outputGain.setTargetValue(juce::Decibels::decibelsToGain(newValue));
    }
    else if (parameterID == "bypass")
    {
        bypassed.store(newValue > 0.5f);
    }
    else
    {
        // Handle band parameters
        for (int band = 0; band < EQConstants::NUM_BANDS; ++band)
        {
            juce::String prefix = "band" + juce::String(band + 1);
            if (parameterID.startsWith(prefix))
            {
                updateBandFromParameters(band);
                break;
            }
        }
    }
}

void FiveBandEQProcessor::updateBandFromParameters(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= EQConstants::NUM_BANDS)
        return;
        
    auto& band = eqBands[bandIndex];
    juce::String prefix = "band" + juce::String(bandIndex + 1);
    
    // Get parameter values
    int typeChoice = static_cast<int>(*apvts.getRawParameterValue(prefix + "_type"));
    float freq = *apvts.getRawParameterValue(prefix + "_freq");
    float gain = *apvts.getRawParameterValue(prefix + "_gain");
    float q = *apvts.getRawParameterValue(prefix + "_q");
    bool enabled = *apvts.getRawParameterValue(prefix + "_enabled") > 0.5f;
    
    // Update band
    FilterType filterType = getFilterTypeFromChoice(typeChoice, static_cast<BandPosition>(bandIndex));
    band->setFilterType(filterType);
    band->setFrequency(freq);
    band->setGain(gain);
    band->setQ(q);
    band->setEnabled(enabled);
}

FilterType FiveBandEQProcessor::getFilterTypeFromChoice(int choiceIndex, BandPosition position)
{
    auto availableTypes = FilterTypeRestrictions::getAvailableTypes(position);
    if (choiceIndex >= 0 && choiceIndex < static_cast<int>(availableTypes.size()))
        return availableTypes[choiceIndex];
    return FilterType::Disabled;
}

void FiveBandEQProcessor::getFrequencyResponse(std::vector<double>& frequencies, 
                                               std::vector<double>& magnitudes,
                                               int numPoints)
{
    // This would calculate the combined frequency response of all bands
    // Implementation for spectrum analyzer display
    frequencies.resize(numPoints);
    magnitudes.resize(numPoints);
    
    // Calculate logarithmic frequency points
    double logMin = std::log10(EQConstants::FREQUENCY_MIN);
    double logMax = std::log10(EQConstants::FREQUENCY_MAX);
    double logStep = (logMax - logMin) / (numPoints - 1);
    
    for (int i = 0; i < numPoints; ++i)
    {
        frequencies[i] = std::pow(10.0, logMin + i * logStep);
        magnitudes[i] = 1.0; // Would calculate combined response here
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FiveBandEQProcessor();
}