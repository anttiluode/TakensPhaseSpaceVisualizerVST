#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class TakensEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TakensEditor(TakensProcessor&);
    ~TakensEditor() override;

    void paint   (juce::Graphics&) override;
    void resized ()                override;
    void timerCallback()           override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    TakensProcessor& processor;

    juce::Slider sTau1, sTau2, sTau3, sGain, sZScale, sZRot;
    juce::Label  lTau1, lTau2, lTau3, lGain, lZScale, lZRot;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        aTau1, aTau2, aTau3, aGain, aZScale, aZRot;

    juce::ComboBox modeCombo;
    juce::Label    modeLabel;

    juce::ComboBox pointsCombo;
    juce::Label    pointsLabel;
    int displayPoints = 3000;

    juce::Label rmsLabel;

    // Mouse rotation
    float rotX = 0.38f, rotY = 0.0f;
    juce::Point<int> lastMousePos;

    void setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name);
    void updateDisplayPoints();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TakensEditor)
};