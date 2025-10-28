#include "PluginProcessor.h"
#include "PluginEditor.h"

ClaritizerAudioProcessor::ClaritizerAudioProcessor()
    : AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput("Input", juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                     ),
      parameters(*this, nullptr, juce::Identifier("PARAMETERS"), createParameterLayout())
{
    clarityParam = parameters.getRawParameterValue("clarity");
    timeParam = parameters.getRawParameterValue("time");
    toneParam = parameters.getRawParameterValue("tone");
    modeParam = parameters.getRawParameterValue("mode");
}

ClaritizerAudioProcessor::~ClaritizerAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ClaritizerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("clarity", 1),
        "Clarity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("time", 1),
        "Time",
        juce::NormalisableRange<float>(0.1f, 3.0f, 0.01f),
        1.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("tone", 1),
        "Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mode", 1),
        "Mode",
        juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f),
        0.0f));
    
    return layout;
}

const juce::String ClaritizerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ClaritizerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ClaritizerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ClaritizerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ClaritizerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ClaritizerAudioProcessor::getNumPrograms()
{
    return 1;
}

int ClaritizerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ClaritizerAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String ClaritizerAudioProcessor::getProgramName(int index)
{
    return {};
}

void ClaritizerAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void ClaritizerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Setup for reverb and filter
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    
    // Setup reverb with lofi settings
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.7f;
    reverbParams.damping = 0.6f;
    reverbParams.wetLevel = 0.3f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);
    reverb.prepare(spec);
    
    // Setup delay buffers (2 seconds max)
    int maxDelaySize = (int)(2.0 * sampleRate);
    delayBufferLeft.setSize(1, maxDelaySize);
    delayBufferRight.setSize(1, maxDelaySize);
    delayBufferLeft.clear();
    delayBufferRight.clear();
    delayWritePosition = 0;
    
    // Setup filter
    lowPassFilter.prepare(spec);
}

void ClaritizerAudioProcessor::releaseResources()
{
    delayBufferLeft.setSize(0, 0);
    delayBufferRight.setSize(0, 0);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ClaritizerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ClaritizerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get parameter values
    float dryWet = clarityParam->load();
    float timeScale = timeParam->load();
    float toneValue = toneParam->load();
    int mode = (int)(modeParam->load());
    
    // Only process Mode A for now - B, C, D do nothing
    if (mode == 0)
    {
        // Create a copy for wet processing
        juce::AudioBuffer<float> wetBuffer;
        wetBuffer.makeCopyOf(buffer);
        
        // Process delay
        int delayTimeInSamples = (int)(0.375 * getSampleRate() * timeScale); // Base 375ms delay
        delayTimeInSamples = juce::jmin(delayTimeInSamples, delayBufferLeft.getNumSamples() - 1);
        
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = wetBuffer.getWritePointer(channel);
            auto* delayData = (channel == 0) ? delayBufferLeft.getWritePointer(0) : delayBufferRight.getWritePointer(0);
            int delayBufferSize = (channel == 0) ? delayBufferLeft.getNumSamples() : delayBufferRight.getNumSamples();
            
            for (int sample = 0; sample < wetBuffer.getNumSamples(); ++sample)
            {
                // Read from delay buffer
                int readPos = delayWritePosition - delayTimeInSamples;
                if (readPos < 0)
                    readPos += delayBufferSize;
                
                float delayedSample = delayData[readPos];
                
                // Mix input with delayed signal (with feedback)
                float inputSample = channelData[sample];
                float outputSample = inputSample + (delayedSample * 0.4f); // 40% feedback
                
                // Write to delay buffer
                delayData[delayWritePosition] = inputSample + (delayedSample * 0.3f);
                
                // Output
                channelData[sample] = outputSample;
                
                // Increment write position (only once per sample)
                if (channel == totalNumInputChannels - 1)
                {
                    delayWritePosition++;
                    if (delayWritePosition >= delayBufferSize)
                        delayWritePosition = 0;
                }
            }
        }
        
        // Apply reverb to wet buffer
        juce::dsp::AudioBlock<float> block(wetBuffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        reverb.process(context);
        
        // Apply tone filter
        *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            getSampleRate(),
            200.0f + (toneValue * 18000.0f), // 200Hz to 18.2kHz
            0.7f);
        lowPassFilter.process(context);
        
        // Mix dry and wet
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* dryData = buffer.getWritePointer(channel);
            auto* wetData = wetBuffer.getReadPointer(channel);
            
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                dryData[sample] = dryData[sample] * (1.0f - dryWet) + wetData[sample] * dryWet;
            }
        }
    }
    // Modes B, C, D do nothing - just pass audio through
}

bool ClaritizerAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ClaritizerAudioProcessor::createEditor()
{
    return new ClaritizerAudioProcessorEditor(*this);
}

void ClaritizerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ClaritizerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClaritizerAudioProcessor();
}
