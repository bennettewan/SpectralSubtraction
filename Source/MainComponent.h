#pragma once

#include <JuceHeader.h>
#include "SpeechEnhancer.h"
#include "InputManager.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    float getSampleRate() const { return rate; }

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    InputManager inputManager;
    SpeechEnhancer speechEnhancer;
private:
    float rate = 48000.f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


void saveWavFile(juce::AudioSampleBuffer* buffer, const std::string& name, double sampleRate);