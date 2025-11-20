#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TransparentSlider : public juce::Slider
{
public:
    void paint(juce::Graphics& g) override
    {
        // Intentionally empty - no rendering
    }
};

class TransparentButton : public juce::TextButton
{
public:
    void paint(juce::Graphics& g) override
    {
        getLookAndFeel().drawButtonBackground(g, *this,
            findColour(buttonColourId),
            isOver(), isDown());
        getLookAndFeel().drawButtonText(g, *this,
            isOver(), isDown());
    }
};

class ClaritizerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ClaritizerLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colours::white);
        setColour(juce::Slider::trackColourId, juce::Colour(0xff9dc3e6));
        setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    }
    
    void setModeColor(juce::Colour color)
    {
        currentModeColor = color;
    }
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override
    {
        // Don't draw anything
    }
    
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        // Don't draw anything
    }
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        
        juce::ColourGradient borderGradient(
            juce::Colours::white, bounds.getX(), bounds.getY(),
            currentModeColor, bounds.getX(), bounds.getBottom(),
            false);
        g.setGradientFill(borderGradient);
        g.drawRect(bounds, buttonBorderThickness);
        
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
        
        auto bounds = button.getLocalBounds();
        if (button.getToggleState())
        {
            juce::ColourGradient textGradient(
                juce::Colours::white, bounds.getCentreX(), bounds.getY(),
                currentModeColor, bounds.getCentreX(), bounds.getBottom(),
                false);
            g.setGradientFill(textGradient);
        }
        else
        {
            g.setColour(juce::Colours::white);
        }
        
        g.drawText(button.getButtonText(), bounds,
                  juce::Justification::centred, true);
    }
    
    float buttonBorderThickness = 4.0f;
    float buttonFontSize = 36.0f;
    
private:
    juce::Colour currentModeColor = juce::Colour(0xff7ba5d1);
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
    
    juce::Viewport debugViewport;
    juce::Component debugContainer;
    
    TransparentSlider claritySlider;
    TransparentSlider timeKnob;
    TransparentSlider toneKnob;
    TransparentButton modeAButton;
    TransparentButton modeBButton;
    TransparentButton modeCButton;
    TransparentButton modeDButton;
    
    bool showDebug = true;
    
    // MODE A DEBUG SLIDERS - NEW ARCHITECTURE (23 total)
    // Chorus (5 params)
    juce::Slider debugModeA_ChorusTime, debugModeA_ChorusFeedback, debugModeA_ChorusModDepth;
    juce::Slider debugModeA_ChorusModRate, debugModeA_ChorusMix;
    juce::Label debugLabelA1, debugLabelA2, debugLabelA3, debugLabelA4, debugLabelA5;
    
    // Delay 1 (6 params)
    juce::Slider debugModeA_D1Time, debugModeA_D1Feedback, debugModeA_D1ModDepth;
    juce::Slider debugModeA_D1ModRate, debugModeA_D1Mix, debugModeA_D1Reverse;
    juce::Label debugLabelA6, debugLabelA7, debugLabelA8, debugLabelA9, debugLabelA10, debugLabelA11;
    
    // Delay 2 (6 params)
    juce::Slider debugModeA_D2Time, debugModeA_D2Feedback, debugModeA_D2ModDepth;
    juce::Slider debugModeA_D2ModRate, debugModeA_D2Mix, debugModeA_D2Reverse;
    juce::Label debugLabelA12, debugLabelA13, debugLabelA14, debugLabelA15, debugLabelA16, debugLabelA17;
    
    // Reverb (6 params)
    juce::Slider debugModeA_Rev1Time, debugModeA_Rev2Time, debugModeA_Rev3Time;
    juce::Slider debugModeA_Rev4Time, debugModeA_RevFeedback, debugModeA_RevMix;
    juce::Label debugLabelA18, debugLabelA19, debugLabelA20, debugLabelA21, debugLabelA22, debugLabelA23;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clarityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;
    
    juce::Image noiseFrame;
    
    juce::Colour modeColors[4] = {
        juce::Colour(0xff7ba5d1),  // Mode A: Blue
        juce::Colour(0xffff3333),  // Mode B: Bright Red
        juce::Colour(0xff33ff33),  // Mode C: Bright Green
        juce::Colour(0xffffdd33)   // Mode D: Bright Yellow
    };

    float noiseOpacity = 0.1f;
    int noisePixelSize = 1;

    juce::Slider modeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modeAttachment;

    void generateNoiseTextures();
    void drawNoiseTexture(juce::Graphics& g, juce::Rectangle<int> bounds);
    
    juce::Rectangle<int> claritySliderBounds;
    juce::Rectangle<int> clarityTrackBounds;
    juce::Rectangle<int> timeKnobBounds;
    juce::Rectangle<int> toneKnobBounds;
    
    int currentMode = 0;
    
    void setupGui();
    void modeButtonClicked(int mode);
    void drawKnob(juce::Graphics& g, juce::Rectangle<int> bounds, float value, const juce::String& label,
                  int labelX, int labelY, int labelW, int labelH);
    void drawClaritySlider(juce::Graphics& g, juce::Rectangle<int> bounds, float value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaritizerAudioProcessorEditor)
};
