/*
  ==============================================================================

    SpectrumGraph.h
    Created: 28 Mar 2022 3:11:21pm
    Author:  Bennett

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class SpectrumGraph  : public juce::Component
{
public:
    SpectrumGraph();
    ~SpectrumGraph() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void addFrequencyData(const std::vector<double>& frequencyData);

    void clear();

    void setTitleText(std::string titleText);
    void setSamplingRate(float rate) { samplingRate = rate; }

    void setSNR(const std::vector<double>& snr) { snrData = snr; }
    void setSmoothingData(const std::vector<double>& smoothing) { smoothingData = smoothing; }
    void setNumFrequencyBands(int numBands) { numFrequencyBands = numBands; }
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumGraph)

    int scopeSize = 512;
    int minimumDB = -100;
    std::vector<double> fftData;
    std::vector<double> scopeData;
    int fifoIndex = 0;
    std::vector<double> snrData;
    std::vector<double> smoothingData;
    int numFrequencyBands = 1;
    float samplingRate = 48000;
    float skew = 0.2f;

    std::vector<float> frequencyLabels = { 100, 500, 1000, 2000, 
                                           3000, 5000, 10000, 20000 };

    std::string title = "Frequency Graph";

    void drawFrame(juce::Graphics& g);
    void setScopedData();
    void drawSNRGraph(juce::Graphics& g);
    void drawSmoothingGraph(juce::Graphics& g);
    void drawFrequencyBands(juce::Graphics& g);
    void drawAxis(juce::Graphics& g);
};
