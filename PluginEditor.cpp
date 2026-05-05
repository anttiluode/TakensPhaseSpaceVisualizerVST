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
    
    setupSlider(sDelay, lDelay, "Delay τ (samples)");
    setupSlider(sGain, lGain, "Input Gain");
    
    aDelay = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "delay_tau", sDelay);
    aGain = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "input_gain", sGain);
    
    pointsLabel.setText("Display Points", juce::dontSendNotification);
    pointsLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    pointsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pointsLabel);
    
    pointsCombo.addItem("500", 500);
    pointsCombo.addItem("1000", 1000);
    pointsCombo.addItem("2000", 2000);
    pointsCombo.addItem("3000", 3000);
    pointsCombo.addItem("4000", 4000);
    pointsCombo.addItem("8000", 8000);
    pointsCombo.setSelectedId(3000, juce::dontSendNotification); // Matches python default
    pointsCombo.onChange = [this] { updateDisplayPoints(); };
    pointsCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a3a));
    pointsCombo.setColour(juce::ComboBox::textColourId, TRACE_COLOR);
    addAndMakeVisible(pointsCombo);
    
    rmsLabel.setText("RMS: 0.000", juce::dontSendNotification);
    rmsLabel.setColour(juce::Label::textColourId, TRACE_COLOR);
    rmsLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(rmsLabel);
    
    updateDisplayPoints();
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
    l.setText(name, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, juce::Colours::grey);
}

void TakensEditor::updateDisplayPoints() {
    displayPoints = pointsCombo.getSelectedId();
}

void TakensEditor::timerCallback() {
    float rms = processor.rmsLevel.load();
    rmsLabel.setText(juce::String::formatted("RMS: %.4f", rms), juce::dontSendNotification);
    repaint();
}

void TakensEditor::paint(juce::Graphics& g) {
    g.fillAll(BG_COLOR);
    
    juce::Rectangle<int> plotArea(40, 40, getWidth() - 80, getHeight() - 160);
    g.setColour(juce::Colour(0xff111420));
    g.fillRect(plotArea);
    g.setColour(GRID_COLOR);
    g.drawRect(plotArea, 1.5f);
    
    int cx = plotArea.getCentreX();
    int cy = plotArea.getCentreY();
    g.setColour(GRID_COLOR);
    g.drawLine(cx, plotArea.getY(), cx, plotArea.getBottom());
    g.drawLine(plotArea.getX(), cy, plotArea.getRight(), cy);
    
    g.setColour(GRID_COLOR.withAlpha(0.3f));
    for (float t = -1.0f; t <= 1.0f; t += 0.5f) {
        if (std::abs(t) < 0.01f) continue;
        int xOffset = cx + t * plotArea.getWidth() / 2;
        int yOffset = cy - t * plotArea.getHeight() / 2;
        g.drawLine(xOffset, plotArea.getY(), xOffset, plotArea.getBottom());
        g.drawLine(plotArea.getX(), yOffset, plotArea.getRight(), yOffset);
    }
    
    // TAKENS EMBEDDING LOGIC
    int delaySamples = (int)sDelay.getValue();
    int currentWrite = processor.writeIndex.load(std::memory_order_relaxed);
    int bufSize = TakensProcessor::bufferSize;
    
    // Extract the points from the ring buffer backwards
    std::vector<juce::Point<float>> points;
    points.reserve(displayPoints);
    float maxAbs = 0.0001f;
    
    for (int i = 0; i < displayPoints; ++i) {
        int t = currentWrite - 1 - i;
        while (t < 0) t += bufSize; // Wrap around ring buffer
        
        int t_tau = t - delaySamples;
        while (t_tau < 0) t_tau += bufSize; // Wrap around for delay
        
        float x = processor.ringBuffer[(size_t)t];
        float y = processor.ringBuffer[(size_t)t_tau];
        
        points.push_back({x, y});
        maxAbs = std::max({maxAbs, std::abs(x), std::abs(y)});
    }
    
    // Draw Scatter Plot Trace
    if (points.size() > 1) {
        // Auto-scale to fit nicely, imitating Python's max(std) sizing
        float hr = maxAbs * 2.5f; 
        if (hr < 1e-9f) hr = 1.0f; // Prevent division by zero on silence
        
        float scaleX = (plotArea.getWidth() * 0.5f) / hr;
        float scaleY = (plotArea.getHeight() * 0.5f) / hr;
        float scale = std::min(scaleX, scaleY);
        
        // Mimic matplotlib's scatter dots instead of connecting lines
        g.setColour(TRACE_COLOR.withAlpha(0.7f));
        for (const auto& pt : points) {
            float px = cx + pt.x * scale;
            float py = cy - pt.y * scale; // Invert Y for screen coords
            
            if (plotArea.contains(px, py)) {
                // Draw 2x2 dot 
                g.fillEllipse(px - 1.0f, py - 1.0f, 2.0f, 2.0f);
            }
        }
    }
    
    // Labels
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("S(t)", plotArea.getRight() - 35, cy + 5, 30, 15, juce::Justification::centred);
    g.drawText("S(t+τ)", cx + 5, plotArea.getY() + 5, 50, 15, juce::Justification::centred);
    
    g.setFont(14.0f);
    g.setColour(TRACE_COLOR);
    g.drawText(juce::String::formatted("Takens Phase Space  |  τ = %d samples  |  Points: %d", 
               delaySamples, displayPoints),
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
    
    int pointsX = spacing + 2 * (knobSize + spacing);
    pointsLabel.setBounds(pointsX, y, knobSize, 20);
    pointsCombo.setBounds(pointsX, y + 25, knobSize, 30);
    
    rmsLabel.setBounds(getWidth() - 120, 15, 100, 20);
}