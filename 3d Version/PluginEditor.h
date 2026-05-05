#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class TakensEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit TakensEditor(TakensProcessor&);
    ~TakensEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // Mouse events for 3D rotation
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    TakensProcessor& processor;
    
    juce::Slider sDelay, sGain;
    juce::Label lDelay, lGain;
    juce::Label rmsLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aDelay, aGain;
    
    juce::ComboBox pointsCombo;
    juce::Label pointsLabel;
    int displayPoints = 3000;
    
    // 3D Projection Variables
    float rotX = 0.38f;
    float rotY = 0.0f;
    juce::Point<int> lastMousePos;
    
    void setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name);
    void updateDisplayPoints();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TakensEditor)
};