/*
  ==============================================================================

    FrequencyGraph.h
    Created: 7 Feb 2022 11:15:50am
    Author:  Bennett

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
template <class T>
class FrequencyGraph  : public juce::Component, private juce::Timer
{
public:
    FrequencyGraph(int order);
    ~FrequencyGraph() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setOrder(int order);
    void processBuffer(const juce::AudioSourceChannelInfo& bufferToFill);
    void pushNextSample(float sample);
    void addFrequencyData(const juce::AudioSampleBuffer& frequencyData);
    void addFrequencyData(const std::vector<T>& frequencyData);

    void clear();
    void drawNextFrameOfSpectrum();

    void setTitleText(std::string titleText);

    void setSamplingRate(float rate) { samplingRate = rate; }
    void setNumFrequencyBands(int numBands) { numFrequencyBands = numBands; }

private:
    int fftOrder;
    int fftSize;
    int scopeSize = 512;
    float minimumdB = -100;
    float minFrequency = 20.f;
    float maxFrequency = 20000.f;
    juce::dsp::FFT* forwardFFT;
    juce::dsp::WindowingFunction<T>* window;
    std::vector<T> fifo;
    std::vector<T> fftData;
    std::vector<T> scopeData;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    float samplingRate = 48000.f;
    int numFrequencyBands = 1;
    double skew = 0.2;

    juce::Label skewLabel;
    juce::Slider skewSlider;


    std::vector<float> frequencyLabels = { 100, 500, 1000,
                                       2000, 3000, 5000, 10000, 20000 };

    std::string title = "Frequency Graph";

    void drawFrame(juce::Graphics& g);
    void drawFrequencyBands(juce::Graphics& g);
    void setScopedData();

    void timerCallback() override;

    void onSliderChange(juce::Slider* slider);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyGraph)
};

