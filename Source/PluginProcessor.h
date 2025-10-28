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

    // Parameters
    juce::AudioProcessorValueTreeState parameters;

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
    
    // Reverb
    juce::dsp::Reverb reverb;
    juce::dsp::ProcessSpec spec;
    
    // Filter for tone control
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaritizerAudioProcessor)
};
