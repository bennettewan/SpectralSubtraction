/*
  ==============================================================================

    MicrophoneManager.cpp
    Created: 21 Mar 2022 9:49:37am
    Author:  Bennett

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MicrophoneManager.h"
#include "MainComponent.h"
#include "SpeechEnhancer.h"

//==============================================================================
MicrophoneManager::MicrophoneManager()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.

    addAndMakeVisible(muteButton);
    muteButton.setButtonText("Mute Microphone");
    muteButton.onClick = [this] { onButtonPress(&muteButton); };

    addAndMakeVisible(subtractionButton);
    subtractionButton.setButtonText("Apply Subtraction");
    subtractionButton.onClick = [this] { onButtonPress(&subtractionButton); };

}

MicrophoneManager::~MicrophoneManager()
{
}

void MicrophoneManager::paint (juce::Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    //g.setColour (juce::Colours::white);
    //g.setFont (14.0f);
    //g.drawText ("MicrophoneManager", getLocalBounds(),
    //            juce::Justification::centred, true);   // draw some placeholder text
}

void MicrophoneManager::resized()
{
    int width = getWidth();
    int height = getHeight();

    muteButton.setBounds((width / 2.f) - (width / 8), height / 4, width / 4.f, height / 2.f);

    //subtractionButton.setBounds(width / 4.f, 0, width / 12.f, height);



}

void MicrophoneManager::initialize(float rate, MainComponent* parent)
{
    mainComponent = parent;
}

void MicrophoneManager::processBuffer(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (microphoneState == MicrophoneState::Muted)
    {
        bufferToFill.clearActiveBufferRegion();
    }
}

void MicrophoneManager::onSliderChange(juce::Slider* slider)
{

}


void MicrophoneManager::onButtonPress(juce::Button* button)
{
    if (button == &muteButton)
    {
        microphoneState = button->getToggleState() ? MicrophoneState::Muted : MicrophoneState::Open;
    }
    else if (button == &subtractionButton)
    {
        bool state = button->getToggleState();
        mainComponent->speechEnhancer.setEnabled(state);
    }

}
