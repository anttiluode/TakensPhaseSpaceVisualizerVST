#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout TakensProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "delay_tau", "Delay τ (samples)", 
        juce::NormalisableRange<float>(1.0f, 200.0f, 1.0f, 1.0f), 
        25.0f));
    
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
    std::fill(ringBufferL.begin(), ringBufferL.end(), 0.0f);
    std::fill(ringBufferR.begin(), ringBufferR.end(), 0.0f);
}

void TakensProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    float inputGain = apvts.getRawParameterValue("input_gain")->load();
    float rmsSum = 0.0f;
    
    int numSamples = buffer.getNumSamples();
    int channels = buffer.getNumChannels();
    
    // Read pointers
    const float* channelDataL = buffer.getReadPointer(0);
    const float* channelDataR = (channels > 1) ? buffer.getReadPointer(1) : channelDataL;
    
    for (int s = 0; s < numSamples; ++s) {
        float inputL = channelDataL[s] * inputGain;
        float inputR = channelDataR[s] * inputGain;
        
        int currentIdx = writeIndex.load(std::memory_order_relaxed);
        ringBufferL[(size_t)currentIdx] = inputL;
        ringBufferR[(size_t)currentIdx] = inputR;
        
        writeIndex.store((currentIdx + 1) % bufferSize, std::memory_order_relaxed);
        rmsSum += (inputL * inputL + inputR * inputR) / 2.0f;
    }
    
    if (numSamples > 0) {
        rmsLevel.store(std::sqrt(rmsSum / numSamples));
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