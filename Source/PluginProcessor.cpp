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

//==============================================================================
// MODE CONFIGURATIONS - All modes start identical
//==============================================================================
ClaritizerAudioProcessor::ModeConfig ClaritizerAudioProcessor::getModeConfig(int mode)
{
    if (useDebugConfigs)
    {
        return debugModeConfigs[mode];
    }
    
    // All modes start with same defaults (like original Mode A)
    ModeConfig config;
    
    // Chorus (bypassed initially - user can enable via debug sliders)
    config.chorus.timeMs = 30.0f;      // Good starting point for chorus
    config.chorus.feedback = 0.0f;      // No feedback initially
    config.chorus.modDepth = 0.0f;      // No modulation initially
    config.chorus.modRate = 0.0f;       // No LFO initially
    config.chorus.mix = 0.0f;           // BYPASSED - enable via sliders
    
    // Delay 1 (main delay - 250ms like original Mode A)
    config.delay1.baseTimeMs = 250.0f;
    config.delay1.feedback = 0.4f;
    config.delay1.modDepth = 0.0f;
    config.delay1.modRate = 0.0f;
    config.delay1.mix = 1.0f;           // Active
    config.delay1.reverse = false;
    
    // Delay 2 (muted initially)
    config.delay2.baseTimeMs = 100.0f;
    config.delay2.feedback = 0.0f;
    config.delay2.modDepth = 0.0f;
    config.delay2.modRate = 0.0f;
    config.delay2.mix = 0.0f;           // MUTED - enable via sliders
    config.delay2.reverse = false;
    
    // Reverb (good starting delays for diffusion network, bypassed initially)
    config.reverb.delay1Time = 37.0f;   // Prime numbers for good diffusion
    config.reverb.delay2Time = 83.0f;
    config.reverb.delay3Time = 127.0f;
    config.reverb.delay4Time = 211.0f;
    config.reverb.sharedFeedback = 0.0f; // No feedback initially
    config.reverb.mix = 0.0f;            // BYPASSED - enable via sliders
    
    return config;
}

//==============================================================================
// Safety limiter
//==============================================================================
float ClaritizerAudioProcessor::softClip(float sample)
{
    if (std::abs(sample) > 0.9f)
        return std::tanh(sample * 0.5f) * 1.2f;
    return sample;
}

//==============================================================================
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

//==============================================================================
void ClaritizerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Setup DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    
    // Setup all delay lines (5 seconds max - plenty of room)
    chorusLeft.prepare(sampleRate, 5.0f);
    chorusRight.prepare(sampleRate, 5.0f);
    delay1Left.prepare(sampleRate, 5.0f);
    delay1Right.prepare(sampleRate, 5.0f);
    delay2Left.prepare(sampleRate, 5.0f);
    delay2Right.prepare(sampleRate, 5.0f);
    reverb1Left.prepare(sampleRate, 5.0f);
    reverb1Right.prepare(sampleRate, 5.0f);
    reverb2Left.prepare(sampleRate, 5.0f);
    reverb2Right.prepare(sampleRate, 5.0f);
    reverb3Left.prepare(sampleRate, 5.0f);
    reverb3Right.prepare(sampleRate, 5.0f);
    reverb4Left.prepare(sampleRate, 5.0f);
    reverb4Right.prepare(sampleRate, 5.0f);
    
    // Setup all LFOs
    chorusLFOLeft.prepare(sampleRate);
    chorusLFORight.prepare(sampleRate);
    lfo1Left.prepare(sampleRate);
    lfo1Right.prepare(sampleRate);
    lfo2Left.prepare(sampleRate);
    lfo2Right.prepare(sampleRate);
    
    // Setup tone filter
    toneFilter.prepare(spec);
}

void ClaritizerAudioProcessor::releaseResources()
{
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

//==============================================================================
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
    
    // Set LFO frequencies
    chorusLFOLeft.setFrequency(config.chorus.modRate);
    chorusLFORight.setFrequency(config.chorus.modRate);
    lfo1Left.setFrequency(config.delay1.modRate);
    lfo1Right.setFrequency(config.delay1.modRate);
    lfo2Left.setFrequency(config.delay2.modRate);
    lfo2Right.setFrequency(config.delay2.modRate);
    
    // Create wet buffer for processing
    juce::AudioBuffer<float> wetBuffer;
    wetBuffer.makeCopyOf(buffer);
    
    // Process each sample
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // LEFT CHANNEL
        float input = wetBuffer.getSample(0, sample);
        
        // === CHORUS MODULE (series, pre) ===
        float chorusTime = (config.chorus.timeMs * timeScale / 1000.0f) * (float)getSampleRate();
        float chorusModDepth = (config.chorus.modDepth / 1000.0f) * (float)getSampleRate();
        float chorusLFO = chorusLFOLeft.getNextSample();
        float chorusDelay = juce::jmax(1.0f, chorusTime + (chorusLFO * chorusModDepth));
        
        float chorusDelayed = chorusLeft.readSample(chorusDelay);
        float chorusFeedback = juce::jlimit(0.0f, 0.90f, config.chorus.feedback);
        float chorusMixed = input + (chorusDelayed * chorusFeedback);
        chorusMixed = softClip(chorusMixed);
        chorusLeft.writeSample(chorusMixed);
        
        float chorusOutput = input * (1.0f - config.chorus.mix) + chorusMixed * config.chorus.mix;
        
        // === PARALLEL DELAYS (Delay 1 & 2) ===
        
        // Delay 1
        float delay1Time = (config.delay1.baseTimeMs * timeScale / 1000.0f) * (float)getSampleRate();
        float delay1ModDepth = (config.delay1.modDepth / 1000.0f) * (float)getSampleRate();
        float lfo1 = lfo1Left.getNextSample();
        float delay1 = juce::jmax(1.0f, delay1Time + (lfo1 * delay1ModDepth));
        
        float delayed1 = delay1Left.readSample(delay1);
        float feedback1 = juce::jlimit(0.0f, 0.90f, config.delay1.feedback);
        float mixed1 = chorusOutput + (delayed1 * feedback1);
        mixed1 = softClip(mixed1);
        delay1Left.writeSample(mixed1);
        float output1 = mixed1 * config.delay1.mix;
        
        // Delay 2
        float delay2Time = (config.delay2.baseTimeMs * timeScale / 1000.0f) * (float)getSampleRate();
        float delay2ModDepth = (config.delay2.modDepth / 1000.0f) * (float)getSampleRate();
        float lfo2 = lfo2Left.getNextSample();
        float delay2 = juce::jmax(1.0f, delay2Time + (lfo2 * delay2ModDepth));
        
        float delayed2 = delay2Left.readSample(delay2);
        float feedback2 = juce::jlimit(0.0f, 0.90f, config.delay2.feedback);
        float mixed2 = chorusOutput + (delayed2 * feedback2);
        mixed2 = softClip(mixed2);
        delay2Left.writeSample(mixed2);
        float output2 = mixed2 * config.delay2.mix;
        
        // Sum parallel delays
        float parallelSum = output1 + output2;
        
        // === REVERB MODULE (series diffusion network, post) ===
        float reverbFeedback = juce::jlimit(0.0f, 0.90f, config.reverb.sharedFeedback);
        
        // Delay 1
        float rev1Time = (config.reverb.delay1Time * timeScale / 1000.0f) * (float)getSampleRate();
        rev1Time = juce::jmax(1.0f, rev1Time);
        float rev1Delayed = reverb1Left.readSample(rev1Time);
        float rev1Mixed = parallelSum + (rev1Delayed * reverbFeedback);
        rev1Mixed = softClip(rev1Mixed);
        reverb1Left.writeSample(rev1Mixed);
        
        // Delay 2 (input is output of Delay 1)
        float rev2Time = (config.reverb.delay2Time * timeScale / 1000.0f) * (float)getSampleRate();
        rev2Time = juce::jmax(1.0f, rev2Time);
        float rev2Delayed = reverb2Left.readSample(rev2Time);
        float rev2Mixed = rev1Mixed + (rev2Delayed * reverbFeedback);
        rev2Mixed = softClip(rev2Mixed);
        reverb2Left.writeSample(rev2Mixed);
        
        // Delay 3 (input is output of Delay 2)
        float rev3Time = (config.reverb.delay3Time * timeScale / 1000.0f) * (float)getSampleRate();
        rev3Time = juce::jmax(1.0f, rev3Time);
        float rev3Delayed = reverb3Left.readSample(rev3Time);
        float rev3Mixed = rev2Mixed + (rev3Delayed * reverbFeedback);
        rev3Mixed = softClip(rev3Mixed);
        reverb3Left.writeSample(rev3Mixed);
        
        // Delay 4 (input is output of Delay 3)
        float rev4Time = (config.reverb.delay4Time * timeScale / 1000.0f) * (float)getSampleRate();
        rev4Time = juce::jmax(1.0f, rev4Time);
        float rev4Delayed = reverb4Left.readSample(rev4Time);
        float rev4Mixed = rev3Mixed + (rev4Delayed * reverbFeedback);
        rev4Mixed = softClip(rev4Mixed);
        reverb4Left.writeSample(rev4Mixed);
        
        // Mix reverb with dry parallel sum
        float reverbOutput = parallelSum * (1.0f - config.reverb.mix) + rev4Mixed * config.reverb.mix;
        
        wetBuffer.setSample(0, sample, reverbOutput);
        
        // RIGHT CHANNEL (same logic)
        input = wetBuffer.getSample(1, sample);
        
        // Chorus
        chorusLFO = chorusLFORight.getNextSample();
        chorusDelay = juce::jmax(1.0f, chorusTime + (chorusLFO * chorusModDepth));
        chorusDelayed = chorusRight.readSample(chorusDelay);
        chorusMixed = input + (chorusDelayed * chorusFeedback);
        chorusMixed = softClip(chorusMixed);
        chorusRight.writeSample(chorusMixed);
        chorusOutput = input * (1.0f - config.chorus.mix) + chorusMixed * config.chorus.mix;
        
        // Delay 1
        lfo1 = lfo1Right.getNextSample();
        delay1 = juce::jmax(1.0f, delay1Time + (lfo1 * delay1ModDepth));
        delayed1 = delay1Right.readSample(delay1);
        mixed1 = chorusOutput + (delayed1 * feedback1);
        mixed1 = softClip(mixed1);
        delay1Right.writeSample(mixed1);
        output1 = mixed1 * config.delay1.mix;
        
        // Delay 2
        lfo2 = lfo2Right.getNextSample();
        delay2 = juce::jmax(1.0f, delay2Time + (lfo2 * delay2ModDepth));
        delayed2 = delay2Right.readSample(delay2);
        mixed2 = chorusOutput + (delayed2 * feedback2);
        mixed2 = softClip(mixed2);
        delay2Right.writeSample(mixed2);
        output2 = mixed2 * config.delay2.mix;
        
        parallelSum = output1 + output2;
        
        // Reverb series
        rev1Delayed = reverb1Right.readSample(rev1Time);
        rev1Mixed = parallelSum + (rev1Delayed * reverbFeedback);
        rev1Mixed = softClip(rev1Mixed);
        reverb1Right.writeSample(rev1Mixed);
        
        rev2Delayed = reverb2Right.readSample(rev2Time);
        rev2Mixed = rev1Mixed + (rev2Delayed * reverbFeedback);
        rev2Mixed = softClip(rev2Mixed);
        reverb2Right.writeSample(rev2Mixed);
        
        rev3Delayed = reverb3Right.readSample(rev3Time);
        rev3Mixed = rev2Mixed + (rev3Delayed * reverbFeedback);
        rev3Mixed = softClip(rev3Mixed);
        reverb3Right.writeSample(rev3Mixed);
        
        rev4Delayed = reverb4Right.readSample(rev4Time);
        rev4Mixed = rev3Mixed + (rev4Delayed * reverbFeedback);
        rev4Mixed = softClip(rev4Mixed);
        reverb4Right.writeSample(rev4Mixed);
        
        reverbOutput = parallelSum * (1.0f - config.reverb.mix) + rev4Mixed * config.reverb.mix;
        
        wetBuffer.setSample(1, sample, reverbOutput);
    }
    
    // Apply tone filter
    float cutoffFreq = 200.0f + (toneValue * 18000.0f);
    *toneFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        getSampleRate(), cutoffFreq, 0.7f);
    
    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);
    toneFilter.process(wetContext);
    
    // Mix dry and wet with FINAL SAFETY LIMITING
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* dryData = buffer.getWritePointer(channel);
        auto* wetData = wetBuffer.getReadPointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float drySample = dryData[sample];
            float wetSample = wetData[sample];
            
            float output = drySample * (1.0f - dryWet) + wetSample * dryWet;
            
            // FINAL HARD LIMIT
            output = juce::jlimit(-1.0f, 1.0f, output);
            
            dryData[sample] = output;
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
