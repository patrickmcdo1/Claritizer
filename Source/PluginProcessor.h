#define JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING 1
#define JUCE_VST3_CAN_REPLACE_VST2 0

#pragma once

#include <JuceHeader.h>

class ClaritizerAudioProcessor : public juce::AudioProcessor
{
public:
    ClaritizerAudioProcessor();
    ~ClaritizerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    struct ModeConfig
    {
        float delayTimeBase;      // Base delay time in seconds
        float feedbackAmount;     // Delay feedback (0.0 - 0.8)
        float chorusDepth;        // Chorus modulation depth in ms
        float chorusRate;         // Chorus LFO rate in Hz
        float reverbSize;         // Reverb room size (0.0 - 1.0)
        float reverbDamping;      // Reverb damping (0.0 - 1.0)
        float reverbWet;          // Reverb wet amount (0.0 - 1.0)
        float bitCrushAmount;     // Bitcrush intensity (1 = none, 16 = heavy)
        float noiseModAmount;     // Noise modulation depth in ms
        float noiseModSpeed;      // Noise smoothing (0.99 = slow, 0.9999 = fast)
    };

    // Parameters
    juce::AudioProcessorValueTreeState parameters;
    
    // Debug mode config overrides (set by editor)
    ModeConfig debugModeConfigs[4];
    bool useDebugConfigs = false;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Parameter pointers
    std::atomic<float>* clarityParam = nullptr;
    std::atomic<float>* timeParam = nullptr;
    std::atomic<float>* toneParam = nullptr;
    std::atomic<float>* modeParam = nullptr;

    // Delay buffers (stereo)
    juce::AudioBuffer<float> delayBufferLeft;
    juce::AudioBuffer<float> delayBufferRight;
    int delayWritePosition = 0;
    
    // White noise generator for modulation
    juce::Random noiseGenerator;
    float smoothedNoiseLeft = 0.0f;
    float smoothedNoiseRight = 0.0f;
    
    // Chorus/modulation LFO
    float lfoPhase = 0.0f;
    float lfoPhaseRight = 0.0f;
    
    // Bitcrusher state
    float lastCrushedSampleLeft = 0.0f;
    float lastCrushedSampleRight = 0.0f;
    int crushCounterLeft = 0;
    int crushCounterRight = 0;
    
    // Reverb
    juce::dsp::Reverb reverb;
    juce::dsp::ProcessSpec spec;
    
    // Filters for tone control
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highPassFilter;

    // Mode configuration structure
    
    

    
    // Helper methods
    ModeConfig getModeConfig(int mode);
    float getModulatedDelayTime(float baseTime, float noiseValue, float lfoValue, const ModeConfig& config);
    float applyBitcrush(float sample, float& lastCrushedSample, int& counter, int crushRate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaritizerAudioProcessor)
};
