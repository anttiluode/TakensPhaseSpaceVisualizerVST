#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class TakensProcessor : public juce::AudioProcessor {
public:
    TakensProcessor();
    ~TakensProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "3D Takens Phase Space"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int bufferSize = 8192;
    std::array<float, bufferSize> ringBufferL { 0.0f }; // Left Channel
    std::array<float, bufferSize> ringBufferR { 0.0f }; // Right Channel
    std::atomic<int> writeIndex { 0 };
    std::atomic<float> rmsLevel { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TakensProcessor)
};