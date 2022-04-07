/*
  ==============================================================================

    MicrophoneManager.h
    Created: 21 Mar 2022 9:49:37am
    Author:  Bennett

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class MainComponent;

enum class MicrophoneState
{
    Open,
    Muted
};

//==============================================================================
/*
*/
class MicrophoneManager  : public juce::Component
{
public:
    MicrophoneManager();
    ~MicrophoneManager() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void initialize(float rate, MainComponent* parent);
    void processBuffer(const juce::AudioSourceChannelInfo& bufferToFill);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrophoneManager)
    MainComponent* mainComponent;

    MicrophoneState microphoneState  = MicrophoneState::Open;

    juce::ToggleButton muteButton;

    juce::ToggleButton subtractionButton;

    void onSliderChange(juce::Slider* slider);
    void onButtonPress(juce::Button* button);
};
