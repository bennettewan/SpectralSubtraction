/*
  ==============================================================================

    GraphComponent.h
    Created: 18 Nov 2021 11:51:34am
    Author:  Matt Klassen

  ==============================================================================
*/

#pragma once


#include <string>
#include <JuceHeader.h>

using namespace juce;

class GraphComponent    : public juce::Component,
                          private juce::Timer
{
public:
    GraphComponent()
    {
        leftEP.setValue(0.0);
        rightEP.setValue(1200);

        startTimerHz(30);
    }
    
    void setDataForGraph(AudioBuffer<float>& _floatBuffer, bool _audioLoaded, int _numSamples, float _magnify, float _leftEndPoint, float _rightEndPoint, unsigned _sampleCount, unsigned sampleRate);

    AudioBuffer<float> floatBuffer;
    bool audioLoaded = false;
    bool updateGraph = false;

    bool hardLeft = true;
    int numSamples = 1000;

    float magnify = 1;
    float leftEndPoint = 0;
    float rightEndPoint = 1;
    float w, h;
    float maxValue = 1.f;
    unsigned sampleCount;
    unsigned sampleRate;
    float addoffset = 0;
    float magfactor = 1;
    int startIndex = 0;
    int endIndex = 0;


    Value leftEP;
    Value rightEP;
    Value dragged;
    juce::Point<int> doubleClick;
    juce::Path freeCurve;
    bool drawCurve = false;
    
    juce::Point<float> signalToScreenCoords (juce::Point<float> P);
    juce::Point<float> screenToSignalCoords (juce::Point<float> Q);

    void setPosition(int sample) {
        samplePosition = sample;
        positionUpdated = true;
    }
    
    void paint (juce::Graphics& g) override;

    void resized() override;

private:
    int samplePosition = 0;
    bool positionUpdated = false;

    void drawGraphBox(juce::Graphics& g, float w, float h);
    
    void mouseDown (const MouseEvent& event) override;
    
    void mouseDrag (const MouseEvent& event) override;
    
    void mouseDoubleClick (const MouseEvent& event) override;
    
    void graphSignal(juce::Graphics& g);

    void scaleInterval();

    int getMult(int numSamples);
    
    void drawDot(juce::Point<float> (P), juce::Graphics& g);
    
    void mouseWheelMove (const MouseEvent& event, const MouseWheelDetails & wheel) override;
    
    void mouseMagnify (const MouseEvent & event, float scaleFactor) override;

    void timerCallback() override;


    float currentSamplePosition() {
        return (int)(samplePosition - leftEndPoint) / (rightEndPoint - leftEndPoint);
    }
        
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphComponent)
};



class GraphLine : public Component
{
public:
    GraphLine(juce::Colour color, float thickness = 1.f) : lineColor(color), lineThickness(thickness), start(0, 0), end(0, 0) {}
    ~GraphLine() override {}

    void paint(juce::Graphics& g) override {
        g.setColour(lineColor);
        g.drawLine(start.x, start.y, end.x, end.y, lineThickness);
    }
    void resized() override {}

    juce::Point<int> start, end;
private:
    juce::Colour lineColor;
    float lineThickness;
};