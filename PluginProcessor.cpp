#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout TakensProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Changed to Samples to match Python exactly (1 to 200 samples)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "delay_tau", "Delay τ (samples)", 
        juce::NormalisableRange<float>(1.0f, 200.0f, 1.0f, 1.0f), 
        15.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "input_gain", "Input Gain",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f),
        1.0f));
    
    return layout;
}

TakensProcessor::TakensProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout()) {}

juce::AudioProcessorEditor* TakensProcessor::createEditor() {
    return new TakensEditor(*this);
}

void TakensProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
    writeIndex.store(0);
    std::fill(ringBuffer.begin(), ringBuffer.end(), 0.0f);
}

void TakensProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    float inputGain = apvts.getRawParameterValue("input_gain")->load();
    
    float rmsSum = 0.0f;
    int totalSamples = 0;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            float input = channelData[s] * inputGain;
            
            // Only capture Channel 0 (Mono/Left) for the visualizer
            if (ch == 0) {
                int currentIdx = writeIndex.load(std::memory_order_relaxed);
                ringBuffer[(size_t)currentIdx] = input;
                writeIndex.store((currentIdx + 1) % bufferSize, std::memory_order_relaxed);
                
                rmsSum += input * input;
                totalSamples++;
            }
            
            // Pass audio through unchanged (we're just analyzing)
            channelData[s] = input;
        }
    }
    
    if (totalSamples > 0) {
        rmsLevel.store(std::sqrt(rmsSum / totalSamples));
    }
}

void TakensProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void TakensProcessor::setStateInformation(const void* data, int size) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new TakensProcessor();
}