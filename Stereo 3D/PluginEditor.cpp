#include "PluginEditor.h"

// ─── Colour palette ──────────────────────────────────────────────────────────
static const juce::Colour COL_BG       { 0xff0a0a1a };
static const juce::Colour COL_PANEL    { 0xff0d0d20 };
static const juce::Colour COL_GRID     { 0xff1a1a3a };
static const juce::Colour COL_TRACE    { 0xff00ffcc };   // cyan newest
static const juce::Colour COL_OLD      { 0xff001166 };   // deep blue oldest
static const juce::Colour COL_KNOB     { 0xff44ffaa };

// ─── Mode metadata ───────────────────────────────────────────────────────────
static const char* MODE_COMBO_NAMES[] = {
    "L mono Takens",
    "Stereo hybrid",
    "Binocular Sum/Diff",
    "Deep L+R+DiffTau"
};

static const char* MODE_AXIS_LABELS[] = {
    "X=L(t)   Y=L(t-t1)   Z=L(t-t2)   [pure single-channel Takens]",
    "X=L(t)   Y=R(t)      Z=L(t-t1)   [stereo phase space]",
    "X=Sum(t) Y=Diff(t)   Z=Sum(t-t1) [binocular sum+difference]",
    "X=L(t)   Y=R(t)      Z=Diff(t-t1)[stereo + delayed disparity]"
};

// ─── Constructor ─────────────────────────────────────────────────────────────
TakensEditor::TakensEditor(TakensProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(780, 700);
    setResizable(true, true);
    setResizeLimits(560, 560, 1600, 1600);

    // Knobs
    setupSlider(sTau1, lTau1, "t1 depth");
    setupSlider(sTau2, lTau2, "t2 depth");
    setupSlider(sGain, lGain, "input gain");

    aTau1 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "delay_tau",  sTau1);
    aTau2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "delay_tau2", sTau2);
    aGain = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "input_gain", sGain);

    // Mode combo
    modeLabel.setText("embed mode", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    modeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modeLabel);

    for (int i = 0; i < 4; ++i)
        modeCombo.addItem(MODE_COMBO_NAMES[i], i + 1);
    modeCombo.setSelectedId(2, juce::dontSendNotification);   // default: Stereo hybrid
    modeCombo.onChange = [this]
    {
        // Write to the int parameter manually (no ComboBoxAttachment for AudioParameterInt)
        if (auto* param = processor.apvts.getParameter("embed_mode"))
            param->setValueNotifyingHost(
                param->convertTo0to1((float)(modeCombo.getSelectedId() - 1)));
    };
    modeCombo.setColour(juce::ComboBox::backgroundColourId, COL_GRID);
    modeCombo.setColour(juce::ComboBox::textColourId,       COL_TRACE);
    modeCombo.setColour(juce::ComboBox::arrowColourId,      COL_TRACE);
    addAndMakeVisible(modeCombo);

    // Display-points combo
    pointsLabel.setText("display pts", juce::dontSendNotification);
    pointsLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    pointsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(pointsLabel);

    for (int n : { 500, 1000, 2000, 3000, 4000, 8000 })
        pointsCombo.addItem(juce::String(n), n);
    pointsCombo.setSelectedId(3000, juce::dontSendNotification);
    pointsCombo.onChange = [this] { displayPoints = pointsCombo.getSelectedId(); };
    pointsCombo.setColour(juce::ComboBox::backgroundColourId, COL_GRID);
    pointsCombo.setColour(juce::ComboBox::textColourId,       COL_TRACE);
    pointsCombo.setColour(juce::ComboBox::arrowColourId,      COL_TRACE);
    addAndMakeVisible(pointsCombo);

    // RMS readout
    rmsLabel.setText("RMS: 0.0000", juce::dontSendNotification);
    rmsLabel.setColour(juce::Label::textColourId, COL_TRACE);
    rmsLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(rmsLabel);

    startTimerHz(30);
}

TakensEditor::~TakensEditor() { stopTimer(); }

// ─── Helpers ─────────────────────────────────────────────────────────────────
void TakensEditor::setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name)
{
    addAndMakeVisible(s);
    addAndMakeVisible(l);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    s.setColour(juce::Slider::thumbColourId,           COL_KNOB);
    s.setColour(juce::Slider::rotarySliderFillColourId, COL_TRACE);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, COL_GRID);
    l.setText(name, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, juce::Colours::grey);
}

// ─── Timer: sync UI state ─────────────────────────────────────────────────────
void TakensEditor::timerCallback()
{
    rmsLabel.setText(juce::String::formatted("RMS: %.4f", processor.rmsLevel.load()),
                     juce::dontSendNotification);

    // Keep mode combo in sync if parameter changed externally (e.g. DAW automation)
    const int paramMode = (int)processor.apvts.getRawParameterValue("embed_mode")->load();
    if (modeCombo.getSelectedId() - 1 != paramMode)
        modeCombo.setSelectedId(paramMode + 1, juce::dontSendNotification);

    repaint();
}

// ─── Mouse: 3-D rotation ─────────────────────────────────────────────────────
void TakensEditor::mouseDown(const juce::MouseEvent& e) { lastMousePos = e.getPosition(); }

void TakensEditor::mouseDrag(const juce::MouseEvent& e)
{
    rotY += (e.getPosition().x - lastMousePos.x) * 0.01f;
    rotX += (e.getPosition().y - lastMousePos.y) * 0.01f;
    lastMousePos = e.getPosition();
}

// ─── Paint ───────────────────────────────────────────────────────────────────
void TakensEditor::paint(juce::Graphics& g)
{
    g.fillAll(COL_BG);

    // Define plot area
    auto plotArea = getLocalBounds().reduced(20, 20).withBottom(getHeight() - 140);
    g.setColour(COL_PANEL);
    g.fillRoundedRectangle(plotArea.toFloat(), 8.0f);

    float cx = plotArea.getCentreX();
    float cy = plotArea.getCentreY();
    float scale = juce::jmin(plotArea.getWidth(), plotArea.getHeight()) * 0.35f;

    // Get parameters
    int t1 = (int)processor.apvts.getRawParameterValue("delay_tau")->load();
    int t2 = (int)processor.apvts.getRawParameterValue("delay_tau2")->load();
    int mode = (int)processor.apvts.getRawParameterValue("embed_mode")->load();
    int currentIdx = processor.writeIndex.load(std::memory_order_relaxed);

    // Struct for storing raw points before zoom
    struct RawPoint { float x, y, z; };
    std::vector<RawPoint> rawPoints;
    rawPoints.reserve(displayPoints);

    // --- STEP 1: Gather raw points and find peak amplitude for Autoscaling ---
    float maxAmp = 0.0001f; // Prevent divide-by-zero

    for (int i = 0; i < displayPoints; ++i)
    {
        int idx0 = (currentIdx - 1 - i + TakensProcessor::bufferSize) % TakensProcessor::bufferSize;
        int idx1 = (idx0 - t1 + TakensProcessor::bufferSize) % TakensProcessor::bufferSize;
        int idx2 = (idx0 - t2 + TakensProcessor::bufferSize) % TakensProcessor::bufferSize;

        float x = 0.0f, y = 0.0f, z = 0.0f;
        float L0 = processor.ringBufferL[idx0];
        float R0 = processor.ringBufferR[idx0];

        // Apply embedding modes
        if (mode == 0) { 
            x = L0; y = processor.ringBufferL[idx1]; z = processor.ringBufferL[idx2]; 
        } else if (mode == 1) { 
            x = L0; y = R0; z = processor.ringBufferL[idx1]; 
        } else if (mode == 2) { 
            x = L0 + R0; y = L0 - R0; z = processor.ringBufferL[idx1] + processor.ringBufferR[idx1]; 
        } else if (mode == 3) { 
            x = L0; y = R0; z = processor.ringBufferL[idx1] - processor.ringBufferR[idx1]; 
        }

        // Track the largest coordinate to calculate our dynamic zoom
        maxAmp = juce::jmax(maxAmp, std::abs(x), std::abs(y), std::abs(z));
        rawPoints.push_back({x, y, z});
    }

    // --- STEP 2: Project points with AutoZoom ---
    struct Point3D {
        float zDepth, screenX, screenY, size;
        juce::Colour col;
    };
    std::vector<Point3D> points;
    points.reserve(displayPoints);

    float cameraDistance = 3.0f;
    float fovScale = 2.0f;
    
    // This multiplier forces the shape to expand up to 1.2, perfectly filling the panel
    float autoZoom = 1.2f / maxAmp;

    for (int i = 0; i < displayPoints; ++i)
    {
        // Apply the dynamic zoom
        float x = rawPoints[i].x * autoZoom;
        float y = rawPoints[i].y * autoZoom;
        float z = rawPoints[i].z * autoZoom;

        // 3D Rotation
        float x1 = x * std::cos(rotY) - z * std::sin(rotY);
        float z1 = x * std::sin(rotY) + z * std::cos(rotY);
        float y1 = y * std::cos(rotX) - z1 * std::sin(rotX);
        float z2 = y * std::sin(rotX) + z1 * std::cos(rotX);

        // Perspective Divide
        float w = 1.0f / (cameraDistance - z2); 
        float screenX = cx + (x1 * w * scale * fovScale);
        float screenY = cy - (y1 * w * scale * fovScale); 
        float dotSize = juce::jmax(0.5f, 5.0f * w); 

        // Color gradient
        float ratio = (float)i / (float)displayPoints;
        juce::Colour col = COL_TRACE.interpolatedWith(COL_OLD, ratio);

        if (plotArea.contains(screenX, screenY)) {
            points.push_back({ z2, screenX, screenY, dotSize, col });
        }
    }

    // --- STEP 3: Painter's Algorithm (Depth Sort) ---
    std::sort(points.begin(), points.end(), [](const Point3D& a, const Point3D& b) {
        return a.zDepth < b.zDepth;
    });

    // --- STEP 4: Draw Axes and Dots ---
    auto drawAxis = [&](float x, float y, float z, juce::Colour col) {
        float x1 = x * std::cos(rotY) - z * std::sin(rotY);
        float z1 = x * std::sin(rotY) + z * std::cos(rotY);
        float y1 = y * std::cos(rotX) - z1 * std::sin(rotX);
        float z2 = y * std::sin(rotX) + z1 * std::cos(rotX);
        float w = 1.0f / (cameraDistance - z2);
        float sx = cx + (x1 * w * scale * fovScale);
        float sy = cy - (y1 * w * scale * fovScale);
        
        g.setColour(col.withAlpha(0.3f));
        g.drawLine(cx, cy, sx, sy, 2.0f);
    };

    // Draw the reference axes slightly outside the max zoom bounds
    drawAxis(1.4f, 0, 0, juce::Colours::red);   
    drawAxis(0, 1.4f, 0, juce::Colours::green); 
    drawAxis(0, 0, 1.4f, juce::Colours::blue);  

    for (const auto& p : points) {
        g.setColour(p.col);
        g.fillEllipse(p.screenX - (p.size * 0.5f), p.screenY - (p.size * 0.5f), p.size, p.size);
    }

    // Draw active mode label
    g.setColour(juce::Colours::grey);
    g.setFont(14.0f);
    g.drawText(MODE_AXIS_LABELS[mode], plotArea.getX(), plotArea.getY() + 10, plotArea.getWidth(), 20, juce::Justification::centred);
}

// ─── Layout ──────────────────────────────────────────────────────────────────
void TakensEditor::resized()
{
    const int bottomY = getHeight() - 128;
    const int labelH  = 18;
    const int knobH   = 88;
    const int comboH  = 28;
    const int itemW   = 90;
    constexpr int nItems = 5;
    const int gap = (getWidth() - nItems * itemW) / (nItems + 1);

    auto xOf = [&](int i) -> int { return gap + i * (itemW + gap); };

    // Three knobs
    lTau1.setBounds(xOf(0), bottomY, itemW, labelH);
    sTau1.setBounds(xOf(0), bottomY + labelH, itemW, knobH);

    lTau2.setBounds(xOf(1), bottomY, itemW, labelH);
    sTau2.setBounds(xOf(1), bottomY + labelH, itemW, knobH);

    lGain.setBounds(xOf(2), bottomY, itemW, labelH);
    sGain.setBounds(xOf(2), bottomY + labelH, itemW, knobH);

    // Mode combo
    modeLabel.setBounds(xOf(3), bottomY,          itemW, labelH);
    modeCombo.setBounds(xOf(3), bottomY + labelH, itemW, comboH);

    // Display-points combo
    pointsLabel.setBounds(xOf(4), bottomY,          itemW, labelH);
    pointsCombo.setBounds(xOf(4), bottomY + labelH, itemW, comboH);

    // RMS top-right
    rmsLabel.setBounds(getWidth() - 140, 14, 130, 20);
}