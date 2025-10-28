#include "PluginProcessor.h"
#include "PluginEditor.h"



void ClaritizerAudioProcessorEditor::drawNoiseTexture(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Set opacity based on mode
    g.setOpacity(noiseOpacity);
    g.drawImageAt(noiseFrames[currentNoiseFrame], 0, 0);
    g.setOpacity(1.0f);
}

ClaritizerAudioProcessorEditor::ClaritizerAudioProcessorEditor (ClaritizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    setupGui();
    generateNoiseTextures();
    setSize (800, 600); // Made wider to fit debug controls
    
    // Start noise animation timer
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
    // Create 4 different noise textures for animation
    juce::Random random;
    
    for (int i = 0; i < 4; ++i)
    {
        noiseFrames[i] = juce::Image(juce::Image::PixelFormat::RGB, 400, 600, true);
        
        juce::Graphics g(noiseFrames[i]);
        g.setColour(juce::Colours::white);
        
        // Fill with random noise
        for (int x = 0; x < 400; x += 2)
        {
            for (int y = 0; y < 600; y += 2)
            {
                if (random.nextFloat() > 0.5f)
                {
                    g.setColour(juce::Colours::white.withAlpha(random.nextFloat() * 0.1f + 0.05f));
                    g.fillRect(x, y, 2, 2);
                }
            }
        }
    }
}


void ClaritizerAudioProcessorEditor::setupGui()
{
    // Clarity slider (invisible, just for value tracking)
    claritySlider.setSliderStyle(juce::Slider::LinearVertical);
    claritySlider.setRange(0.0, 1.0, 0.01);
    claritySlider.setValue(0.5);
    claritySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    claritySlider.setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);
    claritySlider.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
    claritySlider.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(claritySlider);
    
    // Time knob (invisible, just for value tracking)
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
    
    // Tone knob (invisible, just for value tracking)
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
    
    // Create parameter attachments
    clarityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "clarity", claritySlider);
    timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "time", timeKnob);
    toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "tone", toneKnob);
    
    // Create a special slider for mode parameter (hidden)
    modeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    modeSlider.setRange(0, 3, 1);
    modeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    modeSlider.onValueChange = [this] {
        int mode = (int)modeSlider.getValue();
        if (mode != currentMode) {
            currentMode = mode;
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
    
    // Setup debug sliders
    if (showDebug)
    {
        auto setupDebugSlider = [this](juce::Slider& slider, juce::Label& label,
                                       const juce::String& name, float min, float max, float defaultVal)
        {
            addAndMakeVisible(slider);
            addAndMakeVisible(label);
            slider.setRange(min, max, 1.0);
            slider.setValue(defaultVal);
            slider.setSliderStyle(juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
            slider.onValueChange = [this] { resized(); repaint(); };
            label.setText(name, juce::dontSendNotification);
            label.setColour(juce::Label::textColourId, juce::Colours::white);
            label.setFont(juce::Font(12.0f));
        };
        
        setupDebugSlider(debugClarityX, debugLabel1, "ClarityX", 0, 300, 80);
        setupDebugSlider(debugClarityY, debugLabel2, "ClarityY", 0, 300, 80);
        setupDebugSlider(debugClarityW, debugLabel3, "ClarityW", 10, 150, 15);
        setupDebugSlider(debugClarityH, debugLabel4, "ClarityH", 100, 600, 500);
        setupDebugSlider(debugClarityThumbW, debugLabel5, "ClarityThumbW", 20, 150, 50);
        setupDebugSlider(debugClarityThumbH, debugLabel6, "ClarityThumbH", 10, 100, 20);
        
        setupDebugSlider(debugClarityBarY, debugLabel23, "ClarityBarY", 40, 200, 70);
        setupDebugSlider(debugClarityBarHeight, debugLabel24, "ClarityBarH", 100, 500, 250);
        
        setupDebugSlider(debugTimeKnobX, debugLabel7, "TimeKnobX", 100, 400, 220);
        setupDebugSlider(debugTimeKnobY, debugLabel8, "TimeKnobY", 50, 400, 80);
        setupDebugSlider(debugToneKnobX, debugLabel9, "ToneKnobX", 100, 400, 220);
        setupDebugSlider(debugKnobSize, debugLabel10, "KnobSize", 50, 250, 100);
        setupDebugSlider(debugKnobSpacing, debugLabel11, "KnobSpace", 0, 150, 20);
        setupDebugSlider(debugKnobBorderThickness, debugLabel12, "KnobBorder", 1, 20, 6);
        setupDebugSlider(debugKnobArcThickness, debugLabel13, "KnobArc", 1, 20, 8);
        
        setupDebugSlider(debugButtonX, debugLabel14, "ButtonX", 0, 300, 80);
        setupDebugSlider(debugButtonY, debugLabel15, "ButtonY", 200, 600, 400);
        setupDebugSlider(debugButtonW, debugLabel16, "ButtonW", 30, 150, 80);
        setupDebugSlider(debugButtonH, debugLabel17, "ButtonH", 30, 150, 80);
        setupDebugSlider(debugButtonSpacing, debugLabel18, "BtnSpace", 0, 50, 20);
        setupDebugSlider(debugButtonBorderThickness, debugLabel19, "BtnBorder", 1, 15, 4);
        
        setupDebugSlider(debugTitleFontSize, debugLabel20, "TitleFont", 20, 100, 48);
        setupDebugSlider(debugLabelFontSize, debugLabel21, "LabelFont", 10, 50, 24);
        setupDebugSlider(debugButtonFontSize, debugLabel22, "ButtonFont", 20, 80, 36);
    }
}

void ClaritizerAudioProcessorEditor::modeButtonClicked(int mode)
{
    currentMode = mode;
    
    // Update button toggle states
    modeAButton.setToggleState(mode == 0, juce::dontSendNotification);
    modeBButton.setToggleState(mode == 1, juce::dontSendNotification);
    modeCButton.setToggleState(mode == 2, juce::dontSendNotification);
    modeDButton.setToggleState(mode == 3, juce::dontSendNotification);
    
    // Update mode parameter
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
    currentNoiseFrame = (currentNoiseFrame + 1) % 4;
    repaint();
}

void ClaritizerAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Only paint the main plugin area (left 400px)
    auto pluginBounds = getLocalBounds().withWidth(400);
    
    // LAYER 1: Gradient background
    juce::ColourGradient gradient(
        modeColors[currentMode], 0, 0,
        modeColors[currentMode].withLightness(modeColors[currentMode].getLightness() + 0.2f), 0, getHeight(),
        false);
    g.setGradientFill(gradient);
    g.fillRect(pluginBounds);
    
    // Draw outer border
    g.setColour(modeColors[currentMode]);
    g.drawRect(pluginBounds, 3);
    
    // LAYER 2: Black rectangle (with cutouts)
    g.setColour(juce::Colours::black);
    
    // Top section (above title)
    g.fillRect(3, 3, 394, 40);
    
    // Main black area - fill strategically around controls
    // Left margin
    g.fillRect(3, 43, claritySliderBounds.getX() - 3, pluginBounds.getHeight() - 46);
    
    // Between clarity slider and knobs
    g.fillRect(claritySliderBounds.getRight(), 43,
               timeKnobBounds.getX() - claritySliderBounds.getRight(),
               pluginBounds.getHeight() - 46);
    
    // Right of knobs
    g.fillRect(timeKnobBounds.getRight(), 43,
               397 - timeKnobBounds.getRight(),
               pluginBounds.getHeight() - 46);
    
    // Between time knob and tone knob
    g.fillRect(timeKnobBounds.getX(), timeKnobBounds.getBottom(),
               timeKnobBounds.getWidth(),
               toneKnobBounds.getY() - timeKnobBounds.getBottom());
    
    // Between tone knob and buttons
    g.fillRect(timeKnobBounds.getX(), toneKnobBounds.getBottom(),
               timeKnobBounds.getWidth(),
               modeAButton.getY() - toneKnobBounds.getBottom());
    
    // Below buttons
    g.fillRect(modeAButton.getX(), modeCButton.getBottom(),
               modeBButton.getRight() - modeAButton.getX(),
               pluginBounds.getHeight() - modeCButton.getBottom() - 3);
    
    // LAYER 3: Draw noise texture
    drawNoiseTexture(g, pluginBounds);
    
    // LAYER 4: Draw controls
    drawClaritySlider(g, claritySliderBounds, claritySlider.getValue());
    drawKnob(g, timeKnobBounds, (timeKnob.getValue() - 0.1f) / 2.9f, "Time");
    drawKnob(g, toneKnobBounds, toneKnob.getValue(), "Tone");
    
    // Draw title
    g.setColour(modeColors[currentMode]);
    float titleSize = showDebug ? debugTitleFontSize.getValue() : 48.0f;
    g.setFont(juce::Font("Times New Roman", titleSize, juce::Font::bold));
    g.drawText("Claritize", juce::Rectangle<int>(0, 0, 400, 50), juce::Justification::centred);
    
    // Draw debug panel background
    if (showDebug)
    {
        // Update the LookAndFeel with current debug values
        customLookAndFeel.buttonBorderThickness = debugButtonBorderThickness.getValue();
        customLookAndFeel.buttonFontSize = debugButtonFontSize.getValue();
        
        int debugX = 420;
        int debugY = 20;
        g.setColour(juce::Colour(0xff202020));
        g.fillRect(400, 0, 400, getHeight());
    }
}

void ClaritizerAudioProcessorEditor::drawClaritySlider(juce::Graphics& g, juce::Rectangle<int> bounds, float value)
{
    // Define the VISUAL bar area (where you actually draw)
    int barY = showDebug ? (int)debugClarityBarY.getValue() : 100;  // Start lower
    int barHeight = showDebug ? (int)debugClarityBarHeight.getValue() : 280;  // Shorter to avoid buttons
    
    auto trackWidth = 20;
    auto trackX = bounds.getX() + (bounds.getWidth() - trackWidth) / 2;
    
    // Calculate thumb position within the visible bar area
    auto thumbY = barY + barHeight * (1.0f - value);
    
    float thumbW = showDebug ? debugClarityThumbW.getValue() : 50.0f;
    float thumbH = showDebug ? debugClarityThumbH.getValue() : 20.0f;
    auto thumbBounds = juce::Rectangle<float>(
        trackX - (thumbW - trackWidth) / 2, thumbY - thumbH / 2,
        thumbW, thumbH);
    
    // Draw the actual slider bar background
    g.setColour(modeColors[currentMode].withAlpha(0.3f));
    g.fillRect(trackX, barY, trackWidth, barHeight);
    
    // Draw thumb
    g.setColour(juce::Colours::white);
    g.fillRect(thumbBounds);
}

void ClaritizerAudioProcessorEditor::drawKnob(juce::Graphics& g, juce::Rectangle<int> bounds, float value, const juce::String& label)
{
    auto knobSize = juce::jmin(bounds.getWidth(), bounds.getHeight() - 30);
    auto knobBounds = bounds.withSizeKeepingCentre(knobSize, knobSize).toFloat();
    
    auto radius = knobSize / 2.0f - 6.0f;
    auto centreX = knobBounds.getCentreX();
    auto centreY = knobBounds.getCentreY();
    
    auto rotaryStartAngle = juce::MathConstants<float>::pi * 1.2f;
    auto rotaryEndAngle = juce::MathConstants<float>::pi * 2.8f;
    auto angle = rotaryStartAngle + value * (rotaryEndAngle - rotaryStartAngle);
    
    float borderThickness = showDebug ? debugKnobBorderThickness.getValue() : 6.0f;
    float arcThickness = showDebug ? debugKnobArcThickness.getValue() : 8.0f;
    
    // Draw outer circle (gradient shows through)
    g.setColour(modeColors[currentMode]);
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2, borderThickness);
    
    // Draw value arc (white on gradient background)
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                          rotaryStartAngle, angle, true);
    g.setColour(juce::Colours::white);
    g.strokePath(valueArc, juce::PathStrokeType(arcThickness));
    
    // Draw label below
    float labelSize = showDebug ? debugLabelFontSize.getValue() : 24.0f;
    g.setFont(juce::Font("Times New Roman", labelSize, juce::Font::plain));
    g.setColour(modeColors[currentMode]);
    auto labelBounds = bounds.removeFromBottom(30);
    g.drawText(label, labelBounds, juce::Justification::centred);
}

void ClaritizerAudioProcessorEditor::resized()
{
    // Hardcoded values from your current settings
    int clarityX = showDebug ? (int)debugClarityX.getValue() : 80;
    int clarityBarY = showDebug ? (int)debugClarityBarY.getValue() : 100;
    int clarityBarHeight = showDebug ? (int)debugClarityBarHeight.getValue() : 280;
    int clarityW = showDebug ? (int)debugClarityW.getValue() : 50;
    
    int timeKnobX = showDebug ? (int)debugTimeKnobX.getValue() : 220;
    int timeKnobY = showDebug ? (int)debugTimeKnobY.getValue() : 80;
    int toneKnobX = showDebug ? (int)debugToneKnobX.getValue() : 220;
    int knobSize = showDebug ? (int)debugKnobSize.getValue() : 100;
    int knobSpacing = showDebug ? (int)debugKnobSpacing.getValue() : 20;
    
    int buttonX = showDebug ? (int)debugButtonX.getValue() : 113;
    int buttonY = showDebug ? (int)debugButtonY.getValue() : 400;
    int buttonW = showDebug ? (int)debugButtonW.getValue() : 80;
    int buttonH = showDebug ? (int)debugButtonH.getValue() : 84;
    int buttonSpacing = showDebug ? (int)debugButtonSpacing.getValue() : 20;
    
    // Clarity slider - Set bounds to EXACTLY match the visual bar
    claritySliderBounds = juce::Rectangle<int>(
        clarityX - 17,  // Center the clickable area on the visual bar
        clarityBarY,
        clarityW,
        clarityBarHeight
    );
    claritySlider.setBounds(claritySliderBounds);
    
    // Time knob
    timeKnobBounds = juce::Rectangle<int>(timeKnobX, timeKnobY, knobSize, knobSize + 30);
    timeKnob.setBounds(timeKnobBounds);
    
    // Tone knob
    int toneKnobY = timeKnobY + knobSize + 30 + knobSpacing;
    toneKnobBounds = juce::Rectangle<int>(toneKnobX, toneKnobY, knobSize, knobSize + 30);
    toneKnob.setBounds(toneKnobBounds);
    
    // Mode buttons (2x2 grid)
    modeAButton.setBounds(buttonX, buttonY, buttonW, buttonH);
    modeBButton.setBounds(buttonX + buttonW + buttonSpacing, buttonY, buttonW, buttonH);
    modeCButton.setBounds(buttonX, buttonY + buttonH + buttonSpacing, buttonW, buttonH);
    modeDButton.setBounds(buttonX + buttonW + buttonSpacing, buttonY + buttonH + buttonSpacing, buttonW, buttonH);
    
    // Layout debug sliders on the right side
    if (showDebug)
    {
        int debugStartX = 420;
        int debugY = 20;
        int labelW = 85;
        int sliderW = 135;
        int spacing = 28;
        int columnSpacing = 240;
        
        auto layoutDebug = [&](juce::Slider& slider, juce::Label& label, int column)
        {
            int xOffset = column * columnSpacing;
            label.setBounds(debugStartX + xOffset, debugY, labelW, 20);
            slider.setBounds(debugStartX + xOffset + labelW, debugY, sliderW, 20);
            debugY += spacing;
            
            // Reset to top for new column
            if (debugY > 550)
            {
                debugY = 20;
            }
        };
        
        // Column 1
        int col = 0;
        layoutDebug(debugClarityX, debugLabel1, col);
        layoutDebug(debugClarityY, debugLabel2, col);
        layoutDebug(debugClarityW, debugLabel3, col);
        layoutDebug(debugClarityH, debugLabel4, col);
        layoutDebug(debugClarityThumbW, debugLabel5, col);
        layoutDebug(debugClarityThumbH, debugLabel6, col);
        layoutDebug(debugTimeKnobX, debugLabel7, col);
        layoutDebug(debugTimeKnobY, debugLabel8, col);
        
        // Column 2
        col = 1;
        debugY = 20;
        layoutDebug(debugToneKnobX, debugLabel9, col);
        layoutDebug(debugKnobSize, debugLabel10, col);
        layoutDebug(debugKnobSpacing, debugLabel11, col);
        layoutDebug(debugKnobBorderThickness, debugLabel12, col);
        layoutDebug(debugKnobArcThickness, debugLabel13, col);
        layoutDebug(debugButtonX, debugLabel14, col);
        layoutDebug(debugButtonY, debugLabel15, col);
        layoutDebug(debugButtonW, debugLabel16, col);
        
        // Column 3
        col = 2;
        debugY = 20;
        layoutDebug(debugButtonH, debugLabel17, col);
        layoutDebug(debugButtonSpacing, debugLabel18, col);
        layoutDebug(debugButtonBorderThickness, debugLabel19, col);
        layoutDebug(debugTitleFontSize, debugLabel20, col);
        layoutDebug(debugLabelFontSize, debugLabel21, col);
        layoutDebug(debugButtonFontSize, debugLabel22, col);
        layoutDebug(debugClarityBarY, debugLabel23, col);
        layoutDebug(debugClarityBarHeight, debugLabel24, col);
    }
}
