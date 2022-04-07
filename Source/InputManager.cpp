/*
  ==============================================================================

    InputManager.cpp
    Created: 21 Mar 2022 9:46:39am
    Author:  Bennett

  ==============================================================================
*/

#include <JuceHeader.h>
#include "InputManager.h"
#include "MainComponent.h"

#include "FrequencyGraph.cpp"

const juce::Colour enabledColor = juce::Colours::limegreen;
const juce::Colour disabledColor = juce::Colours::navy;

//==============================================================================
InputManager::InputManager() : inputGraph(), frequencyGraph(11)
{
    // File input button
    addAndMakeVisible(fileInputButton);
    fileInputButton.setRadioGroupId(inputRadioButtonID);
    fileInputButton.setClickingTogglesState(true);
    fileInputButton.setButtonText("File Input");
    fileInputButton.onClick = [this]{ onButtonPress(&fileInputButton); };
    //fileInputButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::navy);
    //fileInputButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::darkblue);

    // Microphone input button
    addAndMakeVisible(microphoneInputButton);
    microphoneInputButton.setRadioGroupId(inputRadioButtonID);
    microphoneInputButton.setClickingTogglesState(true);
    microphoneInputButton.setButtonText("Microphone Input");
    microphoneInputButton.onClick = [this] { onButtonPress(&microphoneInputButton); };
    microphoneInputButton.setEnabled(true);
    //microphoneInputButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::navy);

    fileInputButton.setToggleState(true, juce::NotificationType::dontSendNotification);

    // Signal graph
    addAndMakeVisible(inputGraph);
    inputGraph.setTitleText("Input Signal");

    // Frequency visualizer
    addAndMakeVisible(frequencyGraph);
    frequencyGraph.setTitleText("Input Magnitude Spectrum");


    // Gain Slider
    addAndMakeVisible(gainSlider);
    gainSlider.setRange(0.f, 2.f, 0.05f);
    gainSlider.setValue(1.f);
    gainSlider.onValueChange = [this] { onGainSliderChange(); };
    addAndMakeVisible(gainLabel);
    gainLabel.setText("Input Gain", juce::dontSendNotification);
    gainLabel.setFont(juce::Font(14.f, juce::Font::bold));
    gainLabel.setColour(Label::ColourIds::textColourId, juce::Colours::white);
    gainLabel.setJustificationType(Justification::centred);

    // Mute output 
    addAndMakeVisible(muteOutputButton);
    muteOutputButton.setButtonText("Mute Output");
    muteOutputButton.onClick = [this] { onButtonPress(&muteOutputButton); };
    
    addAndMakeVisible(latencyLabel);
    latencyLabel.setText("Latency: 0.0 ms", juce::NotificationType::dontSendNotification);
    latencyLabel.setJustificationType(juce::Justification::centredLeft);


    addAndMakeVisible(fileManager);
    addChildComponent(microphoneManager);
}

InputManager::~InputManager()
{
}

void InputManager::initialize(float rate, MainComponent* parent)
{
    mainComponent = parent;

    fileManager.initialize(rate, parent);
    microphoneManager.initialize(rate, parent);
}

void InputManager::processBuffer(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (inputType == InputType::FileInput)
    {
        fileManager.processBuffer(bufferToFill);

        inputGraph.graph.setPosition(fileManager.getBufferPosition());
    }
    else if (inputType == InputType::MicrophoneInput)
    {
        microphoneManager.processBuffer(bufferToFill);
    }

    bufferToFill.buffer->applyGain(inputGain);

    // Draw frequency graph
    frequencyGraph.processBuffer(bufferToFill);

}

void InputManager::paint (juce::Graphics& g)
{
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    g.setColour (juce::Colours::white);
    g.setFont (26.0f);
    //g.drawText ("Input", getLocalBounds(),
    //            juce::Justification::centredTop, true);   // draw some placeholder text
}

void InputManager::resized()
{
    int width = getWidth();
    int height = getHeight();

    fileInputButton.setBounds((int)(width / 15.f), height * 0.02f, width * 0.2f, height * 0.05f);
    microphoneInputButton.setBounds((int)(width / 15.f), height * 0.08f, width * 0.2f, height * 0.05f);

    fileManager.setBounds(0, (int)(height * 0.15f), width, (int)(height * 0.15f));
    microphoneManager.setBounds(0, (int)(height * 0.15f), width, (int)(height * 0.15f));
   
    inputGraph.setBounds(0, height / 3.f, width, height / 3.f);
    frequencyGraph.setBounds(0, height * (2.f / 3.f), width, height / 3.f);

    gainLabel.setBounds(width * 0.3f, height * 0.01f, width * 0.4f, height * 0.075f);
    gainSlider.setBounds(width * 0.3f, height * 0.05f, width * 0.4f, height * 0.075f);

    muteOutputButton.setBounds(width * 0.7f, height * 0.02f, width * 0.25f, height * 0.05f);
    latencyLabel.setBounds(width * 0.7f, height * 0.08f, width * 0.25f, height * 0.05f);
}

void InputManager::onButtonPress(juce::Button* button)
{
    if (button == &fileInputButton && button->getToggleState())
    {
        //fileInputButton.setColour(juce::TextButton::ColourIds::buttonColourId, enabledColor);
        //microphoneInputButton.setColour(juce::TextButton::ColourIds::buttonColourId, disabledColor);

        fileManager.setVisible(true);
        microphoneManager.setVisible(false);

        inputType = InputType::FileInput;
        mainComponent->speechEnhancer.onModeChange(inputType);
        fileManager.setBufferPosition(0);
    }
    else if (button == &microphoneInputButton && button->getToggleState())
    {
        //fileInputButton.setColour(juce::TextButton::ColourIds::buttonColourId, disabledColor);
        //microphoneInputButton.setColour(juce::TextButton::ColourIds::buttonColourId, enabledColor);

        fileManager.setVisible(false);
        microphoneManager.setVisible(true);

        inputType = InputType::MicrophoneInput;
        mainComponent->speechEnhancer.onModeChange(inputType);
    }
    else if (button == &muteOutputButton)
    {
        muteOutput = muteOutputButton.getToggleState();
    }
    

}

void InputManager::onGainSliderChange()
{
    inputGain = gainSlider.getValue();
}


void InputManager::setInputGraph(juce::AudioSampleBuffer* buffer)
{
    inputGraph.graph.setDataForGraph(*buffer, true, buffer->getNumSamples(), 1.f, 0, buffer->getNumSamples(), buffer->getNumSamples(), mainComponent->getSampleRate());
    inputGraph.graph.updateGraph = true;
    inputGraph.repaint();
}

void InputManager::setLatency(float latency)
{
    std::stringstream latencyText;
    latencyText << std::fixed << std::setprecision(2) << "Latency: " << latency << " ms";

    latencyLabel.setText(latencyText.str(), juce::NotificationType::dontSendNotification);
}
