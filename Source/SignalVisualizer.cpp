/*
  ==============================================================================

    SignalVisualizer.cpp
    Created: 13 Jan 2022 12:29:04pm
    Author:  Bennett

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SignalVisualizer.h"

//==============================================================================
SignalVisualizer::SignalVisualizer(int numChannels) : graph(), channels(numChannels), signalScrollBar(false)
{
    //addAndMakeVisible(signalScrollBar);
    //signalScrollBar.setVisible(true);
    //signalScrollBar.setRangeLimits(0, 1000, sendNotificationSync);
    //signalScrollBar.addListener(this);

    addAndMakeVisible(graph);

    //addAndMakeVisible(zoomSlider);
    //zoomSlider.addListener(this);
    //zoomSlider.setRange(0.01, 2.f, 0.01f);
    //zoomSlider.setValue(1.0, juce::dontSendNotification);
    //zoomSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
    
}

SignalVisualizer::~SignalVisualizer()
{
}

void SignalVisualizer::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);   // draw an outline around the component

    drawGraphLines(g);

    drawWindows(g);

    g.setColour(juce::Colours::black);
    g.setFont(14.0f);
    g.drawText(title, getLocalBounds(),
        juce::Justification::centredTop, true);   // draw some placeholder text
}

void SignalVisualizer::resized()
{
    int width = getWidth();
    int height = getHeight();

    graph.setBounds(0, 0, width, height);
    //graph.setBounds(width * 0.1f, 0, width * 0.9f, height * 0.9f);
    //signalScrollBar.setBounds(width * 0.1f, height * 0.9f, width * 0.9f, height * 0.1f);
    //zoomSlider.setBounds(0, 0, width * 0.05f, height);

}

//void SignalVisualizer::pushBuffer(const juce::AudioSampleBuffer& buffer)
//{
//    signal.pushBuffer(buffer);
//}
//
//void SignalVisualizer::pushSample(const float* sample)
//{
//    signal.pushSample(sample, channels);
//}

void SignalVisualizer::scrollBarMoved(ScrollBar* scrollBarThatHasMoved,
    double newRangeStart)
{
    double newRangeL = scrollBarThatHasMoved->getCurrentRange().getStart();
    graph.leftEndPoint = (float)newRangeL / 1000 * (float)graph.sampleCount;
    graph.rightEndPoint = graph.leftEndPoint + (float)graph.numSamples;
    if (graph.rightEndPoint > graph.sampleCount)
    {
        graph.rightEndPoint = (float)graph.sampleCount;
        graph.leftEndPoint = graph.rightEndPoint - (float)graph.numSamples;
    }
    if (newRangeL == 0) {
        graph.hardLeft = true;
    }
    repaint();
}


void SignalVisualizer::sliderValueChanged(Slider* slider)
{
    if (slider == &zoomSlider)
    {
        graph.magnify = slider->getValue();
        graph.repaint();
    }
}



void SignalVisualizer::drawGraphLines(juce::Graphics& g)
{
    for (int y = 0; y <= graphValuePrecision; ++y)
    {
        int pos = y * graph.getHeight() / graphValuePrecision;
        g.drawRect((int)(getWidth() * 0.1f) - 10, pos, 10, 1);
        float val = juce::jmap<float>(y, 0, graphValuePrecision, graph.maxValue, -graph.maxValue);
        g.setFont(10.f);
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << val;
        g.drawText(stream.str(), (int)(getWidth() * 0.05f), pos, 30, 10, juce::Justification::topLeft);
    }
}

void SignalVisualizer::drawWindows(juce::Graphics& g)
{
    for (int i = 0; i < windowOutline.size(); ++i)
    {
        int x = (windowOutline[i].second + windowOutline[i].first) / 2;
        int width = windowOutline[i].second - windowOutline[i].first;
        g.fillRoundedRectangle(x, graph.getHeight() / 2, width, graph.getHeight() * 0.9, 5);
    }
}

void SignalVisualizer::setTitleText(std::string titleText)
{
    titleText = title;
    repaint();
}