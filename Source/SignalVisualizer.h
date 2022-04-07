/*
  ==============================================================================

    SignalVisualizer.h
    Created: 13 Jan 2022 12:29:03pm
    Author:  Bennett

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GraphComponent.h"

//==============================================================================
/*
*/
class SignalVisualizer : public juce::Component,
                         public juce::ScrollBar::Listener,
                         public juce::Slider::Listener
{
public:
    SignalVisualizer(int channels = 1);
    ~SignalVisualizer() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setWindow(std::vector<std::pair<int, int>>& window ) { windowOutline = window; }

    void setTitleText(std::string titleText);
    //void setBufferSize(int size) { signal.setBufferSize(size); }
    //void setSamplesPerBlock(int samples) { signal.setSamplesPerBlock(samples); }
    //void setColours(juce::Colour backgroundColor, juce::Colour waveformColor) { signal.setColours(backgroundColor, waveformColor); }

    //void pushBuffer(const juce::AudioSampleBuffer& buffer);
    //void pushSample(const float* sample);
    GraphComponent graph;
    int graphValuePrecision = 10;
private:
    int channels;
    juce::ScrollBar signalScrollBar;
    juce::Slider zoomSlider;
    int samplePosition = 0;
    bool updatePosition = false;

    std::vector<std::pair<int, int>> windowOutline;

    std::string title = "Signal";

    void drawGraphLines(juce::Graphics& g);
    void drawWindows(juce::Graphics& g);
        
    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

    void sliderValueChanged(Slider* slider) override;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SignalVisualizer)
};
