#include "PluginProcessor.h"
#include "PluginEditor.h"

void ClaritizerAudioProcessorEditor::drawNoiseTexture(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    juce::Graphics::ScopedSaveState saveState(g);
    g.setOpacity(noiseOpacity);
    g.drawImage(noiseFrame, bounds.toFloat(), juce::RectanglePlacement::stretchToFit);
}

ClaritizerAudioProcessorEditor::ClaritizerAudioProcessorEditor (ClaritizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    setupGui();
    generateNoiseTextures();
    setSize (900, 600);
    startTimerHz(10);
}

ClaritizerAudioProcessorEditor::~ClaritizerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void ClaritizerAudioProcessorEditor::generateNoiseTextures()
{
    juce::Random random;
    noiseFrame = juce::Image(juce::Image::PixelFormat::ARGB, 400, 600, true);
    juce::Graphics g(noiseFrame);
    g.fillAll(juce::Colours::transparentBlack);
    
    int pixelSize = noisePixelSize;
    for (int x = 0; x < 400; x += pixelSize)
    {
        for (int y = 0; y < 600; y += pixelSize)
        {
            float noiseValue = random.nextFloat();
            if (noiseValue > 0.4f)
            {
                float brightness = (noiseValue - 0.4f) / 0.6f;
                g.setColour(juce::Colours::white.withAlpha(brightness * 0.8f));
                g.fillRect(x, y, pixelSize, pixelSize);
            }
        }
    }
}

void ClaritizerAudioProcessorEditor::setupGui()
{
    // Main UI controls (unchanged)
    claritySlider.setSliderStyle(juce::Slider::LinearVertical);
    claritySlider.setRange(0.0, 1.0, 0.01);
    claritySlider.setValue(0.5);
    claritySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    claritySlider.setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);
    claritySlider.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
    claritySlider.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(claritySlider);
    
    timeKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    timeKnob.setRange(0.1, 3.0, 0.01);
    timeKnob.setValue(1.0);
    timeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    timeKnob.setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);
    timeKnob.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
    timeKnob.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    timeKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
    timeKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(timeKnob);
    
    toneKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    toneKnob.setRange(0.0, 1.0, 0.01);
    toneKnob.setValue(0.5);
    toneKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    toneKnob.setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);
    toneKnob.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
    toneKnob.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    toneKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
    toneKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(toneKnob);
    
    claritySlider.setInterceptsMouseClicks(true, false);
    claritySlider.setOpaque(false);
    timeKnob.setInterceptsMouseClicks(true, false);
    timeKnob.setOpaque(false);
    toneKnob.setInterceptsMouseClicks(true, false);
    toneKnob.setOpaque(false);
    
    // Mode buttons
    addAndMakeVisible(modeAButton);
    addAndMakeVisible(modeBButton);
    addAndMakeVisible(modeCButton);
    addAndMakeVisible(modeDButton);
    
    modeAButton.setButtonText("A");
    modeBButton.setButtonText("B");
    modeCButton.setButtonText("C");
    modeDButton.setButtonText("D");
    
    modeAButton.setClickingTogglesState(true);
    modeBButton.setClickingTogglesState(true);
    modeCButton.setClickingTogglesState(true);
    modeDButton.setClickingTogglesState(true);
    
    modeAButton.onClick = [this] { modeButtonClicked(0); };
    modeBButton.onClick = [this] { modeButtonClicked(1); };
    modeCButton.onClick = [this] { modeButtonClicked(2); };
    modeDButton.onClick = [this] { modeButtonClicked(3); };
    
    modeAButton.setToggleState(true, juce::dontSendNotification);
    
    // Parameter attachments
    clarityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "clarity", claritySlider);
    timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "time", timeKnob);
    toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "tone", toneKnob);
    
    modeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    modeSlider.setRange(0, 3, 1);
    modeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    modeSlider.onValueChange = [this] {
        int mode = (int)modeSlider.getValue();
        if (mode != currentMode) {
            currentMode = mode;
            customLookAndFeel.setModeColor(modeColors[mode]);
            modeAButton.setToggleState(mode == 0, juce::dontSendNotification);
            modeBButton.setToggleState(mode == 1, juce::dontSendNotification);
            modeCButton.setToggleState(mode == 2, juce::dontSendNotification);
            modeDButton.setToggleState(mode == 3, juce::dontSendNotification);
            repaint();
        }
    };
    addChildComponent(&modeSlider);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "mode", modeSlider);
    
    // DEBUG SLIDERS - NEW ARCHITECTURE (23 total)
    if (showDebug)
    {
        addAndMakeVisible(debugViewport);
        debugViewport.setViewedComponent(&debugContainer, false);
        debugViewport.setScrollBarsShown(true, false);
        
        // Helper to create audio sliders (0-10 scale)
        auto setupAudioSlider = [this](juce::Slider& slider, juce::Label& label,
                                       const juce::String& name, float defaultVal)
        {
            debugContainer.addAndMakeVisible(slider);
            debugContainer.addAndMakeVisible(label);
            slider.setRange(0.0, 10.0, 0.1);
            slider.setValue(defaultVal);
            slider.setSliderStyle(juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 70, 20);
            slider.setScrollWheelEnabled(false);
            label.setText(name, juce::dontSendNotification);
            label.setColour(juce::Label::textColourId, juce::Colours::white);
            label.setFont(juce::Font(12.0f));
        };
        
        // CHORUS (5 sliders) - with Patrick's settings
        setupAudioSlider(debugModeA_ChorusTime, debugLabelA1, "Chorus_Time", 0.1);
        setupAudioSlider(debugModeA_ChorusFeedback, debugLabelA2, "Chorus_Feedb", 2.0);
        setupAudioSlider(debugModeA_ChorusModDepth, debugLabelA3, "Chorus_ModDep", 5.0);
        setupAudioSlider(debugModeA_ChorusModRate, debugLabelA4, "Chorus_ModRate", 8.0);
        setupAudioSlider(debugModeA_ChorusMix, debugLabelA5, "Chorus_Mix", 2.0);
        
        // DELAY 1 (6 sliders) - Patrick's settings
        setupAudioSlider(debugModeA_D1Time, debugLabelA6, "D1_Time", 5.0);
        setupAudioSlider(debugModeA_D1Feedback, debugLabelA7, "D1_Feedback", 4.0);
        setupAudioSlider(debugModeA_D1ModDepth, debugLabelA8, "D1_ModDepth", 0.0);
        setupAudioSlider(debugModeA_D1ModRate, debugLabelA9, "D1_ModRate", 0.0);
        setupAudioSlider(debugModeA_D1Mix, debugLabelA10, "D1_Mix", 10.0);
        setupAudioSlider(debugModeA_D1Reverse, debugLabelA11, "D1_Reverse", 10.0);
        
        // DELAY 2 (6 sliders) - muted
        setupAudioSlider(debugModeA_D2Time, debugLabelA12, "D2_Time", 2.0);
        setupAudioSlider(debugModeA_D2Feedback, debugLabelA13, "D2_Feedback", 0.0);
        setupAudioSlider(debugModeA_D2ModDepth, debugLabelA14, "D2_ModDepth", 0.0);
        setupAudioSlider(debugModeA_D2ModRate, debugLabelA15, "D2_ModRate", 0.0);
        setupAudioSlider(debugModeA_D2Mix, debugLabelA16, "D2_Mix", 0.0);
        setupAudioSlider(debugModeA_D2Reverse, debugLabelA17, "D2_Reverse", 0.0);
        
        // REVERB (6 sliders) - Patrick's settings
        setupAudioSlider(debugModeA_Rev1Time, debugLabelA18, "Rev1_Time", 0.7);
        setupAudioSlider(debugModeA_Rev2Time, debugLabelA19, "Rev2_Time", 1.7);
        setupAudioSlider(debugModeA_Rev3Time, debugLabelA20, "Rev3_Time", 2.5);
        setupAudioSlider(debugModeA_Rev4Time, debugLabelA21, "Rev4_Time", 4.2);
        setupAudioSlider(debugModeA_RevFeedback, debugLabelA22, "Rev_Feedback", 2.0);
        setupAudioSlider(debugModeA_RevMix, debugLabelA23, "Rev_Mix", 2.0);
        
        // Mapping functions (0-10 slider → actual parameter ranges)
        auto mapTime = [](float v) { return 10.0f + (v / 10.0f) * 1990.0f; }; // 0-10 → 10-2000ms
        auto mapChorusTime = [](float v) { return 10.0f + (v / 10.0f) * 40.0f; }; // 0-10 → 10-50ms for chorus
        auto mapReverbTime = [](float v) { return 10.0f + (v / 10.0f) * 490.0f; }; // 0-10 → 10-500ms for reverb
        auto mapFeedback = [](float v) { return (v / 10.0f) * 0.95f; }; // 0-10 → 0.0-0.95
        auto mapModDepth = [](float v) { return (v / 10.0f) * 50.0f; }; // 0-10 → 0-50ms
        auto mapModRate = [](float v) { return (v / 10.0f) * 5.0f; }; // 0-10 → 0-5Hz
        auto mapMix = [](float v) { return v / 10.0f; }; // 0-10 → 0.0-1.0
        auto mapReverse = [](float v) { return v > 5.0f; }; // 0-10 → bool (>5 = true)
        
        // Update callback - writes to processor's debugModeConfigs[0]
        auto updateModeA = [this, mapTime, mapChorusTime, mapReverbTime, mapFeedback,
                           mapModDepth, mapModRate, mapMix, mapReverse]() {
            audioProcessor.useDebugConfigs = true;
            auto& config = audioProcessor.debugModeConfigs[0];
            
            // Chorus
            config.chorus.timeMs = mapChorusTime(debugModeA_ChorusTime.getValue());
            config.chorus.feedback = mapFeedback(debugModeA_ChorusFeedback.getValue());
            config.chorus.modDepth = mapModDepth(debugModeA_ChorusModDepth.getValue());
            config.chorus.modRate = mapModRate(debugModeA_ChorusModRate.getValue());
            config.chorus.mix = mapMix(debugModeA_ChorusMix.getValue());
            
            // Delay 1
            config.delay1.baseTimeMs = mapTime(debugModeA_D1Time.getValue());
            config.delay1.feedback = mapFeedback(debugModeA_D1Feedback.getValue());
            config.delay1.modDepth = mapModDepth(debugModeA_D1ModDepth.getValue());
            config.delay1.modRate = mapModRate(debugModeA_D1ModRate.getValue());
            config.delay1.mix = mapMix(debugModeA_D1Mix.getValue());
            config.delay1.reverse = mapReverse(debugModeA_D1Reverse.getValue());
            
            // Delay 2
            config.delay2.baseTimeMs = mapTime(debugModeA_D2Time.getValue());
            config.delay2.feedback = mapFeedback(debugModeA_D2Feedback.getValue());
            config.delay2.modDepth = mapModDepth(debugModeA_D2ModDepth.getValue());
            config.delay2.modRate = mapModRate(debugModeA_D2ModRate.getValue());
            config.delay2.mix = mapMix(debugModeA_D2Mix.getValue());
            config.delay2.reverse = mapReverse(debugModeA_D2Reverse.getValue());
            
            // Reverb
            config.reverb.delay1Time = mapReverbTime(debugModeA_Rev1Time.getValue());
            config.reverb.delay2Time = mapReverbTime(debugModeA_Rev2Time.getValue());
            config.reverb.delay3Time = mapReverbTime(debugModeA_Rev3Time.getValue());
            config.reverb.delay4Time = mapReverbTime(debugModeA_Rev4Time.getValue());
            config.reverb.sharedFeedback = mapFeedback(debugModeA_RevFeedback.getValue());
            config.reverb.mix = mapMix(debugModeA_RevMix.getValue());
        };
        
        // Attach callbacks to all Mode A sliders
        debugModeA_ChorusTime.onValueChange = updateModeA;
        debugModeA_ChorusFeedback.onValueChange = updateModeA;
        debugModeA_ChorusModDepth.onValueChange = updateModeA;
        debugModeA_ChorusModRate.onValueChange = updateModeA;
        debugModeA_ChorusMix.onValueChange = updateModeA;
        
        debugModeA_D1Time.onValueChange = updateModeA;
        debugModeA_D1Feedback.onValueChange = updateModeA;
        debugModeA_D1ModDepth.onValueChange = updateModeA;
        debugModeA_D1ModRate.onValueChange = updateModeA;
        debugModeA_D1Mix.onValueChange = updateModeA;
        debugModeA_D1Reverse.onValueChange = updateModeA;
        
        debugModeA_D2Time.onValueChange = updateModeA;
        debugModeA_D2Feedback.onValueChange = updateModeA;
        debugModeA_D2ModDepth.onValueChange = updateModeA;
        debugModeA_D2ModRate.onValueChange = updateModeA;
        debugModeA_D2Mix.onValueChange = updateModeA;
        debugModeA_D2Reverse.onValueChange = updateModeA;
        
        debugModeA_Rev1Time.onValueChange = updateModeA;
        debugModeA_Rev2Time.onValueChange = updateModeA;
        debugModeA_Rev3Time.onValueChange = updateModeA;
        debugModeA_Rev4Time.onValueChange = updateModeA;
        debugModeA_RevFeedback.onValueChange = updateModeA;
        debugModeA_RevMix.onValueChange = updateModeA;
    }
}

void ClaritizerAudioProcessorEditor::modeButtonClicked(int mode)
{
    currentMode = mode;
    customLookAndFeel.setModeColor(modeColors[mode]);
    
    modeAButton.setToggleState(mode == 0, juce::dontSendNotification);
    modeBButton.setToggleState(mode == 1, juce::dontSendNotification);
    modeCButton.setToggleState(mode == 2, juce::dontSendNotification);
    modeDButton.setToggleState(mode == 3, juce::dontSendNotification);
    
    if (auto* param = audioProcessor.parameters.getParameter("mode"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(mode / 3.0f);
        param->endChangeGesture();
    }
    
    repaint();
}

void ClaritizerAudioProcessorEditor::timerCallback()
{
    // No animation needed
}

void ClaritizerAudioProcessorEditor::paint(juce::Graphics& g)
{
    int pluginWidth = 350;
    auto pluginBounds = getLocalBounds().withWidth(pluginWidth);
    
    g.fillAll(juce::Colours::black);
    
    // Draw outer border with gradient
    float borderThickness = 10.0f;
    auto borderBounds = pluginBounds.toFloat();
    juce::ColourGradient borderGradient(
        juce::Colours::white, borderBounds.getX(), borderBounds.getY(),
        modeColors[currentMode], borderBounds.getX(), borderBounds.getBottom(),
        false);
    g.setGradientFill(borderGradient);
    g.drawRect(borderBounds, borderThickness);
    
    // Draw controls
    drawClaritySlider(g, clarityTrackBounds, claritySlider.getValue());
    drawKnob(g, timeKnobBounds, (timeKnob.getValue() - 0.1f) / 2.9f, "Time", 121, 200, 150, 40);
    drawKnob(g, toneKnobBounds, toneKnob.getValue(), "Tone", 176, 360, 150, 20);
    
    // Draw title
    g.setGradientFill(juce::ColourGradient(
        juce::Colours::white, 0, 20,
        modeColors[currentMode], 0, 70, false));
    g.setFont(juce::Font("Times New Roman", 80.0f, juce::Font::bold));
    g.drawText("Claritizer", juce::Rectangle<int>(0, 20, pluginWidth, 50), juce::Justification::centred);
    
    // Debug panel background
    if (showDebug)
    {
        int debugStartX = pluginWidth + 10;
        g.setColour(juce::Colour(0xff202020));
        g.fillRect(debugStartX, 0, getWidth() - debugStartX, getHeight());
    }
    
    // Noise overlay (final layer)
    drawNoiseTexture(g, pluginBounds);
}

void ClaritizerAudioProcessorEditor::drawClaritySlider(juce::Graphics& g, juce::Rectangle<int> bounds, float value)
{
    int trackWidth = 20;
    int trackHeight = 290;
    int trackX = bounds.getX();
    int trackY = bounds.getY();
    
    juce::ColourGradient gradient(
        juce::Colours::white, trackX, trackY,
        modeColors[currentMode], trackX, trackY + trackHeight, false);
    g.setGradientFill(gradient);
    g.fillRect(trackX, trackY, trackWidth, trackHeight);
    
    auto thumbY = trackY + trackHeight * (1.0f - value);
    float thumbW = 80.0f;
    float thumbH = 20.0f;
    auto thumbBounds = juce::Rectangle<float>(
        trackX + (trackWidth - thumbW) / 2, thumbY - thumbH / 2,
        thumbW, thumbH);
    
    g.setColour(juce::Colours::white);
    g.fillRect(thumbBounds);
}

void ClaritizerAudioProcessorEditor::drawKnob(juce::Graphics& g, juce::Rectangle<int> bounds, float value,
                                               const juce::String& label, int labelX, int labelY, int labelW, int labelH)
{
    float radius = 50.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    
    auto rotaryStartAngle = juce::MathConstants<float>::pi * 1.2f;
    auto rotaryEndAngle = juce::MathConstants<float>::pi * 2.8f;
    auto angle = rotaryStartAngle + value * (rotaryEndAngle - rotaryStartAngle);
    
    // Border
    juce::ColourGradient borderGradient(
        juce::Colours::white, centreX, centreY - radius,
        modeColors[currentMode], centreX, centreY + radius, false);
    g.setGradientFill(borderGradient);
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2, 6.0f);
    
    // Value arc
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                          rotaryStartAngle, angle, true);
    g.setColour(juce::Colours::white);
    g.strokePath(valueArc, juce::PathStrokeType(8.0f));
    
    // Position indicator
    float radialAngle = angle - juce::MathConstants<float>::halfPi;
    float indicatorStartX = centreX + std::cos(radialAngle) * 35.0f;
    float indicatorStartY = centreY + std::sin(radialAngle) * 35.0f;
    float indicatorEndX = centreX + std::cos(radialAngle) * 55.0f;
    float indicatorEndY = centreY + std::sin(radialAngle) * 55.0f;
    
    {
        juce::Graphics::ScopedSaveState saveState(g);
        juce::Path clipCircle;
        clipCircle.addEllipse(centreX - radius - 3, centreY - radius - 3, (radius + 3) * 2, (radius + 3) * 2);
        g.reduceClipRegion(clipCircle);
        g.setColour(juce::Colours::white);
        g.drawLine(indicatorStartX, indicatorStartY, indicatorEndX, indicatorEndY, 10.0f);
    }
    
    // Label
    g.setFont(juce::Font("Times New Roman", 24.0f, juce::Font::plain));
    juce::ColourGradient labelGradient(
        juce::Colours::white, labelX, labelY,
        modeColors[currentMode], labelX, labelY + labelH, false);
    g.setGradientFill(labelGradient);
    g.drawText(label, juce::Rectangle<int>(labelX, labelY, labelW, labelH), juce::Justification::centred);
}

void ClaritizerAudioProcessorEditor::resized()
{
    int pluginWidth = 350;
    
    // Clarity slider
    int clarityX = 70;
    int clarityY = 90;
    int clarityTrackW = 20;
    int clarityH = 290;
    float thumbW = 80.0f;
    float thumbH = 20.0f;
    
    int sliderWidth = juce::jmax((int)thumbW, clarityTrackW);
    int sliderX = clarityX - (sliderWidth - clarityTrackW) / 2;
    int verticalPadding = (int)(thumbH / 2.0f) + 2;
    int sliderHeight = clarityH + (verticalPadding * 2);
    int sliderY = clarityY - verticalPadding;
    
    claritySliderBounds = juce::Rectangle<int>(sliderX, sliderY, sliderWidth, sliderHeight);
    claritySlider.setBounds(claritySliderBounds);
    clarityTrackBounds = juce::Rectangle<int>(clarityX, clarityY, clarityTrackW, clarityH);
    
    // Knobs
    int knobSize = 120;
    timeKnobBounds = juce::Rectangle<int>(200 - knobSize/2, 150 - knobSize/2, knobSize, knobSize);
    timeKnob.setBounds(timeKnobBounds);
    
    toneKnobBounds = juce::Rectangle<int>(250 - knobSize/2, 300 - knobSize/2, knobSize, knobSize);
    toneKnob.setBounds(toneKnobBounds);
    
    // Mode buttons
    modeAButton.setBounds(30, 400, 140, 80);
    modeBButton.setBounds(180, 400, 140, 80);
    modeCButton.setBounds(30, 490, 140, 80);
    modeDButton.setBounds(180, 490, 140, 80);
    
    // Debug panel - NEW LAYOUT for 23 sliders
    if (showDebug)
    {
        int debugStartX = pluginWidth + 20;
        debugViewport.setBounds(debugStartX, 0, 500, getHeight());
        
        int debugY = 10;
        int spacing = 28;
        
        auto layoutSlider = [&](juce::Slider& slider, juce::Label& label)
        {
            label.setBounds(20, debugY, 120, 20);
            slider.setBounds(150, debugY, 120, 20);
            debugY += spacing;
        };
        
        // Chorus (5)
        layoutSlider(debugModeA_ChorusTime, debugLabelA1);
        layoutSlider(debugModeA_ChorusFeedback, debugLabelA2);
        layoutSlider(debugModeA_ChorusModDepth, debugLabelA3);
        layoutSlider(debugModeA_ChorusModRate, debugLabelA4);
        layoutSlider(debugModeA_ChorusMix, debugLabelA5);
        
        debugY += 10; // Space
        
        // Delay 1 (6)
        layoutSlider(debugModeA_D1Time, debugLabelA6);
        layoutSlider(debugModeA_D1Feedback, debugLabelA7);
        layoutSlider(debugModeA_D1ModDepth, debugLabelA8);
        layoutSlider(debugModeA_D1ModRate, debugLabelA9);
        layoutSlider(debugModeA_D1Mix, debugLabelA10);
        layoutSlider(debugModeA_D1Reverse, debugLabelA11);
        
        debugY += 10;
        
        // Delay 2 (6)
        layoutSlider(debugModeA_D2Time, debugLabelA12);
        layoutSlider(debugModeA_D2Feedback, debugLabelA13);
        layoutSlider(debugModeA_D2ModDepth, debugLabelA14);
        layoutSlider(debugModeA_D2ModRate, debugLabelA15);
        layoutSlider(debugModeA_D2Mix, debugLabelA16);
        layoutSlider(debugModeA_D2Reverse, debugLabelA17);
        
        debugY += 10;
        
        // Reverb (6)
        layoutSlider(debugModeA_Rev1Time, debugLabelA18);
        layoutSlider(debugModeA_Rev2Time, debugLabelA19);
        layoutSlider(debugModeA_Rev3Time, debugLabelA20);
        layoutSlider(debugModeA_Rev4Time, debugLabelA21);
        layoutSlider(debugModeA_RevFeedback, debugLabelA22);
        layoutSlider(debugModeA_RevMix, debugLabelA23);
        
        debugContainer.setSize(480, juce::jmax(debugY + 20, 700));
    }
}
