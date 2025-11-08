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
    
    // Initialize LFO phases with stereo offset
    lfoPhaseRight = juce::MathConstants<float>::pi * 0.5f; // 90 degrees offset for stereo width
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

// MODE CONFIGURATIONS
ClaritizerAudioProcessor::ModeConfig ClaritizerAudioProcessor::getModeConfig(int mode)
{
    if (useDebugConfigs)
        {
            return debugModeConfigs[mode];
        }
    switch (mode)
    {
        case 0: // Mode A - Gentle/Subtle
            return {
                0.25f,      // delayTimeBase: 250ms
                0.35f,      // feedbackAmount: gentle repeats
                3.0f,       // chorusDepth: 3ms modulation
                0.5f,       // chorusRate: 0.5 Hz (slow)
                0.4f,       // reverbSize: small room
                0.6f,       // reverbDamping: moderate
                0.25f,      // reverbWet: subtle
                2.0f,       // bitCrushAmount: minimal (almost none)
                2.0f,       // noiseModAmount: 2ms noise wobble
                0.9995f     // noiseModSpeed: slow smooth noise
            };
            
        case 1: // Mode B - Vibrant/Energetic
            return {
                0.35f,      // delayTimeBase: 350ms
                0.50f,      // feedbackAmount: more repeats
                5.0f,       // chorusDepth: 5ms modulation
                1.5f,       // chorusRate: 1.5 Hz (medium-fast)
                0.5f,       // reverbSize: medium room
                0.5f,       // reverbDamping: bright
                0.35f,      // reverbWet: moderate
                4.0f,       // bitCrushAmount: moderate texture
                4.0f,       // noiseModAmount: 4ms noise wobble
                0.999f      // noiseModSpeed: medium-fast noise
            };
            
        case 2: // Mode C - Lush/Dreamy
            return {
                0.50f,      // delayTimeBase: 500ms
                0.45f,      // feedbackAmount: sustained but not runaway
                8.0f,       // chorusDepth: 8ms deep modulation
                0.3f,       // chorusRate: 0.3 Hz (very slow)
                0.75f,      // reverbSize: large hall
                0.7f,       // reverbDamping: darker/warmer
                0.50f,      // reverbWet: heavy reverb
                2.0f,       // bitCrushAmount: minimal (keep it clean)
                6.0f,       // noiseModAmount: 6ms heavy drift
                0.9998f     // noiseModSpeed: very slow smooth drift
            };
            
        case 3: // Mode D - Lo-Fi/Vintage
            return {
                0.375f,     // delayTimeBase: 375ms (classic tape delay)
                0.40f,      // feedbackAmount: vintage tape repeats
                2.0f,       // chorusDepth: 2ms subtle warble
                0.8f,       // chorusRate: 0.8 Hz
                0.70f,      // reverbSize: medium-large room
                0.6f,       // reverbDamping: moderate
                0.30f,      // reverbWet: moderate reverb
                8.0f,       // bitCrushAmount: heavy lo-fi texture
                5.0f,       // noiseModAmount: 5ms tape wobble
                0.997f      // noiseModSpeed: faster noise for tape flutter
            };
            
        default:
            return getModeConfig(0);
    }
}

// Helper: Calculate modulated delay time
float ClaritizerAudioProcessor::getModulatedDelayTime(float baseTime, float noiseValue,
                                                       float lfoValue, const ModeConfig& config)
{
    // Combine noise modulation and LFO modulation
    float noiseOffset = noiseValue * config.noiseModAmount * 0.001f; // Convert ms to seconds
    float lfoOffset = lfoValue * config.chorusDepth * 0.001f; // Convert ms to seconds
    
    return baseTime + noiseOffset + lfoOffset;
}

// Helper: Apply bitcrushing effect
float ClaritizerAudioProcessor::applyBitcrush(float sample, float& lastCrushedSample,
                                               int& counter, int crushRate)
{
    if (crushRate <= 1)
        return sample; // No crushing
    
    // Sample rate reduction
    counter++;
    if (counter >= crushRate)
    {
        counter = 0;
        
        // Bit depth reduction (quantization)
        int bits = juce::jmax(1, 16 - (int)(crushRate * 0.5f)); // Reduce bits as crush increases
        float steps = std::pow(2.0f, bits);
        lastCrushedSample = std::floor(sample * steps) / steps;
    }
    
    return lastCrushedSample;
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
    // Setup DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    
    // Setup reverb with default parameters (will be updated per mode)
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 0.3f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);
    reverb.prepare(spec);
    
    // Setup delay buffers (3 seconds max to accommodate all modes and modulation)
    int maxDelaySize = (int)(3.0 * sampleRate);
    delayBufferLeft.setSize(1, maxDelaySize);
    delayBufferRight.setSize(1, maxDelaySize);
    delayBufferLeft.clear();
    delayBufferRight.clear();
    delayWritePosition = 0;
    
    // Setup filters
    lowPassFilter.prepare(spec);
    highPassFilter.prepare(spec);
    
    // Reset modulation state
    lfoPhase = 0.0f;
    lfoPhaseRight = juce::MathConstants<float>::pi * 0.5f;
    smoothedNoiseLeft = 0.0f;
    smoothedNoiseRight = 0.0f;
    crushCounterLeft = 0;
    crushCounterRight = 0;
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
    
    // Get mode configuration
    ModeConfig config = getModeConfig(mode);
    
    // Calculate delay time based on mode and time knob
    float baseDelayTime = config.delayTimeBase * timeScale;
    int baseDelayInSamples = (int)(baseDelayTime * getSampleRate());
    
    // Update reverb parameters for this mode
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = config.reverbSize;
    reverbParams.damping = config.reverbDamping;
    reverbParams.wetLevel = config.reverbWet;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);
    
    // Calculate LFO increment
    float lfoIncrement = (config.chorusRate * 2.0f * juce::MathConstants<float>::pi) / (float)getSampleRate();
    
    // Create a copy for wet processing
    juce::AudioBuffer<float> wetBuffer;
    wetBuffer.makeCopyOf(buffer);
    
    // Process each sample
    for (int sample = 0; sample < wetBuffer.getNumSamples(); ++sample)
    {
        // Update noise modulation (smooth random walk)
        float newNoiseLeft = (noiseGenerator.nextFloat() * 2.0f - 1.0f); // -1 to 1
        float newNoiseRight = (noiseGenerator.nextFloat() * 2.0f - 1.0f);
        smoothedNoiseLeft = smoothedNoiseLeft * config.noiseModSpeed + newNoiseLeft * (1.0f - config.noiseModSpeed);
        smoothedNoiseRight = smoothedNoiseRight * config.noiseModSpeed + newNoiseRight * (1.0f - config.noiseModSpeed);
        
        // Update LFO for chorus
        float lfoValueLeft = std::sin(lfoPhase);
        float lfoValueRight = std::sin(lfoPhaseRight);
        lfoPhase += lfoIncrement;
        lfoPhaseRight += lfoIncrement;
        if (lfoPhase >= juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;
        if (lfoPhaseRight >= juce::MathConstants<float>::twoPi)
            lfoPhaseRight -= juce::MathConstants<float>::twoPi;
        
        // Process each channel
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = wetBuffer.getWritePointer(channel);
            auto* delayData = (channel == 0) ? delayBufferLeft.getWritePointer(0) : delayBufferRight.getWritePointer(0);
            int delayBufferSize = (channel == 0) ? delayBufferLeft.getNumSamples() : delayBufferRight.getNumSamples();
            
            // Get modulated delay time
            float noise = (channel == 0) ? smoothedNoiseLeft : smoothedNoiseRight;
            float lfo = (channel == 0) ? lfoValueLeft : lfoValueRight;
            float modulatedDelayTime = getModulatedDelayTime(baseDelayTime, noise, lfo, config);
            int modulatedDelayInSamples = juce::jlimit(1, delayBufferSize - 1,
                                                        (int)(modulatedDelayTime * getSampleRate()));
            
            // Read from delay buffer with interpolation for smooth modulation
            int readPos = delayWritePosition - modulatedDelayInSamples;
            if (readPos < 0)
                readPos += delayBufferSize;
            
            int readPosNext = readPos + 1;
            if (readPosNext >= delayBufferSize)
                readPosNext -= delayBufferSize;
            
            float frac = (modulatedDelayTime * getSampleRate()) - modulatedDelayInSamples;
            float delayedSample = delayData[readPos] * (1.0f - frac) + delayData[readPosNext] * frac;
            
            // Mix input with delayed signal (with feedback)
            float inputSample = channelData[sample];
            float outputSample = inputSample + (delayedSample * config.feedbackAmount);
            
            // Write to delay buffer
            delayData[delayWritePosition] = inputSample + (delayedSample * config.feedbackAmount * 0.7f);
            
            // Apply to output
            channelData[sample] = outputSample;
        }
        
        // Increment write position once per sample (after processing all channels)
        delayWritePosition++;
        if (delayWritePosition >= delayBufferLeft.getNumSamples())
            delayWritePosition = 0;
    }
    
    // Apply bitcrushing
    int crushRate = (int)config.bitCrushAmount;
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = wetBuffer.getWritePointer(channel);
        float& lastCrushed = (channel == 0) ? lastCrushedSampleLeft : lastCrushedSampleRight;
        int& counter = (channel == 0) ? crushCounterLeft : crushCounterRight;
        
        for (int sample = 0; sample < wetBuffer.getNumSamples(); ++sample)
        {
            channelData[sample] = applyBitcrush(channelData[sample], lastCrushed, counter, crushRate);
        }
    }
    
    // Apply reverb to wet buffer
    juce::dsp::AudioBlock<float> block(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
    
    // Apply tone filtering (crossfade between low-pass and high-pass)
    // toneValue: 0.0 = dark (low-pass), 0.5 = neutral, 1.0 = bright (high-pass)
    if (toneValue < 0.5f)
    {
        // Dark side - apply low-pass filter
        float cutoff = 200.0f + (toneValue * 2.0f * 18000.0f); // 200Hz to 18.2kHz
        *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            getSampleRate(), cutoff, 0.7f);
        lowPassFilter.process(context);
    }
    else
    {
        // Bright side - apply high-pass filter
        float cutoff = 20.0f + ((toneValue - 0.5f) * 2.0f * 800.0f); // 20Hz to 820Hz
        *highPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            getSampleRate(), cutoff, 0.7f);
        highPassFilter.process(context);
    }
    
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
