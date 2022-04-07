/*
  ==============================================================================

    SpectrumGraph.cpp
    Created: 28 Mar 2022 3:11:21pm
    Author:  Bennett

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SpectrumGraph.h"

//==============================================================================
SpectrumGraph::SpectrumGraph() 
{
    scopeData.resize(scopeSize);
}

SpectrumGraph::~SpectrumGraph()
{
}

void SpectrumGraph::paint (juce::Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    g.fillAll(juce::Colours::black);   // clear the background

    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);   // draw an outline around the component

    //drawFrequencyBands(g);
    drawFrame(g);
    drawSNRGraph(g);
    drawSmoothingGraph(g);
    drawAxis(g);

    //g.setColour(juce::Colours::white);
    //g.setFont(14.0f);
    //g.drawText(title, getLocalBounds(),
    //    juce::Justification::centredTop, true);
}

void SpectrumGraph::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

void SpectrumGraph::addFrequencyData(const std::vector<double>& frequencyData)
{
    juce::MessageManagerLock lock;
    fftData = frequencyData;
    setScopedData();
    repaint();
}


void SpectrumGraph::clear()
{
    std::fill(fftData.begin(), fftData.end(), 0.f);
}

void SpectrumGraph::setTitleText(std::string titleText)
{
    title = titleText;
    repaint();
}


void SpectrumGraph::drawFrame(juce::Graphics& g)
{
    int width = getLocalBounds().getWidth() * 0.95f;
    int height = (int)((getLocalBounds().getHeight() / 3.f) * 0.95f);
    int start = int(width * 0.05f);
    g.setColour(juce::Colours::grey);
    g.drawRect(start, 0, width, height, 1);

    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText("Noise Estimation", 0, 0, width, height,
        juce::Justification::centredTop, true);

    if (fftData.size() == 0)
        return;

    g.setColour(juce::Colours::white);
    juce::Path path;
    path.startNewSubPath(start, height);
    for (int i = 0; i < scopeSize; ++i)
    {
        float skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * skew);
        int index = juce::jlimit(0, (int)fftData.size(), (int)(skewedProportionX * (float)fftData.size()));
        //int index = juce::jmap(i, 0, (int)fftData.size(), 0, scopeSize - 1);
        path.lineTo(start + (float)juce::jmap(i, 0, scopeSize - 1, 0, width), 
                    juce::jmap(juce::Decibels::gainToDecibels((float)fftData[index]), (float)minimumDB, 0.f, (float)height, 0.0f));
    }


    juce::PathStrokeType myType = juce::PathStrokeType(1.0);
    g.strokePath(path, myType);

    //for (int y = 0; y <= 10; y += 2)
    //{
    //    int pos = y * height / 10;
    //    g.drawRect(0, pos, 10, 1);
    //    int db = juce::jmap<int>(y, 0, 10, 0, -200);
    //    g.setFont(10.f);
    //    g.drawText(std::to_string(db) + " dB", 10, pos, 30, 10, juce::Justification::topLeft);
    //}
}

void SpectrumGraph::setScopedData()
{
    double maxdB = 0.0;
    int fftSize = fftData.size();

    for (int i = 0; i < scopeSize; ++i)
    {
        float skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * 0.2f);
        int fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionX * (float)fftSize * 0.5f));
        double level = juce::jmap(juce::jlimit((double)minimumDB, maxdB, juce::Decibels::gainToDecibels(fftData[fftDataIndex])
            - juce::Decibels::gainToDecibels((float)fftSize)),
            (double)minimumDB, maxdB, 0.0, 1.0);

        scopeData[i] = level;
    }
}

void SpectrumGraph::drawSNRGraph(juce::Graphics& g)
{

    double minSNR = -10;
    double maxSNR = 20;

    int width = getLocalBounds().getWidth() * 0.95f;
    int height = (int)((getLocalBounds().getHeight() / 3.f) * 0.95f);
    int start = int(width * 0.05f);

    g.setColour(juce::Colours::grey);
    g.drawRect(start, height, width, height, 1);

    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText("A-Posteriori SNR", 0, height, width, height,
        juce::Justification::centredTop, true);

    if (snrData.size() == 0)
        return;

    juce::Path path;
    path.startNewSubPath(start, 2 * height);
    g.setColour(juce::Colours::indianred);

    for (int i = 0; i < scopeSize; ++i)
    {
        float skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * skew);
        int index = juce::jlimit(0, (int)snrData.size(), (int)(skewedProportionX * (float)snrData.size()));

        //int index = juce::jmap(i, 0, (int)snrData.size(), 0, scopeSize - 1);
        double level = juce::jmap(juce::jlimit(minSNR, maxSNR, snrData[index]), minSNR, maxSNR, (double)height * 2, (double)height);

        path.lineTo(start + (i / (float)scopeSize) * width, level);
    }
    juce::PathStrokeType myType = juce::PathStrokeType(1.0);
    g.strokePath(path, myType);
}

void SpectrumGraph::drawSmoothingGraph(juce::Graphics& g)
{

    double minSmoothing = 0;
    double maxSmoothing = 1;

    int width = getLocalBounds().getWidth() * 0.95f;
    int height = (int)((getLocalBounds().getHeight() / 3.f) * 0.95f);
    int start = int(width * 0.05f);

    g.setColour(juce::Colours::grey);
    g.drawRect(start, height * 2, width, height, 1);

    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText("Smoothing Parameter", 0, height * 2, width, height,
        juce::Justification::centredTop, true);

    if (smoothingData.size() == 0)
        return;

    juce::Path path;
    path.startNewSubPath(start, 3 * height);
    g.setColour(juce::Colours::blue);

    for (int i = 0; i < scopeSize; ++i)
    {
        float skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * skew);
        int index = juce::jlimit(0, (int)smoothingData.size(), (int)(skewedProportionX * (float)smoothingData.size()));

        //int index = juce::jmap(i, 0, (int)smoothingData.size(), 0, scopeSize - 1);
        double level = juce::jmap(juce::jlimit(minSmoothing, maxSmoothing, smoothingData[index]), minSmoothing, maxSmoothing, (double)height * 3.0, (double)(height * 2.0) + (height / 2.f));

        path.lineTo(start + (i / (float)scopeSize) * width, level);
    }
    juce::PathStrokeType myType = juce::PathStrokeType(1.0);
    g.strokePath(path, myType);
}

void SpectrumGraph::drawFrequencyBands(juce::Graphics& g)
{
    if (numFrequencyBands > 1)
    {
        int width = getWidth();
        int height = getHeight();
        float bandWidth = width / numFrequencyBands;
        for (int i = 0; i < numFrequencyBands; ++i)
        {
            juce::uint8 s = 255 * juce::jmap((float)i / (float)numFrequencyBands, 0.f, 0.5f);
            juce::Colour color(s, s, s, (juce::uint8)(255 / 2));
            g.setColour(color);
            g.fillRect(i * bandWidth, 0.f, bandWidth, (float)height * 0.95f);
        }
    }
}

void SpectrumGraph::drawAxis(juce::Graphics& g)
{
    int width = getLocalBounds().getWidth() * 0.95f;
    int height = (int)(getLocalBounds().getHeight() * 0.95f);
    int start = int(width * 0.05f);

    float maxFreq = samplingRate / 2.f;
    int ypos = getHeight() * 0.97f;

    g.setColour(juce::Colours::grey);
    g.setFont(10.f);
    for (int i = 0; i < frequencyLabels.size(); ++i)
    {
        //float normX = juce::jmap(frequencyLabels[i], 0.f, maxFreq, 0.f, 1.f);
        //float pos = start + (normX * width);

        double f = frequencyLabels[i] / (samplingRate / 2.0);
        double normX = 1.0 - std::exp(std::log(1.0 - f) / skew);
        int pos = start + width * normX;

        g.drawVerticalLine(pos, height - 10, height + 10);
        g.drawText(std::to_string((int)frequencyLabels[i]) + " Hz", pos, ypos, 50, 10, juce::Justification::bottomLeft);
    }
}

