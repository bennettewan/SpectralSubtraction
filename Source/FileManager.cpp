/*
  ==============================================================================

    FileManager.cpp
    Created: 13 Jan 2022 12:16:25pm
    Author:  Bennett

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FileManager.h"
#include "MainComponent.h"

using namespace juce;

//==============================================================================
FileManager::FileManager() : Thread("MainThread"), state(Stopped)
{
    formatManager.registerBasicFormats();

    // Open button  
    addAndMakeVisible(&openFileButton);
    openFileButton.onClick = [this] { onOpenFileButtonClicked(); };
    openFileButton.setButtonText("Add File...");

    // Play button
    addAndMakeVisible(&playFileButton);
    playFileButton.onClick = [this] { onButtonChange(&playFileButton); };
    playFileButton.setButtonText("Play");
    playFileButton.setColour(TextButton::buttonColourId, Colours::green);
    playFileButton.setEnabled(false);

    // Stop button
    addAndMakeVisible(&stopFileButton);
    stopFileButton.onClick = [this] { onButtonChange(&stopFileButton); };
    stopFileButton.setButtonText("Stop");
    stopFileButton.setColour(TextButton::buttonColourId, Colours::red);
    stopFileButton.setEnabled(false);

    // Save button
    addAndMakeVisible(saveButton);
    saveButton.onClick = [this] { onButtonChange(&saveButton); };
    saveButton.setButtonText("Save Output");
    saveButton.setEnabled(false);

    // Save button
    addAndMakeVisible(resetButton);
    resetButton.onClick = [this] { onButtonChange(&resetButton); };
    resetButton.setButtonText("Reset");
    resetButton.setEnabled(false);

    // Dropdown
    addAndMakeVisible(fileDropdown);
    fileDropdown.onChange = [this] { onFileMenuChanged(); };
    addAndMakeVisible(fileLabel);
    fileLabel.setText("Select File", juce::dontSendNotification);
    fileLabel.setFont(juce::Font(14.f, juce::Font::bold));
    fileLabel.setColour(Label::ColourIds::textColourId, juce::Colours::white);
    fileLabel.setJustificationType(Justification::centred);

    startThread();
}

FileManager::~FileManager()
{
    stopThread(1000);
}

void FileManager::initialize(float sampleRate, MainComponent* parentComponent)
{
    rate = sampleRate;
    mainComponent = parentComponent;
}

void FileManager::reset()
{
    currentBuffer->position = 0;
}

void FileManager::paint(juce::Graphics& g)
{
    //g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));   // clear the background
    //g.fillAll(juce::Colours::darkslateblue);   // clear the background

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1);   // draw an outline around the component

    //g.setColour(juce::Colours::white);
    //g.setFont(24.0f);
    //g.drawText("Input Sample", getLocalBounds(),
    //    juce::Justification::topLeft, true);   // draw some placeholder text

}

void FileManager::resized()
{
    int width = getWidth();
    int height = getHeight();

    openFileButton.setBounds(0, height * 0.05f, (width * 0.3), height * 0.35f);
    fileLabel.setBounds(     0, height * 0.4f,  width * 0.3f,  height * 0.45f / 2.f);
    fileDropdown.setBounds(  0, height * 0.6f,   width * 0.3f,  height * 0.45f / 2.f);

    playFileButton.setBounds(width * 0.33f, height * 0.05f, (width * 0.3f), height * 0.4f);
    stopFileButton.setBounds(width * 0.66f, height * 0.05f, (width * 0.3f), height * 0.4f);
    resetButton.setBounds(width * 0.33f, height * 0.5f,  (width * 0.3f), height * 0.4f);
    saveButton.setBounds(width * 0.66f, height * 0.5f,  (width * 0.3f), height * 0.4f);

}

juce::AudioSampleBuffer* FileManager::getBuffer()
{
    const juce::SpinLock::ScopedTryLockType lock(mutex);

    if (lock.isLocked())
        return currentBuffer.get()->getAudioSampleBuffer();

    return nullptr;
}

void FileManager::processBuffer(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (state == Stopped)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    auto retainedCurrentBuffer = [&]() -> ReferenceCountedBuffer::Ptr
    {
        const juce::SpinLock::ScopedTryLockType lock(mutex);

        if (lock.isLocked())
            return currentBuffer;

        return nullptr;
    }();

    if (retainedCurrentBuffer == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    //Get buffer
    AudioSampleBuffer* currentAudioSampleBuffer = retainedCurrentBuffer->getAudioSampleBuffer();
    bufferPosition = retainedCurrentBuffer->position;
    //inputGraph.graph.setPosition(position);

    int numInputChannels = currentAudioSampleBuffer->getNumChannels();
    int numOutputChannels = bufferToFill.buffer->getNumChannels();

    int outputSamplesRemaining = bufferToFill.numSamples;
    int outputSamplesOffset = 0;

    // While we still have samples to read
    while (outputSamplesRemaining > 0)
    {
        int bufferSamplesRemaining = currentAudioSampleBuffer->getNumSamples() - bufferPosition;
        int samplesThisTime = juce::jmin(outputSamplesRemaining, bufferSamplesRemaining);
        if (samplesThisTime == 0)
            break;
        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            float* outBuffer = bufferToFill.buffer->getWritePointer(channel);
            for (int i = 0; i < samplesThisTime; ++i)
            {
                outBuffer[i] = gain * currentAudioSampleBuffer->getSample(channel % numInputChannels, bufferPosition + i);

            }
        }

        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
        bufferPosition += samplesThisTime;

        // Reached end up buffer
        if (bufferPosition == currentAudioSampleBuffer->getNumSamples())
        {
            bufferPosition = 0;
        }
    }
    retainedCurrentBuffer->position = bufferPosition;




}




void FileManager::onOpenFileButtonClicked()
{

    fileChooser = std::make_unique<juce::FileChooser>("Select a Wave file shorter than 10 seconds to play...",
        juce::File::getSpecialLocation(File::SpecialLocationType::currentExecutableFile),
        "*.wav");
    auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();

            if (file == juce::File{})
                return;

            auto path = file.getFullPathName();

            int id = buffers.size();
            fileDropdown.addItem(file.getFileName(), id + 1);
            
            {
                const juce::ScopedLock lock(pathMutex);
                chosenPath.swapWith(path);
            }

            notify();
        });
}



void FileManager::onFileMenuChanged()
{
    int id = fileDropdown.getSelectedItemIndex();
    if (id >= 0 && id < buffers.size())
    {
        currentBuffer = buffers[id];

        mainComponent->inputManager.setInputGraph(currentBuffer.get()->getAudioSampleBuffer());

    }
}


void FileManager::checkForBuffersToFree()
{
    //for (int i = buffers.size(); --i >= 0;)
    //{
    //  ReferenceCountedBuffer::Ptr buffer(buffers.getUnchecked(i));
    //  if (buffer->getReferenceCount() == 2)
    //    buffers.remove(i);
    //}
}

void FileManager::checkForPathToOpen()
{

    juce::String pathToOpen;

    {
        const juce::ScopedLock lock(pathMutex);
        pathToOpen.swapWith(chosenPath);
    }

    if (pathToOpen.isNotEmpty())
    {
        juce::File file(pathToOpen);
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

        if (reader.get() != nullptr)
        {
            auto duration = (float)reader->lengthInSamples / reader->sampleRate;

            if (duration < 10)
            {
                ReferenceCountedBuffer::Ptr newBuffer = new ReferenceCountedBuffer(file.getFileName(),
                    (int)reader->numChannels,
                    (int)reader->lengthInSamples);

                reader->read(newBuffer->getAudioSampleBuffer(), 0, (int)reader->lengthInSamples, 0, true, true);

                {
                    const juce::SpinLock::ScopedLockType lock(mutex);
                    currentBuffer = newBuffer;

                }

                buffers.add(newBuffer);

                MessageManagerLock msgLock;
                fileDropdown.setSelectedItemIndex(buffers.size() - 1, juce::NotificationType::sendNotificationAsync);

                playFileButton.setEnabled(true);
                resetButton.setEnabled(true);

                mainComponent->speechEnhancer.onFileLoaded();
            }
            else
            {
                // handle the error that the file is 2 seconds or longer..
            }
        }
    }
}



void FileManager::onButtonChange(juce::Button* button)
{
    if (button == &playFileButton)
    {
        state = Playing;
        playFileButton.setEnabled(false);
        stopFileButton.setEnabled(true);

        juce::AudioSampleBuffer* buffer = currentBuffer.get()->getAudioSampleBuffer();
        
    }
    else if (button == &stopFileButton)
    {
        state = Stopped;
        playFileButton.setEnabled(true);
        stopFileButton.setEnabled(false);
    }
    else if (button == &saveButton)
    {
        AudioSampleBuffer* outputBuffer = mainComponent->speechEnhancer.getOutputBuffer();
        if (outputBuffer != nullptr)
        {
            saveWavFile(outputBuffer, "output", rate);
        }
    }
    else if (button == &resetButton)
    {
        bufferPosition = 0;
        currentBuffer.get()->position = 0;
    }

}
