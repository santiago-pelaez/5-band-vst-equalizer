#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FiveBandEQProcessor::FiveBandEQProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", ParameterHelper::createParameterLayout())
{
    // Initialize EQ bands with their respective positions
    for (int i = 0; i < EQConstants::NUM_BANDS; ++i)
    {
        eqBands[i] = std::make_unique<EQBand>(static_cast<BandPosition>(i));
    }

    // Cache raw parameter pointers once during construction. The audio thread
    // can then synchronize values without constructing parameter-ID strings.
    for (int band = 0; band < EQConstants::NUM_BANDS; ++band)
    {
        const juce::String prefix = "band" + juce::String(band + 1);
        auto &pointers = bandParameterPointers[band];

        pointers.type = apvts.getRawParameterValue(prefix + "_type");
        pointers.frequency = apvts.getRawParameterValue(prefix + "_freq");
        pointers.gain = apvts.getRawParameterValue(prefix + "_gain");
        pointers.q = apvts.getRawParameterValue(prefix + "_q");
        pointers.enabled = apvts.getRawParameterValue(prefix + "_enabled");
    }

    outputGainParameter = apvts.getRawParameterValue("output_gain");
    bypassParameter = apvts.getRawParameterValue("bypass");

    // Parameter listeners only mark a pending synchronization. DSP state is
    // updated by the audio thread at the beginning of the next audio block.
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
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int FiveBandEQProcessor::getCurrentProgram()
{
    return 0;
}

void FiveBandEQProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String FiveBandEQProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void FiveBandEQProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void FiveBandEQProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    // Initialize all EQ bands
    for (auto &band : eqBands)
    {
        band->setSampleRate(sampleRate);
    }

    // Set up output gain smoothing
    outputGain.reset(sampleRate, 0.01); // 10ms smoothing
    outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputGainParameter->load()));

    bypassMix.reset(sampleRate, 0.01);
    bypassMix.setCurrentAndTargetValue(bypassParameter->load() > 0.5f ? 1.0f : 0.0f);

    parametersNeedSynchronization.store(true);
    synchronizeParametersFromHost();
}

void FiveBandEQProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FiveBandEQProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    // This is the place where you check if the layout is supported.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
}
#endif

void FiveBandEQProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    synchronizeParametersFromHost();

    if (totalNumInputChannels == 0)
        return;

    auto *leftChannel = buffer.getWritePointer(0);
    auto *rightChannel = totalNumInputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float left = leftChannel[sample];
        const float dryLeft = left;
        float right = rightChannel != nullptr ? rightChannel[sample] : 0.0f;
        const float dryRight = right;

        if (rightChannel == nullptr)
        {
            for (auto &band : eqBands)
                band->processMono(left);
        }
        else
        {
            for (auto &band : eqBands)
                band->processStereo(left, right);
        }

        float gain = outputGain.getNextValue();
        left *= gain;
        right *= gain;

        const float wetMix = 1.0f - bypassMix.getNextValue();
        left = dryLeft + (left - dryLeft) * wetMix;
        right = dryRight + (right - dryRight) * wetMix;

        leftChannel[sample] = left;

        if (rightChannel != nullptr)
            rightChannel[sample] = right;
    }
}

//==============================================================================
bool FiveBandEQProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *FiveBandEQProcessor::createEditor()
{
    return new FiveBandEQProcessorEditor(*this);
}

//==============================================================================
void FiveBandEQProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FiveBandEQProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void FiveBandEQProcessor::parameterChanged(const juce::String &parameterID, float newValue)
{
    juce::ignoreUnused(parameterID, newValue);
    parametersNeedSynchronization.store(true);
}

void FiveBandEQProcessor::synchronizeParametersFromHost()
{
    if (! parametersNeedSynchronization.exchange(false))
        return;

    for (int band = 0; band < EQConstants::NUM_BANDS; ++band)
        updateBandFromParameters(band);

    const float outputGainInDecibels = outputGainParameter->load();
    outputGain.setTargetValue(juce::Decibels::decibelsToGain(outputGainInDecibels));

    const bool hostRequestedBypass = bypassParameter->load() > 0.5f;
    bypassMix.setTargetValue(hostRequestedBypass ? 1.0f : 0.0f);
}

void FiveBandEQProcessor::updateBandFromParameters(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= EQConstants::NUM_BANDS)
        return;

    auto &band = eqBands[bandIndex];
    const auto &parameters = bandParameterPointers[bandIndex];

    if (parameters.type == nullptr || parameters.frequency == nullptr
        || parameters.gain == nullptr || parameters.q == nullptr
        || parameters.enabled == nullptr)
        return;

    // Read only cached atomic values from the audio thread.
    const int typeChoice = static_cast<int>(parameters.type->load());
    const float frequency = parameters.frequency->load();
    const float gain = parameters.gain->load();
    const float q = parameters.q->load();
    const bool enabled = parameters.enabled->load() > 0.5f;

    // Update band
    FilterType filterType = getFilterTypeFromChoice(typeChoice, static_cast<BandPosition>(bandIndex));
    band->setFilterType(filterType);
    band->setFrequency(frequency);
    band->setGain(gain);
    band->setQ(q);
    band->setEnabled(enabled);
}

FilterType FiveBandEQProcessor::getFilterTypeFromChoice(int choiceIndex, BandPosition position)
{
    if (choiceIndex < 0)
        return FilterType::Disabled;

    if (position == BandPosition::Low)
    {
        if (choiceIndex == 0)
            return FilterType::LowCut;
        if (choiceIndex == 1)
            return FilterType::LowShelf;
        if (choiceIndex == 2)
            return FilterType::Disabled;
        return FilterType::Disabled;
    }

    if (position == BandPosition::High)
    {
        if (choiceIndex == 0)
            return FilterType::HighCut;
        if (choiceIndex == 1)
            return FilterType::HighShelf;
        if (choiceIndex == 2)
            return FilterType::Disabled;
        return FilterType::Disabled;
    }

    if (choiceIndex == 0)
        return FilterType::Peak;
    if (choiceIndex == 1)
        return FilterType::LowShelf;
    if (choiceIndex == 2)
        return FilterType::HighShelf;
    if (choiceIndex == 3)
        return FilterType::Disabled;

    return FilterType::Disabled;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new FiveBandEQProcessor();
}
