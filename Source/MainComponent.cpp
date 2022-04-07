#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    
    setSize (1440, 810);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    addAndMakeVisible(inputManager);

    addAndMakeVisible(speechEnhancer);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    rate = sampleRate;
    inputManager.initialize(sampleRate, this);
    speechEnhancer.initialize(sampleRate, samplesPerBlockExpected, this);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    inputManager.processBuffer(bufferToFill);
    speechEnhancer.processBuffer(bufferToFill);


    if (inputManager.muteOutput)
    {
        bufferToFill.clearActiveBufferRegion();
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkcyan);
}

void MainComponent::resized()
{
    int width = getWidth();
    int height = getHeight();

    inputManager.setBounds(0, 0, (int)(width / 3.f), (int)(height));

    speechEnhancer.setBounds((int)(width / 3.f), 0, (int)(2 * width / 3.f), (int)(height));

}



void saveWavFile(juce::AudioSampleBuffer* buffer, const std::string& name, double sampleRate)
{
    int size = buffer->getNumSamples();
    int numChannels = buffer->getNumChannels();

    File file(File::getSpecialLocation(File::SpecialLocationType::currentExecutableFile).getParentDirectory().getFullPathName() + "\\" +
        name + ".wav");
    WavAudioFormat format;
    FileOutputStream* outStream = new FileOutputStream(file);
    if (outStream->openedOk())
    {
        outStream->setPosition(0);
    }
    std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(outStream, sampleRate, numChannels, 16, {}, 0));
    if (writer != nullptr)
    {
        writer->writeFromAudioSampleBuffer(*buffer, 0, size);
    }
}