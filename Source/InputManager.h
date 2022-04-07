/*
  ==============================================================================

    InputManager.h
    Created: 21 Mar 2022 9:46:39am
    Author:  Bennett

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "FileManager.h"
#include "MicrophoneManager.h"
#include "SignalVisualizer.h"
#include "FrequencyGraph.h"

class MainComponent;

enum InputType
{
    FileInput,
    MicrophoneInput
};


const int inputRadioButtonID = 1001;

//==============================================================================
/*
*/
class InputManager  : public juce::Component
{
public:
    InputManager();
    ~InputManager() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void initialize(float rate, MainComponent* parent);
    void processBuffer(const juce::AudioSourceChannelInfo& bufferToFill);

    InputType getInputType() const { return inputType; }

    void setInputGraph(juce::AudioSampleBuffer* buffer);

    void setLatency(float latency);

    FileManager fileManager;
    MicrophoneManager microphoneManager;
    bool muteOutput = false;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputManager)

    MainComponent* mainComponent;

    InputType inputType = FileInput;

    juce::TextButton fileInputButton;
    juce::TextButton microphoneInputButton;

    juce::ToggleButton muteOutputButton;

    // Gain slider
    juce::Slider gainSlider;
    juce::Label gainLabel;

    // waveform
    SignalVisualizer inputGraph;
    FrequencyGraph<float> frequencyGraph;

    juce::Label latencyLabel;

    float inputGain = 1.f;


    void onButtonPress(juce::Button* button);
    void onGainSliderChange();
};
