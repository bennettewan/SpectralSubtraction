/*
  ==============================================================================

    FileManager.h
    Created: 13 Jan 2022 12:16:25pm
    Author:  Bennett

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "ReferenceCountedBuffer.h"
#include "SignalVisualizer.h"
#include "FrequencyGraph.h"

class MainComponent;
class SpectrogramComponent;

enum TransportState
{
    Stopped,
    Starting,
    Playing,
    Stopping
};



//==============================================================================
/*
*/
class FileManager : public juce::Component,
                    private juce::Thread
{
public:
    FileManager();
    ~FileManager() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void initialize(float sampleRate, MainComponent* parentComponent);
    void reset();

    void processBuffer(const juce::AudioSourceChannelInfo& bufferToFill);

    //float getGain() { return gain; }

    TransportState state;

    juce::AudioSampleBuffer* getBuffer();
    bool isFileLoaded() { return buffers.size() > 0; }

    int getBufferPosition() const { return bufferPosition; }
    void setBufferPosition(int position) { 
       if (currentBuffer.get() != nullptr)
           currentBuffer->position = position; 
       bufferPosition = position; 
    }

private:

    void onOpenFileButtonClicked();
    
    void onButtonChange(juce::Button* button);

    void onFileMenuChanged();

    void checkForBuffersToFree();
    void checkForPathToOpen();
    void run() override
    {
        while (!threadShouldExit())
        {
            checkForPathToOpen();
            checkForBuffersToFree();
            wait(500);
        }
    }



    MainComponent* mainComponent;

    float gain = 1.f;
    float rate = 48000.f;

    //Buttons
    juce::TextButton openFileButton;
    juce::TextButton playFileButton;
    juce::TextButton stopFileButton;
    juce::TextButton saveButton;
    juce::TextButton resetButton;

    //buffer
    juce::ReferenceCountedArray<ReferenceCountedBuffer> buffers;
    ReferenceCountedBuffer::Ptr currentBuffer;
    juce::CriticalSection pathMutex;
    juce::SpinLock mutex;
    juce::String chosenPath;
    int bufferPosition = 0;


    // Audio reader
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::FileChooser> fileChooser;
    //int numChannels;


    // Dropdown
    juce::ComboBox fileDropdown;
    juce::Label fileLabel;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileManager)
};
