/*
  ==============================================================================

    FrequencyGraph.cpp
    Created: 7 Feb 2022 11:15:50am
    Author:  Bennett

  ==============================================================================
*/

//#include <JuceHeader.h>
#include "FrequencyGraph.h"

//==============================================================================
template <class T>
FrequencyGraph<T>::FrequencyGraph(int order) : fftSize(1 << order), fftOrder(order), forwardFFT(nullptr), window(nullptr)
{
    startTimerHz(60);
    setOrder(order);

    addAndMakeVisible(skewSlider);
    skewSlider.setRange(0, 1, 0.01);
    skewSlider.setValue(0.2, juce::NotificationType::dontSendNotification);
    skewSlider.onValueChange = [this] {onSliderChange(&skewSlider); };
    skewSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    addAndMakeVisible(skewLabel);
    skewLabel.setText("Skew", juce::NotificationType::dontSendNotification);
    skewLabel.setJustificationType(juce::Justification::centredTop);
}

template <class T>
FrequencyGraph<T>::~FrequencyGraph()
{
    if (forwardFFT != nullptr)
        delete forwardFFT;

    if (window != nullptr)
        delete window;
}

template <class T>
void FrequencyGraph<T>::paint (juce::Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    g.fillAll (juce::Colours::black);   // clear the background

    g.setColour (juce::Colours::grey);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    //drawFrequencyBands(g);
    drawFrame(g);

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    g.drawText (title, getLocalBounds(),
                juce::Justification::centredTop, true);   // draw some placeholder text

   

}

template <class T>
void FrequencyGraph<T>::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
    int width = getWidth();
    int height = getHeight();

    skewLabel.setBounds(width * 0.7, height * 0.03f, width * 0.25f, height * 0.05f);
    skewSlider.setBounds(width * 0.7, height * 0.08f, width * 0.25f, height * 0.05f);
}

template <class T>
void FrequencyGraph<T>::setOrder(int order)
{
    if (forwardFFT != nullptr)
    {
        delete forwardFFT;
    }
    forwardFFT = new juce::dsp::FFT(order);
    fftSize = 1 << order;

    if (window != nullptr)
    {
        delete window;
    }
    window = new juce::dsp::WindowingFunction<T>(fftSize, juce::dsp::WindowingFunction<T>::hann);

    fftOrder = order;
    fifo.clear();
    fifo.resize(fftSize);
    fftData.clear();
    fftData.resize(fftSize);
    scopeData.clear();
    scopeData.resize(scopeSize);
}

template <class T>
void FrequencyGraph<T>::processBuffer(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (bufferToFill.buffer->getNumChannels() > 0)
    {
        const float* buffer = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
        for (int i = 0; i < bufferToFill.numSamples; ++i)
        {
            pushNextSample(buffer[i]);
        }
    }
}

template <class T>
void FrequencyGraph<T>::pushNextSample(float sample)
{
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            clear();
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = sample;
}

template <class T>
void FrequencyGraph<T>::clear()
{
    std::fill(fftData.begin(), fftData.end(), (T)0.f);
    std::copy(fifo.begin(), fifo.end(), fftData.begin());
    //juce::zeromem(&fftData[0], fftData.size());
    //std::memcpy(&fftData[0], &fifo[0], fifo.size());
    nextFFTBlockReady = true;
}

template <class T>
void FrequencyGraph<T>::drawNextFrameOfSpectrum()
{
    window->multiplyWithWindowingTable(&fftData[0], fftSize);
    //forwardFFT->performFrequencyOnlyForwardTransform(&fftData[0]);
    std::vector<juce::dsp::Complex<float>> input;
    for (int i = 0; i < fftSize; ++i)
    {
        input.push_back(juce::dsp::Complex<float>((float)fftData[i], 0));
    }
    std::vector<juce::dsp::Complex<float>> output(fftSize);
    forwardFFT->perform(&input[0], &output[0], false);

    for (int i = 0; i < fftSize; ++i)
    {
        fftData[i] = std::sqrt((output[i].real() * output[i].real()) + (output[i].imag() * output[i].imag()));
    }

    setScopedData();
}

template <class T>
void FrequencyGraph<T>::timerCallback()
{
    if (nextFFTBlockReady)
    {
        drawNextFrameOfSpectrum();
        nextFFTBlockReady = false;
        repaint();
    }
}

template <class T>
void FrequencyGraph<T>::drawFrame(juce::Graphics& g)
{
    int width = getLocalBounds().getWidth() * 0.95f;
    int height = getLocalBounds().getHeight() * 0.95f;
    int widthOffset = getLocalBounds().getWidth() * 0.05f;
    int heightOffset = 0;

    g.setColour(juce::Colours::grey);
    g.drawRect(widthOffset, 0, width, height);

    juce::Path path;
    path.startNewSubPath(widthOffset, height);
    for (int i = 0; i < scopeSize; ++i)
    {
        path.lineTo(widthOffset + (float)juce::jmap(i, 0, scopeSize - 1, 0, width),
            juce::jmap((float)scopeData[i], 0.0f, 1.0f, (float)height, 0.0f));
        
    }


    juce::PathStrokeType myType = juce::PathStrokeType(1.0);
    g.strokePath(path, myType);

    for (int y = 0; y <= 10; ++y)
    {
        int pos = y * height / 10;
        g.drawRect(0, pos, 10, 1);
        int db = juce::jmap<int>(y, 0, 10, 0, -minimumdB);
        g.setFont(8.f);
        g.drawText(std::to_string(db) + " dB", 5, pos, 30, 10, juce::Justification::topLeft);
    }

    g.setFont(10.f);
    int ypos = getHeight() * 0.97f;
    for (int x = 0; x < frequencyLabels.size(); ++x)
    {
        double f = frequencyLabels[x] / (samplingRate / 2.0);
        double normX = 1.0 - std::exp(std::log(1.0 - f) / skew);
        int pos = widthOffset + width * normX;
        //int pos = widthOffset + width * juce::mapFromLog10(frequencyLabels[x], frequencyLabels.front(), frequencyLabels.back());
        //int pos = x * width / 10.f;
        //float skewedProportionX = 1.0f - std::exp(std::log(1.0f - (x / 10.f) * 0.2f));
        //int f = static_cast<int>(juce::jmap(skewedProportionX, 0.f, 1.f, 0.f, (samplingRate / 2.f)));
        g.drawVerticalLine(pos, height - 10, height + 10);
        g.drawText(std::to_string((int)frequencyLabels[x]) + " Hz", pos, ypos, 50, 10, juce::Justification::bottomLeft);
    }
}

template<class T>
void FrequencyGraph<T>::onSliderChange(juce::Slider* slider)
{
    if (slider == &skewSlider)
    {
        skew = slider->getValue();
    }
}


template <class T>
void FrequencyGraph<T>::addFrequencyData(const juce::AudioSampleBuffer& frequencyData)
{
    juce::MessageManagerLock lock;
    std::memcpy(&fftData[0], frequencyData.getReadPointer(0), fftData.size());
    setScopedData();
    repaint();
}

template <class T>
void FrequencyGraph<T>::addFrequencyData(const std::vector<T>& frequencyData)
{
    juce::MessageManagerLock lock;
    //std::memcpy(&fftData[0], &frequencyData[0], fftData.size());
    fftData = frequencyData;
    setScopedData();
    repaint();
}

template <class T>
void FrequencyGraph<T>::setScopedData()
{
    float maxdB = 0.0f;

    for (int i = 0; i < scopeSize; ++i)
    {
        float skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * skew);
        int fftDataIndex = juce::jlimit(0, fftSize, (int)(skewedProportionX * (float)fftSize));
        float level = juce::jmap(juce::jlimit((float)minimumdB, maxdB, (juce::Decibels::gainToDecibels((float)fftData[fftDataIndex])
            - juce::Decibels::gainToDecibels((float)fftSize))),
            (float)minimumdB, maxdB, 0.0f, 1.0f);
        scopeData[i] = level;

        //int index = juce::jmap(i, 0, scopeSize, 0, fftSize);
        //scopeData[i] = fftData[index];
        //float db = juce::jlimit(minimumdB, maxdB, juce::Decibels::gainToDecibels((float)fftData[index]));
        //scopeData[i] = juce::mapFromLog10(db, minimumdB, maxdB);
        //scopeData[i] = juce::jmap(db, minimumdB, maxdB, 0.0f, 1.0f);
    }

}

template <class T>
void FrequencyGraph<T>::setTitleText(std::string titleText)
{
    title = titleText;
    repaint();
}

template <class T>
void FrequencyGraph<T>::drawFrequencyBands(juce::Graphics& g)
{
    if (numFrequencyBands > 1)
    {
        int width = getLocalBounds().getWidth() * 0.95f;
        int height = getLocalBounds().getHeight() * 0.95f;
        int widthOffset = getLocalBounds().getWidth() * 0.05f;

        float bandWidth = width / numFrequencyBands;
        for (int i = 0; i < numFrequencyBands; ++i)
        {
            float start = width * (1.0 - std::exp(std::log(1.0 - (i / (float)numFrequencyBands)) / skew));
            float end = width * (1.0 - std::exp(std::log(1.0 - ((i + 1) / (float)numFrequencyBands)) / skew));
            float pos = widthOffset + (width * start);

            juce::uint8 s = 255 * juce::jmap((float)i / (float)numFrequencyBands, 0.f, 0.5f);
            juce::Colour color(s, s, s, (juce::uint8)(255 / 2));
            g.setColour(color);
            g.fillRect(pos, 0.f, end - start, (float)height);
        }
    }
}
