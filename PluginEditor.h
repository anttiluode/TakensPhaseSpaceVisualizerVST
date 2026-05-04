#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <deque>

class TakensEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit TakensEditor(TakensProcessor&);
    ~TakensEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    TakensProcessor& processor;
    
    // UI Controls
    juce::Slider sDelay, sGain;
    juce::Label lDelay, lGain;
    juce::Label rmsLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aDelay, aGain;
    
    // Phase space path
    std::deque<juce::Point<float>> scopePath;
    static constexpr int MAX_SCOPE_PTS = 500;
    
    // Display points control
    juce::ComboBox pointsCombo;
    juce::Label pointsLabel;
    int displayPoints = 2000;
    
    void setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name);
    void updateDisplayPoints();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TakensEditor)
};