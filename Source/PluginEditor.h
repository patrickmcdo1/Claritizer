#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TransparentSlider : public juce::Slider
{
public:
    void paint(juce::Graphics& g) override
    {
        // Intentionally empty
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
        
        // Draw border with gradient (white to mode color)
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
            // Draw text with gradient when toggled (white to mode color)
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
    juce::Colour currentModeColor = juce::Colour(0xff7ba5d1); // Default to Mode A blue
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
    
    // Debug panel viewport for scrolling
    juce::Viewport debugViewport;
    juce::Component debugContainer;
    
    TransparentSlider claritySlider;
    TransparentSlider timeKnob;
    TransparentSlider toneKnob;
    TransparentButton modeAButton;
    TransparentButton modeBButton;
    TransparentButton modeCButton;
    TransparentButton modeDButton;
    
    // DEBUG SLIDERS - comprehensive UI controls
    bool showDebug = true;
    bool uiSectionExpanded = false;  // Collapsed by default
    bool audioProcessingSectionExpanded = true;  // Expanded by default
    juce::TextButton uiSectionButton;
    juce::TextButton audioProcessingSectionButton;
    
    // Clarity slider controls
    juce::Slider debugClaritySliderX, debugClaritySliderY;
    juce::Slider debugClarityTrackWidth, debugClarityTrackHeight;
    juce::Slider debugClarityThumbW, debugClarityThumbH;
    juce::Label debugLabel1, debugLabel2, debugLabel3, debugLabel4, debugLabel5, debugLabel6;
    
    // Knob controls
    juce::Slider debugTimeKnobX, debugTimeKnobY;
    juce::Slider debugToneKnobX, debugToneKnobY;
    juce::Slider debugKnobRadius, debugKnobBorderThickness, debugKnobArcThickness;
    juce::Slider debugKnobSpacing;
    juce::Label debugLabel7, debugLabel8, debugLabel9, debugLabel10, debugLabel11, debugLabel12, debugLabel13, debugLabel14;
    
    // Knob label controls
    juce::Slider debugTimeLabelX, debugTimeLabelY, debugTimeLabelW, debugTimeLabelH;
    juce::Slider debugToneLabelX, debugToneLabelY, debugToneLabelW, debugToneLabelH;
    juce::Slider debugLabelFontSize;
    juce::Label debugLabel15, debugLabel16, debugLabel17, debugLabel18, debugLabel19, debugLabel20;
    juce::Label debugLabel21, debugLabel22, debugLabel23;
    
    // Button controls
    juce::Slider debugButtonX, debugButtonY, debugButtonW, debugButtonH, debugButtonSpacing;
    juce::Slider debugButtonBorderThickness, debugButtonFontSize;
    juce::Label debugLabel24, debugLabel25, debugLabel26, debugLabel27, debugLabel28;
    juce::Label debugLabel29, debugLabel30;
    
    // Title controls
    juce::Slider debugTitleX, debugTitleY, debugTitleW, debugTitleH, debugTitleFontSize;
    juce::Label debugLabel31, debugLabel32, debugLabel33, debugLabel34, debugLabel35;
    
    // Border/edge controls
    juce::Slider debugOuterBorderThickness;
    juce::Slider debugPluginWidth;
    juce::Label debugLabel36, debugLabel37;
    
    // Noise controls
    juce::Slider debugNoiseOpacity, debugNoisePixelSize;
    juce::Label debugLabel38, debugLabel39;
    int noisePixelSize = 1;
    
    // Knob tick mark controls
    juce::Slider debugKnobTickLength, debugKnobTickThickness, debugKnobTickStartRadius;
    juce::Label debugLabel40, debugLabel41, debugLabel42;
    float knobTickLength = 20.0f;
    float knobTickThickness = 10.0f;
    float knobTickStartRadius = 35.0f;
    
    // Audio processing debug sliders - Mode A
    juce::Slider debugModeA_DelayTime, debugModeA_Feedback, debugModeA_ChorusDepth, debugModeA_ChorusRate;
    juce::Slider debugModeA_ReverbSize, debugModeA_ReverbDamping, debugModeA_ReverbWet;
    juce::Slider debugModeA_BitCrush, debugModeA_NoiseModAmount, debugModeA_NoiseModSpeed;
    juce::Label debugLabelA1, debugLabelA2, debugLabelA3, debugLabelA4, debugLabelA5;
    juce::Label debugLabelA6, debugLabelA7, debugLabelA8, debugLabelA9, debugLabelA10;
    
    // Audio processing debug sliders - Mode B
    juce::Slider debugModeB_DelayTime, debugModeB_Feedback, debugModeB_ChorusDepth, debugModeB_ChorusRate;
    juce::Slider debugModeB_ReverbSize, debugModeB_ReverbDamping, debugModeB_ReverbWet;
    juce::Slider debugModeB_BitCrush, debugModeB_NoiseModAmount, debugModeB_NoiseModSpeed;
    juce::Label debugLabelB1, debugLabelB2, debugLabelB3, debugLabelB4, debugLabelB5;
    juce::Label debugLabelB6, debugLabelB7, debugLabelB8, debugLabelB9, debugLabelB10;
    
    // Audio processing debug sliders - Mode C
    juce::Slider debugModeC_DelayTime, debugModeC_Feedback, debugModeC_ChorusDepth, debugModeC_ChorusRate;
    juce::Slider debugModeC_ReverbSize, debugModeC_ReverbDamping, debugModeC_ReverbWet;
    juce::Slider debugModeC_BitCrush, debugModeC_NoiseModAmount, debugModeC_NoiseModSpeed;
    juce::Label debugLabelC1, debugLabelC2, debugLabelC3, debugLabelC4, debugLabelC5;
    juce::Label debugLabelC6, debugLabelC7, debugLabelC8, debugLabelC9, debugLabelC10;
    
    // Audio processing debug sliders - Mode D
    juce::Slider debugModeD_DelayTime, debugModeD_Feedback, debugModeD_ChorusDepth, debugModeD_ChorusRate;
    juce::Slider debugModeD_ReverbSize, debugModeD_ReverbDamping, debugModeD_ReverbWet;
    juce::Slider debugModeD_BitCrush, debugModeD_NoiseModAmount, debugModeD_NoiseModSpeed;
    juce::Label debugLabelD1, debugLabelD2, debugLabelD3, debugLabelD4, debugLabelD5;
    juce::Label debugLabelD6, debugLabelD7, debugLabelD8, debugLabelD9, debugLabelD10;
    
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
