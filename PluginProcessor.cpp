#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout TakensProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "delay_tau", "Delay τ (ms)", 
        juce::NormalisableRange<float>(1.0f, 80.0f, 0.1f, 0.5f), 
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
    this->sampleRate = sampleRate;
    int delaySamples = (int)(sampleRate * 0.1); // 100ms max
    delayBuffer.assign(delaySamples + 10, 0.0f);
    writeIndex = 0;
    
    (void)samplesPerBlock; // suppress unused parameter warning
}

void TakensProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    float delayMs = apvts.getRawParameterValue("delay_tau")->load();
    float inputGain = apvts.getRawParameterValue("input_gain")->load();
    
    int delaySamples = (int)(delayMs * sampleRate / 1000.0f);
    
    // Avoid division by zero or negative delay
    if (delaySamples < 1) delaySamples = 1;
    if (delaySamples >= (int)delayBuffer.size()) delaySamples = (int)delayBuffer.size() - 1;
    
    // Update RMS
    float rmsSum = 0.0f;
    int totalSamples = 0;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            float input = channelData[s] * inputGain;
            
            // Read delayed value for embedding
            int readIndex = writeIndex - delaySamples;
            if (readIndex < 0) readIndex += (int)delayBuffer.size();
            
            float delayed = delayBuffer[readIndex];
            
            // Store current sample
            delayBuffer[writeIndex] = input;
            writeIndex = (writeIndex + 1) % (int)delayBuffer.size();
            
            // Send to GUI for visualization (channel 0 only)
            if (ch == 0 && s % 4 == 0) {
                scopeX.store(input);
                scopeY.store(delayed);
            }
            
            // Pass audio through unchanged (we're just analyzing)
            channelData[s] = input;
            
            // Accumulate for RMS
            rmsSum += input * input;
            totalSamples++;
        }
    }
    
    // Calculate and store RMS
    if (totalSamples > 0) {
        float rms = std::sqrt(rmsSum / totalSamples);
        rmsLevel.store(rms);
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