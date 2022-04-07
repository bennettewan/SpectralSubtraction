/*
  ==============================================================================

    SpeechEnhancer.h
    Created: 7 Feb 2022 12:23:34pm
    Author:  Bennett

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "SignalVisualizer.h"
#include "SpectralSubtraction.h"
#include "FrequencyGraph.h"
#include "CircularBuffer.h"
#include "SpectrumGraph.h"

enum InputType;

class MainComponent;
//==============================================================================
/*
*/
class SpeechEnhancer  : public juce::Component, public juce::Slider::Listener, private juce::Timer
{
public:
    SpeechEnhancer();
    ~SpeechEnhancer() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void initialize(float samplingRate, int samplesExpected, MainComponent* parent);

    void processBuffer(const juce::AudioSourceChannelInfo& bufferToFill);

    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }

    void onModeChange(InputType inputType);
    void onFileLoaded();

    AudioSampleBuffer* getOutputBuffer() const { return outputBuffer; }

    

    SpectralSubtraction spectralSubtraction;
private:
    MainComponent* mainComponent;
    float sampleRate;

    AudioSampleBuffer* outputBuffer = nullptr;
    std::deque<CircularBuffer<float>> realtimeBuffer;
    std::deque<float> realtimeOut;
    std::vector<float> microphoneNoiseProfileBuffer;

    juce::TextButton enabledButton;
    juce::TextButton computeButton;
    juce::TextButton adaptiveEstimationButton;

    juce::Slider subtractionFactorSlider;
    juce::Label subtractionFactorLabel;
    juce::Slider fftOrderSlider;
    juce::Label fftOrderLabel;
    juce::Slider noiseProfileFramesSlider;
    juce::Label noiseProfileFramesLabel;
    juce::Slider spectralFloorSlider;
    juce::Label spectralFloorLabel;
    juce::Slider smoothingRateSlider;
    juce::Label smoothingRateLabel;
    juce::Slider smoothingCurveSlider;
    juce::Label smoothingCurveLabel;
    juce::Slider frequencyBandsSlider;
    juce::Label frequencyBandsLabel;
    std::vector<juce::Slider*> bandWeightSliders;
    juce::Label bandWeightLabel;

    juce::ComboBox domainDropdown;
    juce::Label domainLabel;


    SpectrumGraph noiseSpectrumGraph;
    FrequencyGraph<float> outputFrequencyGraph;
    FrequencyGraph<double> noiseEstimateGraph;
    AudioVisualiserComponent outputSignal;

    juce::ComboBox windowDropdown;
    juce::Label windowLabel;
    bool isRecordingInput = false;
    int position = 0;
    int bufferPosition = 0;
    int frameNumber = 0;


    bool isEnabled = false;
    bool isUpdatingNoiseProfile = false;
    

    void setNoiseEstimationGraph();

    void sliderValueChanged(Slider* slider) override;

    void onButtonClick(juce::Button* button);

    void onDropdownChange(juce::ComboBox* dropdown);

    void processRealtime(const juce::AudioSourceChannelInfo& bufferToFill);
    
    void timerCallback();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeechEnhancer)
};
