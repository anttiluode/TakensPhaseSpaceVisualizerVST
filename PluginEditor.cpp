#include "PluginEditor.h"

static const juce::Colour BG_COLOR { 0xff0a0a1a };
static const juce::Colour GRID_COLOR { 0xff1a1a3a };
static const juce::Colour TRACE_COLOR { 0xff00ffcc };
static const juce::Colour SLIDER_COLOR { 0xff44ffaa };

TakensEditor::TakensEditor(TakensProcessor& p) 
    : AudioProcessorEditor(&p), processor(p) {
    
    setSize(700, 650);
    setResizable(true, true);
    setResizeLimits(500, 500, 1500, 1500);
    
    setupSlider(sDelay, lDelay, "Delay τ (ms)");
    setupSlider(sGain, lGain, "Input Gain");
    
    aDelay = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "delay_tau", sDelay);
    aGain = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "input_gain", sGain);
    
    // Points selector
    pointsLabel.setText("Display Points", juce::dontSendNotification);
    pointsLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    pointsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pointsLabel);
    
    pointsCombo.addItem("500", 500);
    pointsCombo.addItem("1000", 1000);
    pointsCombo.addItem("2000", 2000);
    pointsCombo.addItem("4000", 4000);
    pointsCombo.addItem("8000", 8000);
    pointsCombo.setSelectedId(2000, juce::dontSendNotification);
    pointsCombo.onChange = [this] { updateDisplayPoints(); };
    pointsCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a3a));
    pointsCombo.setColour(juce::ComboBox::textColourId, TRACE_COLOR);
    addAndMakeVisible(pointsCombo);
    
    // RMS Label
    rmsLabel.setText("RMS: 0.000", juce::dontSendNotification);
    rmsLabel.setColour(juce::Label::textColourId, TRACE_COLOR);
    rmsLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(rmsLabel);
    
    startTimerHz(30);
}

TakensEditor::~TakensEditor() {
    stopTimer();
}

void TakensEditor::setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name) {
    addAndMakeVisible(s);
    addAndMakeVisible(l);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    s.setColour(juce::Slider::thumbColourId, SLIDER_COLOR);
    s.setColour(juce::Slider::rotarySliderFillColourId, TRACE_COLOR);
    // FIXED: remove the non-existent setTextBoxTextColour call
    // Text box text color is controlled via LookAndFeel or just use default
    l.setText(name, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, juce::Colours::grey);
}

void TakensEditor::updateDisplayPoints() {
    displayPoints = pointsCombo.getSelectedId();
}

void TakensEditor::timerCallback() {
    float x = processor.scopeX.load();
    float y = processor.scopeY.load();
    float rms = processor.rmsLevel.load();
    
    scopePath.push_back({x, y});
    while (scopePath.size() > displayPoints) {
        scopePath.pop_front();
    }
    
    rmsLabel.setText(juce::String::formatted("RMS: %.4f", rms), juce::dontSendNotification);
    repaint();
}

void TakensEditor::paint(juce::Graphics& g) {
    g.fillAll(BG_COLOR);
    
    // Phase space area
    juce::Rectangle<int> plotArea(40, 40, getWidth() - 80, getHeight() - 160);
    g.setColour(juce::Colour(0xff111420));
    g.fillRect(plotArea);
    g.setColour(GRID_COLOR);
    g.drawRect(plotArea, 1.5f);
    
    // Grid lines
    int cx = plotArea.getCentreX();
    int cy = plotArea.getCentreY();
    g.setColour(GRID_COLOR);
    g.drawLine(cx, plotArea.getY(), cx, plotArea.getBottom());
    g.drawLine(plotArea.getX(), cy, plotArea.getRight(), cy);
    
    // Minor grid
    g.setColour(GRID_COLOR.withAlpha(0.3f));
    for (float t = -1.0f; t <= 1.0f; t += 0.5f) {
        if (std::abs(t) < 0.01f) continue;
        int xOffset = cx + t * plotArea.getWidth() / 2;
        int yOffset = cy - t * plotArea.getHeight() / 2;
        g.drawLine(xOffset, plotArea.getY(), xOffset, plotArea.getBottom());
        g.drawLine(plotArea.getX(), yOffset, plotArea.getRight(), yOffset);
    }
    
    // Draw trace
    if (scopePath.size() > 1) {
        // Auto-scale to fit ~60% of the plot area
        float maxAbs = 0.001f;
        for (const auto& pt : scopePath) {
            maxAbs = std::max({maxAbs, std::abs(pt.x), std::abs(pt.y)});
        }
        
        float scaleX = (plotArea.getWidth() * 0.5f) / maxAbs;
        float scaleY = (plotArea.getHeight() * 0.5f) / maxAbs;
        float scale = std::min(scaleX, scaleY);
        
        juce::Path path;
        bool first = true;
        
        for (const auto& pt : scopePath) {
            float px = cx + pt.x * scale;
            float py = cy - pt.y * scale; // Invert Y for screen coords
            
            if (first) {
                path.startNewSubPath(px, py);
                first = false;
            } else {
                path.lineTo(px, py);
            }
        }
        
        // Glow
        g.setColour(TRACE_COLOR.withAlpha(0.15f));
        g.strokePath(path, juce::PathStrokeType(5.0f));
        
        // Main trace
        g.setColour(TRACE_COLOR);
        g.strokePath(path, juce::PathStrokeType(1.8f));
    }
    
    // Labels
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("S(t)", plotArea.getRight() - 35, cy + 5, 30, 15, 
               juce::Justification::centred);
    g.drawText("S(t+τ)", cx + 5, plotArea.getY() + 5, 50, 15,
               juce::Justification::centred);
    
    // Title
    float delayMs = sDelay.getValue();
    g.setFont(14.0f);
    g.setColour(TRACE_COLOR);
    g.drawText(juce::String::formatted("Takens Phase Space  |  τ = %.1f ms  |  Points: %d", 
               delayMs, displayPoints),
               plotArea.getX(), plotArea.getY() - 25, plotArea.getWidth(), 20,
               juce::Justification::centred);
}

void TakensEditor::resized() {
    int knobSize = 90;
    int y = getHeight() - 110;
    int spacing = (getWidth() - (knobSize * 3)) / 4;
    
    auto positionKnob = [&](juce::Slider& s, juce::Label& l, int idx) {
        int x = spacing + idx * (knobSize + spacing);
        s.setBounds(x, y + 25, knobSize, knobSize);
        l.setBounds(x, y, knobSize, 20);
    };
    
    positionKnob(sDelay, lDelay, 0);
    positionKnob(sGain, lGain, 1);
    
    // Points control position
    int pointsX = spacing + 2 * (knobSize + spacing);
    pointsLabel.setBounds(pointsX, y, knobSize, 20);
    pointsCombo.setBounds(pointsX, y + 25, knobSize, 30);
    
    // RMS label
    rmsLabel.setBounds(getWidth() - 120, 15, 100, 20);
}