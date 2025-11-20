#define JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING 1
#define JUCE_VST3_CAN_REPLACE_VST2 0

#pragma once

#include <JuceHeader.h>

//==============================================================================
// Delay Line - Simple circular buffer with interpolation
//==============================================================================
class DelayLine
{
public:
    void prepare(double sampleRate, float maxDelaySeconds)
    {
        int bufferSize = (int)(sampleRate * maxDelaySeconds) + 1;
        buffer.setSize(1, bufferSize);
        buffer.clear();
        writePosition = 0;
        this->sampleRate = sampleRate;
    }
    
    void clear()
    {
        buffer.clear();
        writePosition = 0;
    }
    
    void writeSample(float sample)
    {
        buffer.setSample(0, writePosition, sample);
        writePosition = (writePosition + 1) % buffer.getNumSamples();
    }
    
    // Read with linear interpolation
    float readSample(float delayInSamples)
    {
        float readPos = writePosition - delayInSamples;
        while (readPos < 0)
            readPos += buffer.getNumSamples();
        
        int readPos1 = (int)readPos;
        int readPos2 = (readPos1 + 1) % buffer.getNumSamples();
        float frac = readPos - readPos1;
        
        float sample1 = buffer.getSample(0, readPos1);
        float sample2 = buffer.getSample(0, readPos2);
        
        return sample1 + frac * (sample2 - sample1);
    }
    
private:
    juce::AudioBuffer<float> buffer;
    int writePosition = 0;
    double sampleRate = 44100.0;
};

//==============================================================================
// Simple LFO for modulation
//==============================================================================
class SimpleLFO
{
public:
    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        phase = 0.0f;
    }
    
    void setFrequency(float hz)
    {
        increment = (hz * juce::MathConstants<float>::twoPi) / (float)sampleRate;
    }
    
    float getNextSample()
    {
        float value = std::sin(phase);
        phase += increment;
        if (phase >= juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;
        return value;
    }
    
private:
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float increment = 0.0f;
};

//==============================================================================
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
    
    // Chorus module configuration (series, pre-delays)
    struct ChorusConfig
    {
        float timeMs;           // Chorus delay time (10-50ms typical)
        float feedback;         // Light feedback (0.0-0.3)
        float modDepth;         // LFO modulation depth in ms
        float modRate;          // LFO rate in Hz
        float mix;              // Chorus wet amount
    };
    
    // Main delay configuration (parallel)
    struct DelayConfig
    {
        float baseTimeMs;       // Base delay time (before TIME knob scaling)
        float feedback;         // Feedback amount (0.0 - 0.95)
        float modDepth;         // LFO modulation depth in ms
        float modRate;          // LFO rate in Hz
        float mix;              // Output mix (0.0 = muted, 1.0 = full)
        bool reverse;           // Reverse delay effect (placeholder for now)
    };
    
    // Reverb module configuration (series diffusion network, post-delays)
    struct ReverbConfig
    {
        float delay1Time;       // First delay time in ms
        float delay2Time;       // Second delay time in ms
        float delay3Time;       // Third delay time in ms
        float delay4Time;       // Fourth delay time in ms
        float sharedFeedback;   // Shared feedback for all 4 delays
        float mix;              // Reverb wet/dry mix
    };
    
    // Complete mode configuration
    struct ModeConfig
    {
        ChorusConfig chorus;
        DelayConfig delay1;
        DelayConfig delay2;
        ReverbConfig reverb;
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

    // Chorus module (stereo)
    DelayLine chorusLeft, chorusRight;
    SimpleLFO chorusLFOLeft, chorusLFORight;
    
    // 2 parallel main delays (stereo)
    DelayLine delay1Left, delay1Right;
    DelayLine delay2Left, delay2Right;
    SimpleLFO lfo1Left, lfo1Right;
    SimpleLFO lfo2Left, lfo2Right;
    
    // Reverb module - 4 series delays (stereo)
    DelayLine reverb1Left, reverb1Right;
    DelayLine reverb2Left, reverb2Right;
    DelayLine reverb3Left, reverb3Right;
    DelayLine reverb4Left, reverb4Right;
    
    // Tone filter
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> toneFilter;
    juce::dsp::ProcessSpec spec;

    // Helper methods
    ModeConfig getModeConfig(int mode);
    float softClip(float sample);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaritizerAudioProcessor)
};
