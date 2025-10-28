#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ClaritizerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ClaritizerLookAndFeel()
     {
        setColour(juce::Slider::thumbColourId, juce::Colours::white);
        setColour(juce::Slider::trackColourId, juce::Colour(0xff9dc3e6));
        setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    }
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override
    {
        // Don't draw anything - we'll handle it in paint()
    }
    
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        // Don't draw anything - we'll handle it in paint()
    }
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        
        // Draw border - we'll use a fixed thickness here
        g.setColour(juce::Colour(0xff9dc3e6));
        g.drawRect(bounds, buttonBorderThickness);
        
        // Fill if toggled
        if (button.getToggleState())
        {
            g.setColour(juce::Colours::black);
            g.fillRect(bounds.reduced(buttonBorderThickness));
        }
    }
    
    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                       bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        g.setFont(juce::Font("Times New Roman", buttonFontSize, juce::Font::bold));
        g.setColour(button.getToggleState() ? juce::Colour(0xff9dc3e6) : juce::Colours::white);
        g.drawText(button.getButtonText(), button.getLocalBounds(),
                  juce::Justification::centred, true);
    }
    
    // Public members that can be set from the editor
    float buttonBorderThickness = 4.0f;
    float buttonFontSize = 36.0f;
};

class ClaritizerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    ClaritizerAudioProcessorEditor (ClaritizerAudioProcessor&);
    ~ClaritizerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    ClaritizerAudioProcessor& audioProcessor;
    ClaritizerLookAndFeel customLookAndFeel;
    
    // GUI Components
    juce::Slider claritySlider;
    juce::Slider timeKnob;
    juce::Slider toneKnob;
    juce::TextButton modeAButton;
    juce::TextButton modeBButton;
    juce::TextButton modeCButton;
    juce::TextButton modeDButton;
    
    // DEBUG SLIDERS - Set showDebug to false when done tweaking!
    bool showDebug = true;
    juce::Slider debugClarityBarY, debugClarityBarHeight;
    juce::Slider debugTitleFontSize, debugLabelFontSize;
    juce::Label debugLabel23, debugLabel24;
    juce::Slider debugClarityX, debugClarityY, debugClarityW, debugClarityH;
    juce::Slider debugClarityThumbW, debugClarityThumbH;
    juce::Slider debugTimeKnobX, debugTimeKnobY, debugKnobSize, debugKnobSpacing;
    juce::Slider debugToneKnobX, debugKnobBorderThickness, debugKnobArcThickness;
    juce::Slider debugButtonX, debugButtonY, debugButtonW, debugButtonH, debugButtonSpacing;
    juce::Slider debugButtonBorderThickness, debugButtonFontSize;  // ADD THIS LINE
    juce::Label debugLabel1, debugLabel2, debugLabel3, debugLabel4, debugLabel5;
    juce::Label debugLabel6, debugLabel7, debugLabel8, debugLabel9, debugLabel10;
    juce::Label debugLabel11, debugLabel12, debugLabel13, debugLabel14, debugLabel15;
    juce::Label debugLabel16, debugLabel17, debugLabel18, debugLabel19, debugLabel20;
    juce::Label debugLabel21, debugLabel22;
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clarityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;
    
    // Noise animation
    juce::Image noiseFrames[4];
    int currentNoiseFrame = 0;
    
    juce::Colour modeColors[4] = {
        juce::Colour(0xff7ba5d1),  // Mode A
        juce::Colour(0xff7ba5d1),  // Mode B
        juce::Colour(0xff7ba5d1),  // Mode C
        juce::Colour(0xff7ba5d1)   // Mode D
    };

    float noiseOpacity = 0.05f;  // Default noise opacity

    // For the mode slider
    juce::Slider modeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modeAttachment;

    // Function declarations
    void generateNoiseTextures();
    void drawNoiseTexture(juce::Graphics& g, juce::Rectangle<int> bounds);
    
    // Layout bounds
    juce::Rectangle<int> claritySliderBounds;
    juce::Rectangle<int> timeKnobBounds;
    juce::Rectangle<int> toneKnobBounds;
    
    int currentMode = 0;
    
    void setupGui();
    void modeButtonClicked(int mode);
    void drawKnob(juce::Graphics& g, juce::Rectangle<int> bounds, float value, const juce::String& label);
    void drawClaritySlider(juce::Graphics& g, juce::Rectangle<int> bounds, float value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaritizerAudioProcessorEditor)
};
