#include "PluginEditor.h"

static const juce::Colour COL_BG       { 0xff0a0a1a };
static const juce::Colour COL_PANEL    { 0xff0d0d20 };
static const juce::Colour COL_TRACE    { 0xff00ffcc };   
static const juce::Colour COL_OLD      { 0xff001166 };   
static const juce::Colour COL_KNOB     { 0xff44ffaa };

static const char* MODE_COMBO_NAMES[] = {
    "L Mono",
    "Stereo Hybrid",
    "Binoc Sum/Diff",
    "Deep L+R+Diff"
};

static const char* MODE_AXIS_LABELS[] = {
    "X=L(t)   Y=L(t-t1)   Z=L(t-t3)   [Mono Phase Space]",
    "X=L(t)   Y=R(t)      Z=L(t-t3)   [Stereo Hybrid]",
    "X=Sum(t) Y=Diff(t)   Z=Sum(t-t3) [Binocular Sum+Difference]",
    "X=L(t)   Y=R(t)      Z=(L-R)(t-t3) [Deep L+R+DiffTau]"
};

TakensEditor::TakensEditor(TakensProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(900, 700);
    setResizable(true, true);
    setResizeLimits(600, 500, 1600, 1200);

    // FIX: Set the default camera angle to an Isometric 3D view
    rotX = 0.45f;
    rotY = -0.65f; 

    setupSlider(sTau1, lTau1, "t1 (X/Y)");
    setupSlider(sTau2, lTau2, "t2 (Mono Y)");
    setupSlider(sTau3, lTau3, "t3 (Depth Z)"); 
    setupSlider(sGain, lGain, "Input Gain");

    aTau1 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "delay_tau", sTau1);
    aTau2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "delay_tau2", sTau2);
    aTau3 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "delay_tau3", sTau3);
    aGain = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "input_gain", sGain);

    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    modeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modeLabel);

    for (int i = 0; i < 4; ++i)
        modeCombo.addItem(MODE_COMBO_NAMES[i], i + 1);

    int m = (int)p.apvts.getRawParameterValue("embed_mode")->load();
    modeCombo.setSelectedId(m + 1, juce::dontSendNotification);
    modeCombo.onChange = [this]() {
        processor.apvts.getParameter("embed_mode")->setValueNotifyingHost(
            (float)(modeCombo.getSelectedId() - 1) / 3.0f);
    };
    addAndMakeVisible(modeCombo);

    pointsLabel.setText("Points", juce::dontSendNotification);
    pointsLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    pointsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pointsLabel);

    pointsCombo.addItem("1000", 1);
    pointsCombo.addItem("3000", 2);
    pointsCombo.addItem("5000", 3);
    pointsCombo.addItem("8000", 4);
    pointsCombo.setSelectedId(2, juce::dontSendNotification);
    pointsCombo.onChange = [this]() { updateDisplayPoints(); };
    addAndMakeVisible(pointsCombo);

    rmsLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(rmsLabel);

    startTimerHz(30);
}

TakensEditor::~TakensEditor() {}

void TakensEditor::setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    s.setColour(juce::Slider::rotarySliderFillColourId, COL_KNOB);
    s.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setColour(juce::Label::textColourId, juce::Colours::grey);
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void TakensEditor::updateDisplayPoints()
{
    switch (pointsCombo.getSelectedId()) {
        case 1: displayPoints = 1000; break;
        case 2: displayPoints = 3000; break;
        case 3: displayPoints = 5000; break;
        case 4: displayPoints = 8000; break;
    }
}

void TakensEditor::timerCallback()
{
    float rms = processor.rmsLevel.load();
    rmsLabel.setText(juce::String::formatted("RMS: %.3f", rms), juce::dontSendNotification);
    repaint();
}

void TakensEditor::mouseDown(const juce::MouseEvent& e) { lastMousePos = e.getPosition(); }

void TakensEditor::mouseDrag(const juce::MouseEvent& e)
{
    auto delta = e.getPosition() - lastMousePos;
    rotY += delta.x * 0.01f;
    rotX += delta.y * 0.01f;
    lastMousePos = e.getPosition();
}

void TakensEditor::paint(juce::Graphics& g)
{
    g.fillAll(COL_BG);

    auto plotArea = getLocalBounds().reduced(20, 20).withBottom(getHeight() - 140);
    g.setColour(COL_PANEL);
    g.fillRoundedRectangle(plotArea.toFloat(), 8.0f);

    float cx = plotArea.getCentreX();
    float cy = plotArea.getCentreY();
    float scale = juce::jmin(plotArea.getWidth(), plotArea.getHeight()) * 0.40f;

    int t1 = (int)processor.apvts.getRawParameterValue("delay_tau")->load();
    int t2 = (int)processor.apvts.getRawParameterValue("delay_tau2")->load();
    int t3 = (int)processor.apvts.getRawParameterValue("delay_tau3")->load();
    int mode = (int)processor.apvts.getRawParameterValue("embed_mode")->load();
    int currentIdx = processor.writeIndex.load(std::memory_order_relaxed);

    struct RawPoint { float x, y, z; };
    std::vector<RawPoint> rawPoints;
    rawPoints.reserve(displayPoints);

    float maxAmp = 0.0001f;

    for (int i = 0; i < displayPoints; ++i)
    {
        int idx0 = (currentIdx - 1 - i + TakensProcessor::bufferSize) % TakensProcessor::bufferSize;
        int idx1 = (idx0 - t1 + TakensProcessor::bufferSize) % TakensProcessor::bufferSize;
        int idx2 = (idx0 - t2 + TakensProcessor::bufferSize) % TakensProcessor::bufferSize;
        int idx3 = (idx0 - t3 + TakensProcessor::bufferSize) % TakensProcessor::bufferSize;

        float x = 0.0f, y = 0.0f, z = 0.0f;
        float L0 = processor.ringBufferL[idx0];
        float R0 = processor.ringBufferR[idx0];
        float L1 = processor.ringBufferL[idx1];
        float R1 = processor.ringBufferR[idx1];
        float L3 = processor.ringBufferL[idx3]; 
        float R3 = processor.ringBufferR[idx3];

        switch (mode) {
            case 0: x = L0; y = L1; z = L3; break;
            case 1: x = L0; y = R0; z = L3; break;
            case 2: x = L0 + R0; y = L0 - R0; z = L3 + R3; break;
            case 3: x = L0; y = R0; z = L3 - R3; break;
        }

        maxAmp = juce::jmax(maxAmp, std::abs(x), std::abs(y), std::abs(z));
        rawPoints.push_back({x, y, z});
    }

    struct Point3D {
        float zDepth, screenX, screenY, size;
        juce::Colour col;
    };
    std::vector<Point3D> points;
    points.reserve(displayPoints);

    // FIX: Push camera closer to exaggerate 3D depth
    float cameraDistance = 2.4f; 
    float fovScale = 2.0f;
    float autoZoom = 1.1f / maxAmp;

    for (int i = 0; i < displayPoints; ++i)
    {
        float x = rawPoints[i].x * autoZoom;
        float y = rawPoints[i].y * autoZoom;
        float z = rawPoints[i].z * autoZoom;

        float x1 = x * std::cos(rotY) - z * std::sin(rotY);
        float z1 = x * std::sin(rotY) + z * std::cos(rotY);
        float y1 = y * std::cos(rotX) - z1 * std::sin(rotX);
        float z2 = y * std::sin(rotX) + z1 * std::cos(rotX);

        float w = 1.0f / (cameraDistance - z2); 
        float screenX = cx + (x1 * w * scale * fovScale);
        float screenY = cy - (y1 * w * scale * fovScale); 
        
        // FIX: Make the dots scale much larger when close to camera
        float dotSize = juce::jmax(0.5f, 12.0f * w); 

        // FIX: Add "Depth Fog" - further away gets darker
        float brightness = juce::jlimit(0.2f, 1.0f, w * 1.5f);
        float ratio = (float)i / (float)displayPoints;
        juce::Colour col = COL_TRACE.interpolatedWith(COL_OLD, ratio).withMultipliedBrightness(brightness);

        if (plotArea.contains(screenX, screenY)) {
            points.push_back({ z2, screenX, screenY, dotSize, col });
        }
    }

    std::sort(points.begin(), points.end(), [](const Point3D& a, const Point3D& b) {
        return a.zDepth < b.zDepth;
    });

    auto drawAxis = [&](float x, float y, float z, juce::Colour col) {
        float x1 = x * std::cos(rotY) - z * std::sin(rotY);
        float z1 = x * std::sin(rotY) + z * std::cos(rotY);
        float y1 = y * std::cos(rotX) - z1 * std::sin(rotX);
        float z2 = y * std::sin(rotX) + z1 * std::cos(rotX);
        float w = 1.0f / (cameraDistance - z2);
        float sx = cx + (x1 * w * scale * fovScale);
        float sy = cy - (y1 * w * scale * fovScale);
        g.setColour(col.withAlpha(0.6f)); // Make axes a bit more visible
        g.drawLine(cx, cy, sx, sy, 3.0f);
        g.fillEllipse(sx - 3, sy - 3, 6, 6); // Add a tip to the axis
    };

    drawAxis(1.3f, 0, 0, juce::Colours::red);   
    drawAxis(0, 1.3f, 0, juce::Colours::green); 
    drawAxis(0, 0, 1.3f, juce::Colours::blue);  

    for (const auto& p : points) {
        g.setColour(p.col);
        g.fillEllipse(p.screenX - (p.size * 0.5f), p.screenY - (p.size * 0.5f), p.size, p.size);
    }

    g.setColour(juce::Colours::grey);
    g.setFont(14.0f);
    g.drawText(MODE_AXIS_LABELS[mode], plotArea.getX(), plotArea.getY() + 10, plotArea.getWidth(), 20, juce::Justification::centred);
}

void TakensEditor::resized()
{
    const int bottomY = getHeight() - 128;
    const int labelH  = 18;
    const int knobH   = 88;
    const int comboH  = 28;
    
    constexpr int nItems = 6;
    const int itemW   = 85; 
    const int gap = (getWidth() - nItems * itemW) / (nItems + 1);

    auto xOf = [&](int i) -> int { return gap + i * (itemW + gap); };

    lTau1.setBounds(xOf(0), bottomY, itemW, labelH);
    sTau1.setBounds(xOf(0), bottomY + labelH, itemW, knobH);

    lTau2.setBounds(xOf(1), bottomY, itemW, labelH);
    sTau2.setBounds(xOf(1), bottomY + labelH, itemW, knobH);

    lTau3.setBounds(xOf(2), bottomY, itemW, labelH);
    sTau3.setBounds(xOf(2), bottomY + labelH, itemW, knobH);

    lGain.setBounds(xOf(3), bottomY, itemW, labelH);
    sGain.setBounds(xOf(3), bottomY + labelH, itemW, knobH);

    modeLabel.setBounds(xOf(4), bottomY, itemW, labelH);
    modeCombo.setBounds(xOf(4), bottomY + labelH, itemW, comboH);

    pointsLabel.setBounds(xOf(5), bottomY, itemW, labelH);
    pointsCombo.setBounds(xOf(5), bottomY + labelH, itemW, comboH);

    rmsLabel.setBounds(10, 10, 200, 20);
}