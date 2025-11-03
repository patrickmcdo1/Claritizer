#include "PluginProcessor.h"
#include "PluginEditor.h"

void ClaritizerAudioProcessorEditor::drawNoiseTexture(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Save the current state
    juce::Graphics::ScopedSaveState saveState(g);
    
    // Set opacity for the noise overlay
    g.setOpacity(noiseOpacity);
    
    // Draw the noise frame scaled to fit the plugin bounds
    g.drawImage(noiseFrame,
                bounds.toFloat(),
                juce::RectanglePlacement::stretchToFit);
}

ClaritizerAudioProcessorEditor::ClaritizerAudioProcessorEditor (ClaritizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    setupGui();
    generateNoiseTextures();
    setSize (900, 600); // Made wider for debug panel
    
    startTimerHz(10);
}

ClaritizerAudioProcessorEditor::~ClaritizerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    modeAttachment = nullptr;
    clarityAttachment = nullptr;
    timeAttachment = nullptr;
    toneAttachment = nullptr;
}

void ClaritizerAudioProcessorEditor::generateNoiseTextures()
{
    juce::Random random;
    
    // Create single noise frame (no animation)
    // Use ARGB format to support transparency properly
    noiseFrame = juce::Image(juce::Image::PixelFormat::ARGB, 400, 600, true);
    
    juce::Graphics g(noiseFrame);
    
    // Clear to transparent
    g.fillAll(juce::Colours::transparentBlack);
    
    // Create pixelated noise - use the adjustable pixel size
    int pixelSize = noisePixelSize;
    
    for (int x = 0; x < 400; x += pixelSize)
    {
        for (int y = 0; y < 600; y += pixelSize)
        {
            // Random value for this pixel
            float noiseValue = random.nextFloat();
            
            // Only draw if above threshold (creates sparse noise)
            if (noiseValue > 0.4f)
            {
                // Map the noise value to brightness (0.4-1.0 range mapped to visible brightness)
                float brightness = (noiseValue - 0.4f) / 0.6f; // normalize to 0-1
                
                // Make it white with varying intensity
                g.setColour(juce::Colours::white.withAlpha(brightness * 0.8f));
                g.fillRect(x, y, pixelSize, pixelSize);
            }
        }
    }
}

void ClaritizerAudioProcessorEditor::setupGui()
{
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
    
    if (showDebug)
    {
        // Setup viewport for scrolling debug panel
        addAndMakeVisible(debugViewport);
        debugViewport.setViewedComponent(&debugContainer, false);
        debugViewport.setScrollBarsShown(true, false);
        
        auto setupDebugSlider = [this](juce::Slider& slider, juce::Label& label,
                                       const juce::String& name, float min, float max, float defaultVal)
        {
            debugContainer.addAndMakeVisible(slider);
            debugContainer.addAndMakeVisible(label);
            slider.setRange(min, max, 1.0);
            slider.setValue(defaultVal);
            slider.setSliderStyle(juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
            slider.setScrollWheelEnabled(false); // DISABLE SCROLL WHEEL
            slider.onValueChange = [this] { resized(); repaint(); };
            label.setText(name, juce::dontSendNotification);
            label.setColour(juce::Label::textColourId, juce::Colours::white);
            label.setFont(juce::Font(12.0f));
        };
        
        // Values from your screenshots
        setupDebugSlider(debugClaritySliderX, debugLabel1, "ClaritySliderX", 0, 350, 70);
        setupDebugSlider(debugClaritySliderY, debugLabel2, "ClaritySliderY", 0, 300, 90);
        setupDebugSlider(debugClarityTrackWidth, debugLabel3, "ClarityTrackW", 10, 150, 20);
        setupDebugSlider(debugClarityTrackHeight, debugLabel4, "ClarityTrackH", 100, 600, 290);
        setupDebugSlider(debugClarityThumbW, debugLabel5, "ClarityThumbW", 20, 150, 80);
        setupDebugSlider(debugClarityThumbH, debugLabel6, "ClarityThumbH", 10, 100, 20);
        
        setupDebugSlider(debugTimeKnobX, debugLabel7, "TimeKnobX", 50, 400, 200);
        setupDebugSlider(debugTimeKnobY, debugLabel8, "TimeKnobY", 50, 600, 150);
        setupDebugSlider(debugToneKnobX, debugLabel9, "ToneKnobX", 50, 400, 250);
        setupDebugSlider(debugToneKnobY, debugLabel10, "ToneKnobY", 50, 600, 300);
        setupDebugSlider(debugKnobRadius, debugLabel11, "KnobRadius", 20, 150, 50);
        setupDebugSlider(debugKnobSpacing, debugLabel12, "KnobSpacing", 0, 150, 20);
        setupDebugSlider(debugKnobBorderThickness, debugLabel13, "KnobBorder", 1, 20, 10);
        setupDebugSlider(debugKnobArcThickness, debugLabel14, "KnobArc", 1, 20, 10);
        
        setupDebugSlider(debugTimeLabelX, debugLabel15, "TimeLabelX", 0, 500, 121);
        setupDebugSlider(debugTimeLabelY, debugLabel16, "TimeLabelY", 0, 600, 200);
        setupDebugSlider(debugTimeLabelW, debugLabel17, "TimeLabelW", 50, 300, 150);
        setupDebugSlider(debugTimeLabelH, debugLabel18, "TimeLabelH", 20, 100, 40);
        setupDebugSlider(debugToneLabelX, debugLabel19, "ToneLabelX", 0, 500, 176);
        setupDebugSlider(debugToneLabelY, debugLabel20, "ToneLabelY", 0, 600, 360);
        setupDebugSlider(debugToneLabelW, debugLabel21, "ToneLabelW", 50, 300, 150);
        setupDebugSlider(debugToneLabelH, debugLabel22, "ToneLabelH", 20, 100, 20);
        setupDebugSlider(debugLabelFontSize, debugLabel23, "LabelFont", 10, 50, 24);
        
        setupDebugSlider(debugButtonX, debugLabel24, "ButtonX", 0, 300, 30);
        setupDebugSlider(debugButtonY, debugLabel25, "ButtonY", 200, 600, 400);
        setupDebugSlider(debugButtonW, debugLabel26, "ButtonW", 30, 200, 140);
        setupDebugSlider(debugButtonH, debugLabel27, "ButtonH", 30, 150, 80);
        setupDebugSlider(debugButtonSpacing, debugLabel28, "ButtonSpacing", 0, 50, 10);
        setupDebugSlider(debugButtonBorderThickness, debugLabel29, "ButtonBorder", 1, 15, 8);
        setupDebugSlider(debugButtonFontSize, debugLabel30, "ButtonFont", 20, 100, 48);
        
        setupDebugSlider(debugTitleX, debugLabel31, "TitleX", 0, 400, 0);
        setupDebugSlider(debugTitleY, debugLabel32, "TitleY", 0, 200, 20);
        setupDebugSlider(debugTitleW, debugLabel33, "TitleW", 100, 600, 350);
        setupDebugSlider(debugTitleH, debugLabel34, "TitleH", 20, 150, 50);
        setupDebugSlider(debugTitleFontSize, debugLabel35, "TitleFont", 20, 120, 80);
        
        setupDebugSlider(debugOuterBorderThickness, debugLabel36, "OuterBorder", 1, 20, 10);
        setupDebugSlider(debugPluginWidth, debugLabel37, "PluginWidth", 300, 500, 350);
        
        // Noise controls - special setup with different step size for opacity
        debugContainer.addAndMakeVisible(debugNoiseOpacity);
        debugContainer.addAndMakeVisible(debugLabel38);
        debugNoiseOpacity.setRange(0.0, 1.0, 0.01); // Fine control for opacity
        debugNoiseOpacity.setValue(0.1);
        debugNoiseOpacity.setSliderStyle(juce::Slider::LinearHorizontal);
        debugNoiseOpacity.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
        debugNoiseOpacity.setScrollWheelEnabled(false);
        debugNoiseOpacity.onValueChange = [this] {
            noiseOpacity = debugNoiseOpacity.getValue();
            repaint();
        };
        debugLabel38.setText("NoiseOpacity", juce::dontSendNotification);
        debugLabel38.setColour(juce::Label::textColourId, juce::Colours::white);
        debugLabel38.setFont(juce::Font(12.0f));
        
        debugContainer.addAndMakeVisible(debugNoisePixelSize);
        debugContainer.addAndMakeVisible(debugLabel39);
        debugNoisePixelSize.setRange(1, 16, 1);
        debugNoisePixelSize.setValue(1);
        debugNoisePixelSize.setSliderStyle(juce::Slider::LinearHorizontal);
        debugNoisePixelSize.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
        debugNoisePixelSize.setScrollWheelEnabled(false);
        debugNoisePixelSize.onValueChange = [this] {
            noisePixelSize = (int)debugNoisePixelSize.getValue();
            generateNoiseTextures(); // Regenerate noise with new pixel size
            repaint();
        };
        debugLabel39.setText("NoisePixelSize", juce::dontSendNotification);
        debugLabel39.setColour(juce::Label::textColourId, juce::Colours::white);
        debugLabel39.setFont(juce::Font(12.0f));
        
        // Knob tick mark controls
        debugContainer.addAndMakeVisible(debugKnobTickLength);
        debugContainer.addAndMakeVisible(debugLabel40);
        debugKnobTickLength.setRange(1, 80, 1);
        debugKnobTickLength.setValue(20);
        debugKnobTickLength.setSliderStyle(juce::Slider::LinearHorizontal);
        debugKnobTickLength.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
        debugKnobTickLength.setScrollWheelEnabled(false);
        debugKnobTickLength.onValueChange = [this] {
            knobTickLength = debugKnobTickLength.getValue();
            repaint();
        };
        debugLabel40.setText("KnobTickLength", juce::dontSendNotification);
        debugLabel40.setColour(juce::Label::textColourId, juce::Colours::white);
        debugLabel40.setFont(juce::Font(12.0f));
        
        debugContainer.addAndMakeVisible(debugKnobTickThickness);
        debugContainer.addAndMakeVisible(debugLabel41);
        debugKnobTickThickness.setRange(1, 20, 0.5);
        debugKnobTickThickness.setValue(10);
        debugKnobTickThickness.setSliderStyle(juce::Slider::LinearHorizontal);
        debugKnobTickThickness.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
        debugKnobTickThickness.setScrollWheelEnabled(false);
        debugKnobTickThickness.onValueChange = [this] {
            knobTickThickness = debugKnobTickThickness.getValue();
            repaint();
        };
        debugLabel41.setText("KnobTickThick", juce::dontSendNotification);
        debugLabel41.setColour(juce::Label::textColourId, juce::Colours::white);
        debugLabel41.setFont(juce::Font(12.0f));
        
        debugContainer.addAndMakeVisible(debugKnobTickStartRadius);
        debugContainer.addAndMakeVisible(debugLabel42);
        debugKnobTickStartRadius.setRange(0, 60, 1);
        debugKnobTickStartRadius.setValue(35);
        debugKnobTickStartRadius.setSliderStyle(juce::Slider::LinearHorizontal);
        debugKnobTickStartRadius.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
        debugKnobTickStartRadius.setScrollWheelEnabled(false);
        debugKnobTickStartRadius.onValueChange = [this] {
            knobTickStartRadius = debugKnobTickStartRadius.getValue();
            repaint();
        };
        debugLabel42.setText("KnobTickStart", juce::dontSendNotification);
        debugLabel42.setColour(juce::Label::textColourId, juce::Colours::white);
        debugLabel42.setFont(juce::Font(12.0f));
    }
}

void ClaritizerAudioProcessorEditor::modeButtonClicked(int mode)
{
    currentMode = mode;
    
    // Update the LookAndFeel with the new mode color
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
    // Timer callback removed - noise is static now
}

void ClaritizerAudioProcessorEditor::paint(juce::Graphics& g)
{
    int pluginWidth = showDebug ? (int)debugPluginWidth.getValue() : 350;
    auto pluginBounds = getLocalBounds().withWidth(pluginWidth);
    
    // LAYER 1: Black background ONLY for plugin area
    g.setColour(juce::Colours::black);
    g.fillRect(pluginBounds);
    
    // Draw outer border with gradient (white to mode color)
    float borderThickness = showDebug ? debugOuterBorderThickness.getValue() : 8.0f;
    auto borderBounds = pluginBounds.toFloat().reduced(0, 0);
    juce::ColourGradient borderGradient(
        juce::Colours::white, borderBounds.getX(), borderBounds.getY(),
        modeColors[currentMode], borderBounds.getX(), borderBounds.getBottom(),
        false);
    g.setGradientFill(borderGradient);
    g.drawRect(borderBounds, borderThickness);
    
    // LAYER 2: Draw controls
    drawClaritySlider(g, clarityTrackBounds, claritySlider.getValue());
    
    int timeLabelX = showDebug ? (int)debugTimeLabelX.getValue() : 200;
    int timeLabelY = showDebug ? (int)debugTimeLabelY.getValue() : 200;
    int timeLabelW = showDebug ? (int)debugTimeLabelW.getValue() : 150;
    int timeLabelH = showDebug ? (int)debugTimeLabelH.getValue() : 40;
    drawKnob(g, timeKnobBounds, (timeKnob.getValue() - 0.1f) / 2.9f, "Time",
             timeLabelX, timeLabelY, timeLabelW, timeLabelH);
    
    int toneLabelX = showDebug ? (int)debugToneLabelX.getValue() : 200;
    int toneLabelY = showDebug ? (int)debugToneLabelY.getValue() : 400;
    int toneLabelW = showDebug ? (int)debugToneLabelW.getValue() : 150;
    int toneLabelH = showDebug ? (int)debugToneLabelH.getValue() : 40;
    drawKnob(g, toneKnobBounds, toneKnob.getValue(), "Tone",
             toneLabelX, toneLabelY, toneLabelW, toneLabelH);
    
    // Draw title with gradient
    float titleSize = showDebug ? debugTitleFontSize.getValue() : 60.0f;
    int titleX = showDebug ? (int)debugTitleX.getValue() : 0;
    int titleY = showDebug ? (int)debugTitleY.getValue() : 20;
    int titleW = showDebug ? (int)debugTitleW.getValue() : pluginWidth;
    int titleH = showDebug ? (int)debugTitleH.getValue() : 80;
    
    // Create gradient for title text (white to mode color)
    juce::ColourGradient titleGradient(
        juce::Colours::white, titleX, titleY,
        modeColors[currentMode], titleX, titleY + titleH, false);
    g.setGradientFill(titleGradient);
    g.setFont(juce::Font("Times New Roman", titleSize, juce::Font::bold));
    g.drawText("Claritize", juce::Rectangle<int>(titleX, titleY, titleW, titleH), juce::Justification::centred);
    
    // Draw debug panel background
    if (showDebug)
    {
        customLookAndFeel.buttonBorderThickness = debugButtonBorderThickness.getValue();
        customLookAndFeel.buttonFontSize = debugButtonFontSize.getValue();
        
        int debugStartX = pluginWidth + 10;
        g.setColour(juce::Colour(0xff202020));
        g.fillRect(debugStartX, 0, getWidth() - debugStartX, getHeight());
        
        // Draw background for the debug container
        g.setColour(juce::Colour(0xff303030));
        auto viewportBounds = debugViewport.getBounds();
        g.fillRect(viewportBounds);
    }
    
    // FINAL LAYER: Draw noise texture on top of EVERYTHING
    drawNoiseTexture(g, pluginBounds);
}

void ClaritizerAudioProcessorEditor::drawClaritySlider(juce::Graphics& g, juce::Rectangle<int> bounds, float value)
{
    int trackWidth = showDebug ? (int)debugClarityTrackWidth.getValue() : 80;
    int trackHeight = showDebug ? (int)debugClarityTrackHeight.getValue() : 400;
    int trackX = bounds.getX();
    int trackY = bounds.getY();
    
    // Draw gradient background for the track (white to mode color)
    juce::ColourGradient gradient(
        juce::Colours::white, trackX, trackY,
        modeColors[currentMode], trackX, trackY + trackHeight, false);
    g.setGradientFill(gradient);
    g.fillRect(trackX, trackY, trackWidth, trackHeight);
    
    // Calculate thumb position
    auto thumbY = trackY + trackHeight * (1.0f - value);
    
    float thumbW = showDebug ? debugClarityThumbW.getValue() : 100.0f;
    float thumbH = showDebug ? debugClarityThumbH.getValue() : 20.0f;
    auto thumbBounds = juce::Rectangle<float>(
        trackX + (trackWidth - thumbW) / 2, thumbY - thumbH / 2,
        thumbW, thumbH);
    
    // Draw thumb
    g.setColour(juce::Colours::white);
    g.fillRect(thumbBounds);
}

void ClaritizerAudioProcessorEditor::drawKnob(juce::Graphics& g, juce::Rectangle<int> bounds, float value,
                                               const juce::String& label, int labelX, int labelY, int labelW, int labelH)
{
    float radius = showDebug ? debugKnobRadius.getValue() : 50.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    
    auto rotaryStartAngle = juce::MathConstants<float>::pi * 1.2f;
    auto rotaryEndAngle = juce::MathConstants<float>::pi * 2.8f;
    auto angle = rotaryStartAngle + value * (rotaryEndAngle - rotaryStartAngle);
    
    float borderThickness = showDebug ? debugKnobBorderThickness.getValue() : 6.0f;
    float arcThickness = showDebug ? debugKnobArcThickness.getValue() : 8.0f;
    
    // Draw outer circle border with gradient (white to mode color)
    juce::ColourGradient borderGradient(
        juce::Colours::white, centreX, centreY - radius,
        modeColors[currentMode], centreX, centreY + radius, false);
    g.setGradientFill(borderGradient);
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2, borderThickness);
    
    // Draw value arc
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                          rotaryStartAngle, angle, true);
    g.setColour(juce::Colours::white);
    g.strokePath(valueArc, juce::PathStrokeType(arcThickness));
    
    // Draw position indicator - a white line from center pointing to current value
    // Clip it to the knob circle so the corners don't stick out
    float indicatorStartRadius = showDebug ? knobTickStartRadius : 35.0f; // Distance from center to start
    float indicatorLength = showDebug ? knobTickLength : 20.0f; // Length of the indicator
    float indicatorThickness = showDebug ? knobTickThickness : 10.0f;
    
    // Calculate radial direction (perpendicular to the circle, pointing outward)
    float radialAngle = angle - juce::MathConstants<float>::halfPi;
    
    // Position the indicator starting from indicatorStartRadius from center
    float indicatorStartX = centreX + std::cos(radialAngle) * indicatorStartRadius;
    float indicatorStartY = centreY + std::sin(radialAngle) * indicatorStartRadius;
    float indicatorEndX = centreX + std::cos(radialAngle) * (indicatorStartRadius + indicatorLength);
    float indicatorEndY = centreY + std::sin(radialAngle) * (indicatorStartRadius + indicatorLength);
    
    // Save graphics state and apply circular clipping
    {
        juce::Graphics::ScopedSaveState saveState(g);
        
        // Create circular clipping path - extend to outer edge of border
        float clipRadius = radius + (borderThickness / 2.0f);
        juce::Path clipCircle;
        clipCircle.addEllipse(centreX - clipRadius, centreY - clipRadius, clipRadius * 2, clipRadius * 2);
        g.reduceClipRegion(clipCircle);
        
        // Draw with solid white for maximum visibility (now clipped to circle)
        g.setColour(juce::Colours::white);
        juce::Line<float> indicatorLine(indicatorStartX, indicatorStartY, indicatorEndX, indicatorEndY);
        g.drawLine(indicatorLine, indicatorThickness);
    }
    
    // Draw label with gradient (white to mode color)
    float labelSize = showDebug ? debugLabelFontSize.getValue() : 24.0f;
    g.setFont(juce::Font("Times New Roman", labelSize, juce::Font::plain));
    
    juce::ColourGradient labelGradient(
        juce::Colours::white, labelX, labelY,
        modeColors[currentMode], labelX, labelY + labelH, false);
    g.setGradientFill(labelGradient);
    
    auto labelBounds = juce::Rectangle<int>(labelX, labelY, labelW, labelH);
    g.drawText(label, labelBounds, juce::Justification::centred);
}

void ClaritizerAudioProcessorEditor::resized()
{
    int pluginWidth = showDebug ? (int)debugPluginWidth.getValue() : 350;
    
    // Clarity slider
    int clarityX = showDebug ? (int)debugClaritySliderX.getValue() : 40;
    int clarityY = showDebug ? (int)debugClaritySliderY.getValue() : 80;
    int clarityTrackW = showDebug ? (int)debugClarityTrackWidth.getValue() : 80;
    int clarityH = showDebug ? (int)debugClarityTrackHeight.getValue() : 400;
    float thumbW = showDebug ? debugClarityThumbW.getValue() : 100.0f;
    float thumbH = showDebug ? debugClarityThumbH.getValue() : 20.0f;
    
    // Make the slider bounds wide enough to include the full thumb width
    // AND tall enough to include the full thumb height at top and bottom
    int sliderWidth = juce::jmax((int)thumbW, clarityTrackW);
    int sliderX = clarityX - (sliderWidth - clarityTrackW) / 2;
    
    // Add vertical padding equal to half the thumb height at top and bottom
    int verticalPadding = (int)(thumbH / 2.0f) + 2; // +2 for extra safety margin
    int sliderHeight = clarityH + (verticalPadding * 2);
    int sliderY = clarityY - verticalPadding;
    
    claritySliderBounds = juce::Rectangle<int>(sliderX, sliderY, sliderWidth, sliderHeight);
    claritySlider.setBounds(claritySliderBounds);
    
    // Store the track bounds separately for drawing (without the padding)
    clarityTrackBounds = juce::Rectangle<int>(clarityX, clarityY, clarityTrackW, clarityH);
    
    // Time knob
    int timeKnobX = showDebug ? (int)debugTimeKnobX.getValue() : 250;
    int timeKnobY = showDebug ? (int)debugTimeKnobY.getValue() : 80;
    float knobRadius = showDebug ? debugKnobRadius.getValue() : 50.0f;
    int knobSize = (int)(knobRadius * 2 + 20); // Add padding for interaction
    
    timeKnobBounds = juce::Rectangle<int>(timeKnobX - knobSize/2, timeKnobY - knobSize/2, knobSize, knobSize);
    timeKnob.setBounds(timeKnobBounds);
    
    // Tone knob
    int toneKnobX = showDebug ? (int)debugToneKnobX.getValue() : 200;
    int toneKnobY = showDebug ? (int)debugToneKnobY.getValue() : 300;
    
    toneKnobBounds = juce::Rectangle<int>(toneKnobX - knobSize/2, toneKnobY - knobSize/2, knobSize, knobSize);
    toneKnob.setBounds(toneKnobBounds);
    
    // Mode buttons
    int buttonX = showDebug ? (int)debugButtonX.getValue() : 30;
    int buttonY = showDebug ? (int)debugButtonY.getValue() : 400;
    int buttonW = showDebug ? (int)debugButtonW.getValue() : 100;
    int buttonH = showDebug ? (int)debugButtonH.getValue() : 80;
    int buttonSpacing = showDebug ? (int)debugButtonSpacing.getValue() : 20;
    
    modeAButton.setBounds(buttonX, buttonY, buttonW, buttonH);
    modeBButton.setBounds(buttonX + buttonW + buttonSpacing, buttonY, buttonW, buttonH);
    modeCButton.setBounds(buttonX, buttonY + buttonH + buttonSpacing, buttonW, buttonH);
    modeDButton.setBounds(buttonX + buttonW + buttonSpacing, buttonY + buttonH + buttonSpacing, buttonW, buttonH);
    
    if (showDebug)
    {
        int debugStartX = pluginWidth + 20;
        debugViewport.setBounds(debugStartX, 0, 500, getHeight());
        
        int debugY = 20;
        int labelW = 110;
        int sliderW = 120;
        int spacing = 28;
        int columnSpacing = 250;
        int totalWidth = columnSpacing * 2 + 20;
        
        auto layoutDebug = [&](juce::Slider& slider, juce::Label& label, int column)
        {
            int xOffset = column * columnSpacing;
            label.setBounds(10 + xOffset, debugY, labelW, 20);
            slider.setBounds(10 + xOffset + labelW, debugY, sliderW, 20);
            debugY += spacing;
            
            // Reset to top for new column
            if (debugY > 550 && column == 0)
            {
                debugY = 20;
            }
        };
        
        // Column 1
        int col = 0;
        layoutDebug(debugClaritySliderX, debugLabel1, col);
        layoutDebug(debugClaritySliderY, debugLabel2, col);
        layoutDebug(debugClarityTrackWidth, debugLabel3, col);
        layoutDebug(debugClarityTrackHeight, debugLabel4, col);
        layoutDebug(debugClarityThumbW, debugLabel5, col);
        layoutDebug(debugClarityThumbH, debugLabel6, col);
        layoutDebug(debugTimeKnobX, debugLabel7, col);
        layoutDebug(debugTimeKnobY, debugLabel8, col);
        layoutDebug(debugToneKnobX, debugLabel9, col);
        layoutDebug(debugToneKnobY, debugLabel10, col);
        layoutDebug(debugKnobRadius, debugLabel11, col);
        layoutDebug(debugKnobSpacing, debugLabel12, col);
        layoutDebug(debugKnobBorderThickness, debugLabel13, col);
        layoutDebug(debugKnobArcThickness, debugLabel14, col);
        layoutDebug(debugTimeLabelX, debugLabel15, col);
        layoutDebug(debugTimeLabelY, debugLabel16, col);
        layoutDebug(debugTimeLabelW, debugLabel17, col);
        layoutDebug(debugTimeLabelH, debugLabel18, col);
        layoutDebug(debugToneLabelX, debugLabel19, col);
        
        // Column 2
        col = 1;
        debugY = 20;
        layoutDebug(debugToneLabelY, debugLabel20, col);
        layoutDebug(debugToneLabelW, debugLabel21, col);
        layoutDebug(debugToneLabelH, debugLabel22, col);
        layoutDebug(debugLabelFontSize, debugLabel23, col);
        layoutDebug(debugButtonX, debugLabel24, col);
        layoutDebug(debugButtonY, debugLabel25, col);
        layoutDebug(debugButtonW, debugLabel26, col);
        layoutDebug(debugButtonH, debugLabel27, col);
        layoutDebug(debugButtonSpacing, debugLabel28, col);
        layoutDebug(debugButtonBorderThickness, debugLabel29, col);
        layoutDebug(debugButtonFontSize, debugLabel30, col);
        layoutDebug(debugTitleX, debugLabel31, col);
        layoutDebug(debugTitleY, debugLabel32, col);
        layoutDebug(debugTitleW, debugLabel33, col);
        layoutDebug(debugTitleH, debugLabel34, col);
        layoutDebug(debugTitleFontSize, debugLabel35, col);
        layoutDebug(debugOuterBorderThickness, debugLabel36, col);
        layoutDebug(debugPluginWidth, debugLabel37, col);
        layoutDebug(debugNoiseOpacity, debugLabel38, col);
        layoutDebug(debugNoisePixelSize, debugLabel39, col);
        layoutDebug(debugKnobTickLength, debugLabel40, col);
        layoutDebug(debugKnobTickThickness, debugLabel41, col);
        layoutDebug(debugKnobTickStartRadius, debugLabel42, col);
        
        // Set container size to fit all sliders
        debugContainer.setSize(totalWidth, juce::jmax(debugY + 20, 600));
    }
}
