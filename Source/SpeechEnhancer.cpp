/*
  ==============================================================================

    SpeechEnhancer.cpp
    Created: 7 Feb 2022 12:23:34pm
    Author:  Bennett

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SpeechEnhancer.h"
#include "MainComponent.h"
#include <complex>
#include "FileManager.h"

#include "FrequencyGraph.cpp"


//==============================================================================
SpeechEnhancer::SpeechEnhancer() : spectralSubtraction(11), outputFrequencyGraph(11), noiseEstimateGraph(11), realtimeBuffer(), realtimeOut(),
                                   outputSignal(1)
{
    startTimer(100);

    outputSignal.setBufferSize(512);
    outputSignal.setSamplesPerBlock(128);

    addAndMakeVisible(subtractionFactorSlider);
    subtractionFactorSlider.addListener(this);
    subtractionFactorSlider.setRange(0, 10, 0.05);
    subtractionFactorSlider.setValue(4.0, juce::NotificationType::sendNotification);
    subtractionFactorSlider.setEnabled(true);
    subtractionFactorSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);

    addAndMakeVisible(subtractionFactorLabel);
    subtractionFactorLabel.setText("Over-Subtraction Factor", juce::NotificationType::dontSendNotification);
    subtractionFactorLabel.setJustificationType(juce::Justification::centredTop);

    addAndMakeVisible(spectralFloorSlider);
    spectralFloorSlider.addListener(this);
    spectralFloorSlider.setRange(0, 0.2, 0.001);
    spectralFloorSlider.setValue(0.03, juce::NotificationType::sendNotification);
    spectralFloorSlider.setEnabled(true);
    spectralFloorSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);

    addAndMakeVisible(spectralFloorLabel);
    spectralFloorLabel.setText("Spectral Floor", juce::NotificationType::dontSendNotification);
    spectralFloorLabel.setJustificationType(juce::Justification::centredTop);

    addAndMakeVisible(enabledButton);
    enabledButton.onClick = [this]{ onButtonClick(&enabledButton);};
    enabledButton.setClickingTogglesState(true);
    enabledButton.setToggleState(false, juce::dontSendNotification);
    enabledButton.setButtonText("Enable Subtraction");
    enabledButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::green);
    enabledButton.setEnabled(false);

    addAndMakeVisible(outputFrequencyGraph);
    addAndMakeVisible(noiseSpectrumGraph);
    addAndMakeVisible(noiseEstimateGraph);
    noiseSpectrumGraph.setTitleText("Noise Spectrum");
    noiseSpectrumGraph.setNumFrequencyBands(spectralSubtraction.getNumFrequencyBands());
    outputFrequencyGraph.setTitleText("Output Magnitude Spectrum");
    noiseEstimateGraph.setTitleText("Noise Subtracted");

    addAndMakeVisible(outputSignal);

    // Compute button
    addAndMakeVisible(computeButton);
    computeButton.onClick = [this] { onButtonClick(&computeButton);};
    computeButton.setButtonText("Compute Noise Profile");
    computeButton.setEnabled(false);
    computeButton.setClickingTogglesState(false);
    computeButton.setToggleState(false, juce::dontSendNotification);
    computeButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::green);

    // Adaptive Estimation Button
    addAndMakeVisible(adaptiveEstimationButton);
    adaptiveEstimationButton.onClick = [this] { onButtonClick(&adaptiveEstimationButton); };
    adaptiveEstimationButton.setButtonText("Adaptive Estimation");
    adaptiveEstimationButton.setClickingTogglesState(true);
    adaptiveEstimationButton.setToggleState(false, juce::dontSendNotification);
    adaptiveEstimationButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::green);


    // Window Dropdown
    addAndMakeVisible(windowDropdown);
    windowDropdown.onChange = [this] {onDropdownChange(&windowDropdown);};
    windowDropdown.addItem("Rectangular", 1);
    windowDropdown.addItem("Triangular", 2);
    windowDropdown.addItem("Hann", 3);
    windowDropdown.addItem("Hamming", 4);
    windowDropdown.addItem("Blackman", 5);
    windowDropdown.addItem("Blackman Harris", 6);
    windowDropdown.addItem("Flat Top", 7);
    windowDropdown.addItem("Kaiser", 8);
    windowDropdown.setSelectedItemIndex(3, juce::NotificationType::dontSendNotification);
    addAndMakeVisible(windowLabel);
    windowLabel.setText("Window Type", juce::NotificationType::dontSendNotification);
    windowLabel.setJustificationType(juce::Justification::centredTop);

    // FFT order slider
    addAndMakeVisible(&fftOrderSlider);
    fftOrderSlider.addListener(this);
    fftOrderSlider.setRange(1, 15, 1.0);
    fftOrderSlider.setEnabled(true);
    fftOrderSlider.textFromValueFunction = [](double value)
    {
        return std::to_string(1 << (int)value);
    };
    fftOrderSlider.setValue(11, juce::NotificationType::dontSendNotification);
    fftOrderSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);

    addAndMakeVisible(fftOrderLabel);
    fftOrderLabel.setText("Window Size", juce::NotificationType::dontSendNotification);
    fftOrderLabel.setJustificationType(juce::Justification::centredTop);

    // Noise profile frames slider
    addAndMakeVisible(&noiseProfileFramesSlider);
    noiseProfileFramesSlider.addListener(this);
    noiseProfileFramesSlider.setRange(5, 15, 1.0);
    noiseProfileFramesSlider.setValue(10, juce::NotificationType::dontSendNotification);
    noiseProfileFramesSlider.setEnabled(true);
    noiseProfileFramesSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);

    addAndMakeVisible(noiseProfileFramesLabel);
    noiseProfileFramesLabel.setText("Number of Frames in Noise Estimation", juce::NotificationType::dontSendNotification);
    noiseProfileFramesLabel.setJustificationType(juce::Justification::centredTop);


    // smoothing rate slider
    addAndMakeVisible(&smoothingRateSlider);
    smoothingRateSlider.addListener(this);
    smoothingRateSlider.setRange(1, 6, 0.1);
    smoothingRateSlider.setValue(3, juce::NotificationType::dontSendNotification);
    smoothingRateSlider.setEnabled(true);
    smoothingRateSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    addAndMakeVisible(smoothingRateLabel);
    smoothingRateLabel.setText("Estimation Smoothing Rate", juce::NotificationType::dontSendNotification);
    smoothingRateLabel.setJustificationType(juce::Justification::centredTop);

    // smoothing curve slider
    addAndMakeVisible(&smoothingCurveSlider);
    smoothingCurveSlider.addListener(this);
    smoothingCurveSlider.setRange(1, 6, 0.1);
    smoothingCurveSlider.setValue(3, juce::NotificationType::dontSendNotification);
    smoothingCurveSlider.setEnabled(true);
    smoothingCurveSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    addAndMakeVisible(smoothingCurveLabel);
    smoothingCurveLabel.setText("Estimation Smoothing Shift", juce::NotificationType::dontSendNotification);
    smoothingCurveLabel.setJustificationType(juce::Justification::centredTop);

    // Frequency Bands slider
    addAndMakeVisible(&frequencyBandsSlider);
    frequencyBandsSlider.addListener(this);
    frequencyBandsSlider.setRange(1, 8, 1.0);
    frequencyBandsSlider.setValue(spectralSubtraction.getNumFrequencyBands(), juce::NotificationType::dontSendNotification);
    frequencyBandsSlider.setEnabled(true);
    frequencyBandsSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    addAndMakeVisible(frequencyBandsLabel);
    frequencyBandsLabel.setText("Frequency Bands", juce::NotificationType::dontSendNotification);
    frequencyBandsLabel.setJustificationType(juce::Justification::centredTop);

    for (int i = 0; i < 8; ++i)
    {
        juce::Slider* currSlider = new juce::Slider;
        addAndMakeVisible(*currSlider);
        currSlider->addListener(this);
        currSlider->setRange(0, 3, 0.1);
        currSlider->setValue(1.0, juce::NotificationType::sendNotification);
        currSlider->setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
        currSlider->setEnabled(i < spectralSubtraction.getNumFrequencyBands());
        bandWeightSliders.push_back(currSlider);
    }
    addAndMakeVisible(bandWeightLabel);
    bandWeightLabel.setText("Band Weights", juce::NotificationType::dontSendNotification);
    bandWeightLabel.setJustificationType(juce::Justification::centredTop);



    // Domain
    addAndMakeVisible(&domainDropdown);
    domainDropdown.addItem("Magnitude Spectrum", 1);
    domainDropdown.addItem("Power Spectrum", 2);
    domainDropdown.onChange = [this]{ onDropdownChange(&domainDropdown); };
    domainDropdown.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    addAndMakeVisible(&domainLabel);
    domainLabel.setText("Domain", juce::NotificationType::dontSendNotification);
    domainLabel.setJustificationType(juce::Justification::centredTop);

}

SpeechEnhancer::~SpeechEnhancer()
{
    int numBands = bandWeightSliders.size();
    for (int i = 0; i < numBands; ++i)
    {
        delete bandWeightSliders[i];
    }
}

void SpeechEnhancer::paint (juce::Graphics& g)
{

}

void SpeechEnhancer::resized()
{
    int width = getWidth();
    int height = getHeight();

    enabledButton.setBounds(width * 0.01f, height * 0.01f, width * 0.15, height / 16.f);
    computeButton.setBounds(width * (0.5f / 3.f), height * 0.01f, width * 0.15, height / 16.f);
    adaptiveEstimationButton.setBounds(width * (1.f / 3.f), height * 0.01f, width * 0.15, height / 16.f);

    domainLabel.setBounds(width * 0.01f, height * 0.1f, width * 0.225f, height / 32.f);
    domainDropdown.setBounds(width * 0.01f, height * 0.13f, width * 0.225f, height / 32.f);

    noiseProfileFramesLabel.setBounds(width * 0.25f, height * 0.1f, width * 0.225f, height / 32.f);
    noiseProfileFramesSlider.setBounds(width * 0.25f, height * 0.13f, width * 0.225f, height / 32.f);


    fftOrderLabel.setBounds (width * 0.01f, height * 0.18f, width * 0.225f, height / 32.f);
    fftOrderSlider.setBounds(width * 0.01f, height * 0.2f, width * 0.225f, height / 32.f);
    windowLabel.setBounds(width * 0.25f, height * 0.18f, width * 0.225f, height / 32.f);
    windowDropdown.setBounds(width * 0.25f, height * 0.2f,  width * 0.225f, height / 32.f);
    
    subtractionFactorLabel.setBounds (width * 0.01f, height * 0.28f, width * 0.225f, height / 32.f);
    subtractionFactorSlider.setBounds(width * 0.01f, height * 0.3f, width * 0.225f, height / 32.f);
    
    spectralFloorLabel.setBounds (width * 0.25f, height * 0.28f, width * 0.225f, height / 32.f);
    spectralFloorSlider.setBounds(width * 0.25f, height * 0.3f, width * 0.225f, height / 32.f);

    smoothingRateLabel.setBounds(width * 0.01f, height * 0.38f, width * 0.225f, height / 32.f);
    smoothingRateSlider.setBounds(width * 0.01f, height * 0.4f, width * 0.225f, height / 32.f);
    
    smoothingCurveLabel.setBounds(width * 0.25f, height * 0.38f, width * 0.225f, height / 32.f);
    smoothingCurveSlider.setBounds(width * 0.25f, height * 0.4f, width * 0.225f, height / 32.f);

    frequencyBandsLabel.setBounds(width * 0.01f, height * 0.48f, width * 0.225f, height / 32.f);
    frequencyBandsSlider.setBounds(width * 0.01f, height * 0.5f, width * 0.225f, height / 32.f);


    noiseSpectrumGraph.setBounds  (width / 2.f, 0,                width / 2.f, height / 3.f);
    outputSignal.setBounds        (width / 2.f, height / 3.f,     width / 2.f, height / 3.f);
    outputFrequencyGraph.setBounds(width / 2.f, 2 * height / 3.f, width / 2.f, height / 3.f);
    noiseEstimateGraph.setBounds  (0, 2 * height / 3.f, width / 2.f, height / 3.f);

    float sliderWidth = (width / 4.f) / (float)bandWeightSliders.size();
    for (int i = 0; i < bandWeightSliders.size(); ++i)
    {
        bandWeightSliders[i]->setBounds((width * 0.25f) + i * sliderWidth, height * 0.5f, sliderWidth, height * 0.15f);
    }
    bandWeightLabel.setBounds(width * 0.25f, height * 0.48, width * 0.225f, height / 32.f);

}

void SpeechEnhancer::initialize(float samplingRate, int samplesExpected, MainComponent* parent) 
{
    mainComponent = parent;
    sampleRate = samplingRate;
    spectralSubtraction.setSampleRate(samplingRate);
    noiseEstimateGraph.setSamplingRate(sampleRate);
    noiseSpectrumGraph.setSamplingRate(sampleRate);
    outputFrequencyGraph.setSamplingRate(sampleRate);

    float latency = ((spectralSubtraction.getWindowSize() * 1.5f) / sampleRate) * 1000.f;
    mainComponent->inputManager.setLatency(latency);
}


void SpeechEnhancer::processBuffer(const juce::AudioSourceChannelInfo& bufferToFill)
{

    if (mainComponent->inputManager.getInputType() == InputType::FileInput && mainComponent->inputManager.fileManager.state != TransportState::Playing)
        return;

    if (isRecordingInput)
    {
        for (int i = 0; i < bufferToFill.numSamples; ++i)
        {
            microphoneNoiseProfileBuffer.push_back(bufferToFill.buffer->getSample(0, i));
            if (microphoneNoiseProfileBuffer.size() == spectralSubtraction.getNoiseProfileSize())
            {
                const juce::MessageManagerLock mmLock;
                spectralSubtraction.setAverageNoise(spectralSubtraction.bufferToNoiseProfile(microphoneNoiseProfileBuffer));
                isRecordingInput = false;
                setNoiseEstimationGraph();
                break;
            }
        }
    }


    int numOutputChannels = bufferToFill.buffer->getNumChannels();
    int numSamples = bufferToFill.numSamples;


    processRealtime(bufferToFill);

    outputSignal.pushBuffer(bufferToFill);
    outputFrequencyGraph.processBuffer(bufferToFill);


}


void SpeechEnhancer::processRealtime(const juce::AudioSourceChannelInfo& bufferToFill)
{
    int numOutputChannels = bufferToFill.buffer->getNumChannels();
    int numSamples = bufferToFill.numSamples;
    int windowSize = spectralSubtraction.getWindowSize();
    int hopSize = windowSize / 2;

    InputType inputType = mainComponent->inputManager.getInputType();
    juce::AudioSampleBuffer* noiseBuffer = inputType == InputType::MicrophoneInput ? bufferToFill.buffer : mainComponent->inputManager.fileManager.getBuffer();


    // Add to circular buffer
    for (int i = 0; i < numSamples; ++i)
    {
        if (inputType == InputType::FileInput)
        {
            if (bufferPosition >= noiseBuffer->getNumSamples())
            {
                bufferPosition = 0;
                //frameNumber = 0;
            }
        }

        float sample = inputType == InputType::MicrophoneInput ? noiseBuffer->getSample(0, i) : noiseBuffer->getSample(0, bufferPosition);

        //Start a new frame every hopSize samples
        if (position % hopSize == 0)
        {
            realtimeBuffer.push_back(CircularBuffer<float>(windowSize * 2));
        }

        // Copy sample into every frame that isn't full
        for (int j = 0; j < realtimeBuffer.size(); ++j)
        {
            if (realtimeBuffer[j].size() < windowSize)
                realtimeBuffer[j].enqueue(sample);
        }
        position++;
        bufferPosition++;
    }

    // Process on buffers
    if (realtimeBuffer.size() > 1 && (realtimeBuffer[0].size() == windowSize && realtimeBuffer[1].size() == windowSize))
    {
        std::vector<float> outBuffer(hopSize + windowSize, 0.f);

        // Loop over all frames, only processing full frames
        for (int frame = 0; frame < realtimeBuffer.size(); ++frame)
        {
            CircularBuffer<float>& currFrame = realtimeBuffer[frame];
            if (currFrame.size() < windowSize)
                continue;

            float* fullBuffer = currFrame.getBuffer();

            // Get the active section of the circular buffer
            std::vector<float> tempBuffer = currFrame.toVector();

            // Process subtraction
            spectralSubtraction.processBuffer(&tempBuffer[0], windowSize);

            // Add to output buffer
            for (int k = 0; k < windowSize; ++k)
            {
                outBuffer[(frame * hopSize) + k] += tempBuffer[k];
            }
        }
        // Remove first frame
        realtimeBuffer.pop_front();

        // First frame does not have any overlap over first hopSize samples, but others do which we don't want to add
        int start = (frameNumber == 0) ? 0 : hopSize;
        for (int x = start; x < windowSize; ++x)
        {
            realtimeOut.push_back(outBuffer[x]);
        }
        frameNumber++;
    }



    // Copy circular buffer into output
    if (realtimeOut.size() > 0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = realtimeOut.front();
            realtimeOut.pop_front();
            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                float* outBuffer = bufferToFill.buffer->getWritePointer(channel);
                outBuffer[i] = sample;
            }
            if (realtimeOut.size() == 0)
                break;
        }
    }
}

// Periodically draw the noise estimation graph
void SpeechEnhancer::timerCallback()
{
    setNoiseEstimationGraph();
}


void SpeechEnhancer::onModeChange(InputType inputType)
{
    switch (inputType)
    {
        case InputType::FileInput:
            //computeButton.setButtonText("Compute Output");
            computeButton.setClickingTogglesState(false);
            outputFrequencyGraph.clear();
            if (mainComponent->inputManager.fileManager.isFileLoaded())
            {
                computeButton.setEnabled(true);
                bufferPosition = 0;
                frameNumber = 0;
                mainComponent->inputManager.fileManager.setBufferPosition(0);
            }
            else
            {
                computeButton.setEnabled(false);
            }
            break;
        case InputType::MicrophoneInput:
            computeButton.setClickingTogglesState(true);
            //computeButton.setButtonText("Compute Noise Profile");
            computeButton.setEnabled(true);
            break;
    }


}


void SpeechEnhancer::onFileLoaded()
{
    computeButton.setEnabled(true);
}

void SpeechEnhancer::sliderValueChanged(Slider* slider)
{
    if (slider == &subtractionFactorSlider)
    {
        spectralSubtraction.setSubtractionConstant(slider->getValue());
    }
    else if (slider == &fftOrderSlider)
    {
        spectralSubtraction.setFFTOrder(slider->getValue());
        float latency = ((spectralSubtraction.getWindowSize() * 1.5f) / sampleRate) * 1000.f;
        mainComponent->inputManager.setLatency(latency);
        //setNoiseEstimationGraph();
    }
    else if (slider == &noiseProfileFramesSlider)
    {
        spectralSubtraction.setNoiseProfileFrames(slider->getValue());
        //setNoiseEstimationGraph();
    }
    else if (slider == &smoothingRateSlider)
    {
        spectralSubtraction.setSmoothingRate(slider->getValue());
    }
    else if (slider == &smoothingCurveSlider)
    {
        spectralSubtraction.setSmoothingCurve(slider->getValue());
    }
    else if (slider == &frequencyBandsSlider)
    {
        int num = (int)slider->getValue();
        spectralSubtraction.setNumFrequencyBands(num);
        noiseSpectrumGraph.setNumFrequencyBands(num);
        outputFrequencyGraph.setNumFrequencyBands(num);
        noiseEstimateGraph.setNumFrequencyBands(num);

        for (int i = 0; i < bandWeightSliders.size(); ++i)
        {
            bandWeightSliders[i]->setEnabled(i < spectralSubtraction.getNumFrequencyBands());
        }
    }
    else
    {
        for (int i = 0; i < bandWeightSliders.size(); ++i)
        {
            if (slider == bandWeightSliders[i])
            {
                spectralSubtraction.setBandWeight(i, slider->getValue());
            }
        }

    }
}



void SpeechEnhancer::onButtonClick(juce::Button* button)
{
    if (button == &computeButton)
    {
        InputType inputType = mainComponent->inputManager.getInputType();
        if (inputType == InputType::FileInput)
        {
            if (spectralSubtraction.getAdaptivateEstimationEnabled())
            {
                spectralSubtraction.setNoiseEstimationEnabled(computeButton.getToggleState());
                if (computeButton.getToggleState())
                    spectralSubtraction.resetEstimation();

            }
            else
            {
                juce::AudioSampleBuffer* buffer = mainComponent->inputManager.fileManager.getBuffer();
                if (buffer) {
                    spectralSubtraction.setSignal(buffer);
                    spectralSubtraction.computeFileNoiseProfile();

                }
            }
            bufferPosition = 0;
            frameNumber = 0;
            mainComponent->inputManager.fileManager.setBufferPosition(0);

        }
        else if (inputType == InputType::MicrophoneInput)
        {
            if (spectralSubtraction.getAdaptivateEstimationEnabled()) {
                spectralSubtraction.setNoiseEstimationEnabled(computeButton.getToggleState());
                if (computeButton.getToggleState())
                    spectralSubtraction.resetEstimation();
            }
            else
                isRecordingInput = true;
        }

        enabledButton.setEnabled(true);
    }
    else if (button == &enabledButton)
    {
        isEnabled = enabledButton.getToggleState();
        spectralSubtraction.setSubtractionEnabled(isEnabled);
    }
    else if (button == &adaptiveEstimationButton)
    {
        spectralSubtraction.setAdaptiveEstimationEnabled(adaptiveEstimationButton.getToggleState());
        if (adaptiveEstimationButton.getToggleState()) {
            computeButton.setClickingTogglesState(true);
        }
        else {
            if (computeButton.getToggleState())
            {
                computeButton.setToggleState(false, juce::NotificationType::sendNotification);
            }
            computeButton.setClickingTogglesState(false);
        }
    }
}


void SpeechEnhancer::onDropdownChange(juce::ComboBox* dropdown)
{
    if (dropdown == &windowDropdown)
    {
        spectralSubtraction.setWindowType((Window::WindowingMethod)(dropdown->getSelectedItemIndex()));
    }
    else if (dropdown == &domainDropdown)
    {
        int domain = domainDropdown.getSelectedId();
        spectralSubtraction.setSubtractionDomain(domain);
    }
}

void SpeechEnhancer::setNoiseEstimationGraph()
{
    const Frame* noiseEst = spectralSubtraction.getNoiseEstimation();
    if (noiseEst == nullptr)
        return;

    int size = noiseEst->size();
    noiseSpectrumGraph.addFrequencyData(*noiseEst);
    const Frame& snr = spectralSubtraction.getAPosSNR();
    noiseSpectrumGraph.setSNR(snr);
    const Frame& smoothing = spectralSubtraction.getEstimationSmoothing();
    noiseSpectrumGraph.setSmoothingData(smoothing);

    noiseEstimateGraph.addFrequencyData(spectralSubtraction.getNoiseSubtracted());
}

