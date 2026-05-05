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
    
    setupSlider(sDelay, lDelay, "Z-Depth Delay");
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
    pointsCombo.setSelectedId(3000, juce::dontSendNotification);
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

TakensEditor::~TakensEditor() { stopTimer(); }

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

void TakensEditor::updateDisplayPoints() { displayPoints = pointsCombo.getSelectedId(); }
void TakensEditor::timerCallback() {
    float rms = processor.rmsLevel.load();
    rmsLabel.setText(juce::String::formatted("RMS: %.4f", rms), juce::dontSendNotification);
    repaint();
}

// Mouse interaction for 3D rotation
void TakensEditor::mouseDown(const juce::MouseEvent& e) {
    lastMousePos = e.getPosition();
}

void TakensEditor::mouseDrag(const juce::MouseEvent& e) {
    auto currentPos = e.getPosition();
    rotY += (currentPos.x - lastMousePos.x) * 0.01f;
    rotX += (currentPos.y - lastMousePos.y) * 0.01f;
    lastMousePos = currentPos;
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
    
    // 3D HYBRID EMBEDDING LOGIC
    int delaySamples = (int)sDelay.getValue();
    int currentWrite = processor.writeIndex.load(std::memory_order_relaxed);
    int bufSize = TakensProcessor::bufferSize;
    
    struct Point3D { float x, y, z; };
    std::vector<Point3D> points;
    points.reserve(displayPoints);
    float maxAbs = 0.0001f;
    
    for (int i = 0; i < displayPoints; ++i) {
        int t = currentWrite - 1 - i;
        while (t < 0) t += bufSize;
        
        int t_tau = t - delaySamples;
        while (t_tau < 0) t_tau += bufSize; 
        
        float x = processor.ringBufferL[(size_t)t];      // Left Channel
        float y = processor.ringBufferR[(size_t)t];      // Right Channel
        float z = processor.ringBufferL[(size_t)t_tau];  // Left Delayed (Takens Depth)
        
        points.push_back({x, y, z});
        maxAbs = std::max({maxAbs, std::abs(x), std::abs(y), std::abs(z)});
    }
    
    if (points.size() > 1) {
        float hr = maxAbs * 2.0f; 
        if (hr < 1e-9f) hr = 1.0f;
        
        float cs = std::min(plotArea.getWidth(), plotArea.getHeight()) * 0.45f / hr;
        
        juce::Path path;
        bool first = true;
        
        // Render from oldest to newest
        for (int i = (int)points.size() - 1; i >= 0; --i) {
            float x = points[i].x;
            float y = points[i].y;
            float z = points[i].z;
            
            // 3D Rotation Matrix
            float x1 = x * std::cos(rotY) - z * std::sin(rotY);
            float z1 = x * std::sin(rotY) + z * std::cos(rotY);
            float y2 = y * std::cos(rotX) - z1 * std::sin(rotX);
            float z2 = y * std::sin(rotX) + z1 * std::cos(rotX);
            
            // Perspective Projection
            float d = 2.8f;
            float s = d / (d + z2 * 0.6f);
            float px = cx + (x1 * s) * cs;
            float py = cy - (y2 * s) * cs; // Invert Y
            
            if (plotArea.contains(px, py)) {
                if (first) {
                    path.startNewSubPath(px, py);
                    first = false;
                } else {
                    path.lineTo(px, py);
                }
            }
        }
        
        // Draw the 3D Trajectory
        g.setColour(TRACE_COLOR.withAlpha(0.2f));
        g.strokePath(path, juce::PathStrokeType(4.0f)); // Glow
        g.setColour(TRACE_COLOR.withAlpha(0.8f));
        g.strokePath(path, juce::PathStrokeType(1.2f)); // Core line
        
        // Render Newest Point Glow
        if (!points.empty()) {
            float x = points[0].x, y = points[0].y, z = points[0].z;
            float x1 = x * std::cos(rotY) - z * std::sin(rotY);
            float z1 = x * std::sin(rotY) + z * std::cos(rotY);
            float y2 = y * std::cos(rotX) - z1 * std::sin(rotX);
            float z2 = y * std::sin(rotX) + z1 * std::cos(rotX);
            float s = 2.8f / (2.8f + z2 * 0.6f);
            float px = cx + (x1 * s) * cs, py = cy - (y2 * s) * cs;
            
            if (plotArea.contains(px, py)) {
                g.setColour(juce::Colours::white);
                g.fillEllipse(px - 2.5f, py - 2.5f, 5.0f, 5.0f);
            }
        }
    }
    
    // Labels
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("Drag to rotate 3D Manifold", plotArea.getRight() - 150, plotArea.getY() + 10, 140, 20, juce::Justification::centredRight);
    
    g.setFont(14.0f);
    g.setColour(TRACE_COLOR);
    g.drawText(juce::String::formatted("Hybrid 3D Takens  |  X:L(t)  Y:R(t)  Z:L(t-τ)  |  Z-Depth τ = %d", delaySamples),
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