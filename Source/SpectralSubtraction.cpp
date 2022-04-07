/*
  ==============================================================================

    SpectralSubtraction.cpp
    Created: 7 Feb 2022 2:37:57pm
    Author:  Bennett

    Reference: https://reader.elsevier.com/reader/sd/pii/S1877705813016172?token=57D024BF8D9A282F839BB1B1A59904FEF39002D640F79D7F1D549093BDFD9F880078AFC9BC8826B86638E17071BD5239&originRegion=us-east-1&originCreation=20220330183504
  ==============================================================================
*/

#include "SpectralSubtraction.h"
#include <cmath>
#include <complex>



SpectralSubtraction::SpectralSubtraction(int fft_order) : bandWeights(8, 1.f)
{
    setFFTOrder(fft_order);
    setWindowOverlap(0.5f);

    calculateFrequencyBands();
}

SpectralSubtraction::~SpectralSubtraction()
{
    if (fft) 
        delete fft;
}




// Process a buffer in place
void SpectralSubtraction::processBuffer(float* buffer, int size)
{
    // Enframe buffer into overlapping frames
    Matrix frames = createSignalFrames(buffer, size, windowSize, hopSize);

    // Transform into frequency domain
    SpectrumMatrix freqData = createFrequencyData(frames);

    if (subtractionEnabled)
    {
        // Subtract noise and inverse transform into time domain
        Matrix cleanFrames = processSubtraction(freqData);

        // Overlap and add samples to reconstruct signal
        std::vector<float> cleanSignal = createSamplesFromFrames(cleanFrames, windowSize, hopSize);

        // Copy into buffer
        for (int i = 0; i < size; ++i)
        {
            buffer[i] = cleanSignal[i];
        }
        //std::memcpy(buffer, &cleanSignal[0], size);
    }
    

}


// Process spectral subtraction
Matrix SpectralSubtraction::processSubtraction(const std::vector<Spectrum>& frequencyData)
{
    int numFrames = frequencyData.size();
    Matrix output(numFrames);

    // Return if no noise estimate is available
    const Frame* noiseEst = getNoiseEstimation();
    if (!noiseEst)
        return Matrix();

    // Loop through each frame
    for (int i = 0; i < numFrames; ++i)
    {
        const Spectrum& dirtyFrame = frequencyData[i];
        Spectrum cleanFrame(windowSize);

        // Process subtraction separetely in each frequency band
        for (int n = 0; n < numFrequencyBands; ++n)
        {
            // This check prevents issues when band slider is changed while processing
            if (n >= frequencyBandRanges.size()) 
                break;

            // Calculate oversubtraction determined from frame SNR
            const std::pair<int, int>& range = frequencyBandRanges[n];
            double snr = segmentalSNR(dirtyFrame, *noiseEst, range);
            double overSubtraction = calculateOverSubtraction(snr);
            const double& bandWeight = bandWeights[n];

            // Loop through each frequency index
            for (int w = range.first; w < range.second; ++w)
            {
                // Get magnitude and phase of input signal
                double currMag = dirtyFrame[w].magnitude();
                double phase = dirtyFrame[w].phase();

                // Magnitude or power spectrum
                double Y_w = subtractionDomain == 1 ? currMag : (currMag * currMag);

                // Get noise estimate 
                double D_w = (*noiseEst)[w];

                // How much magnitude should be subtracted
                double magSubtracted = (overSubtraction * bandWeight * D_w);
                noiseSubtracted[w] = magSubtracted;

                // Apply subtraction. Flooring smoothes out the valleys to reduce distortion
                double S_w = std::max(Y_w - magSubtracted, subtractionFloor * D_w);
                
                // Handle magnitude/power spectrum
                double mag = subtractionDomain == 1 ? S_w : std::sqrt(S_w);

                // Convert to complex number and add to clean signal
                double real = mag * std::cos(phase);
                double imag = mag * std::sin(phase);
                cleanFrame[w] = Complex(real, imag);

            }
        }
        
        // Transform output back to time domain
        output[i] = calculateIFFT(cleanFrame);
    }

    return output;
}





// Change the fft order for processing
void SpectralSubtraction::setFFTOrder(int fft_order) 
{
    order = fft_order;
    NFFT = 1 << order;
    windowSize = NFFT;
    hopSize = windowSize* (1.f - windowOverlap);

    if (fft) delete fft;
    fft = new juce::dsp::FFT(order);

    // Resize noise estimate buffers
    a_SNR.resize(windowSize);
    noiseSubtracted.resize(windowSize);
    estimationSmoothing.resize(windowSize);

    // Create window functions
    createWindow();

}



// Set the signal to process
void SpectralSubtraction::setSignal(juce::AudioSampleBuffer* buffer) 
{ 
    signal = buffer; 
}


// Create frequency data from signal data
SpectrumMatrix SpectralSubtraction::createFrequencyData(const Matrix& frames)
{
    int numFrames = frames.size();
    SpectrumMatrix frequencyData(numFrames);
    for (int i = 0; i < numFrames; ++i)
    {
        // Transform to frequency domain
        frequencyData[i] = frequencySpectrum(frames[i]);

        // Update noise estimation
        if (adaptiveEstimationEnabled && noiseEstimationEnabled)
        {
            Frame spectrum = subtractionDomain == 1 ? complexToMagnitudeSpectrum(frequencyData[i]) : complexToPowerSpectrum(frequencyData[i]);
            updateNoiseEstimation(spectrum);
        }

    }
    return frequencyData;
}

// Generate a noise spectrum based on a buffer
Frame SpectralSubtraction::bufferToNoiseProfile(const std::vector<float>& buffer)
{
    Matrix noiseFrames = createSignalFrames(&buffer[0], buffer.size(), windowSize, hopSize);
    return noiseAverageSpectrum(noiseFrames);
}

// Compute the noise spectrum based on a files signal
Frame SpectralSubtraction::computeFileNoiseProfile()
{
    if (signal == nullptr)
        return Frame();

    // Assume that the first second of the signal contains only noise
    int numSamples = getNoiseProfileSize();

    // Copy samples into buffer
    std::vector<float> noiseBuffer(numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        noiseBuffer[i] = signal->getSample(0, i);
    }

    // Compute average noise estimation
    Frame estimate = bufferToNoiseProfile(noiseBuffer);

    averageNoise = estimate;
    return estimate;
}


// Averages the amplitude spectrum over the given frames
Frame SpectralSubtraction::noiseAverageSpectrum(const Matrix& frames)
{
    Frame averageNoiseSpectrum;
    int numFrames = frames.size();
    Frame avgAmp(windowSize, 0);

    // Loop through all frames
    for (int i = 0; i < numFrames; ++i) 
    {
        // Transform to frequency data
        Spectrum temp = frequencySpectrum(frames[i]);

        // Sum frequency data
        for (int k = 0; k < windowSize; ++k)
            avgAmp[k] += temp[k].magnitude();

    }

    // Average
    for (int i = 0; i < windowSize; ++i) 
    {
        averageNoiseSpectrum.push_back(avgAmp[i] / numFrames);
    }

    return averageNoiseSpectrum;
}


// Window and transform into frequency domain
Spectrum SpectralSubtraction::frequencySpectrum(const Frame& frame)
{
    int size = frame.size();
    Frame fft_input(size, 0);
    const Frame& currWindow = windows[windowType];
    for (int j = 0; j < size; ++j) 
    {
        fft_input[j] = currWindow[j] * frame[j];
    }
    Spectrum fft_out = calculateFFT(fft_input);
    return fft_out;
}


// Run FFT on a single frame
Spectrum SpectralSubtraction::calculateFFT(Frame& fft_input)
{
    int size = fft_input.size();
    Spectrum result(size);

    std::vector<juce::dsp::Complex<float>> input(size);
    for (int i = 0; i < size; ++i) 
    {
        input[i] = juce::dsp::Complex<float>(fft_input[i], 0);
    }
    
    std::vector<juce::dsp::Complex<float>> output(size);
    
    fft->perform(&input[0], &output[0], false);
    
    for (int i = 0; i < result.size(); ++i) 
    {
        result[i] = Complex(output[i].real(), output[i].imag());
    }

    return result;
}

// Run IFFT on a single frame
Frame SpectralSubtraction::calculateIFFT(Spectrum& ifft_input)
{
    int size = ifft_input.size();
    Frame result(size, 0);

    std::vector<juce::dsp::Complex<float>> input(size);
    for (int i = 0; i < size; ++i) {
        input[i] = juce::dsp::Complex<float>(ifft_input[i].real, ifft_input[i].imag);
    }
    
    std::vector<juce::dsp::Complex<float>> output(result.size());
    
    fft->perform(&input[0], &output[0], true);
    
    for (int i = 0; i < result.size(); ++i) {
        result[i] = output[i].real();
    }

 
    return result;
}

// Sets how much each window overlaps 
void SpectralSubtraction::setWindowOverlap(float overlap)
{
    windowOverlap = overlap;
    hopSize = windowSize * (1.f - windowOverlap);
}

// Create all window functions
void SpectralSubtraction::createWindow()
{
    windows.clear();
    for (int i = 0; i < Window::numWindowingMethods; ++i)
    {
        Frame tempWindow(windowSize, 0);
        Window::fillWindowingTables(&tempWindow[0], windowSize, (Window::WindowingMethod)i, true, 0.0);
        int k = 2;
        std::transform(tempWindow.begin(), tempWindow.end(), tempWindow.begin(), [k](double &c){ return c / k;});
        windows.push_back(tempWindow);
    }
}


// Split the given buffer into overlapping frames
Matrix SpectralSubtraction::createSignalFrames(const float* buffer, int size, int windowLength, int hopLength)
{
    // Calculate number of frames to fit in buffer
    int numFrames = numberFrames(size, windowLength, hopLength); 

    if (size < windowLength)
        numFrames = 1;

    Matrix frames(numFrames);

    // Create each frame
    for (int i = 0; i < numFrames; ++i)
    {
        Frame window(windowLength);
        for (int j = 0; j < windowLength; ++j)
        {
            int index = (i * hopLength) + j;
            window[j] = (index < size) ? buffer[index] : 0;
        }
        frames[i] = window;
    }
    return frames;
}



// Deframe samples using overlap and add to convert to output signal
std::vector<float> SpectralSubtraction::createSamplesFromFrames(const Matrix& frames, int windowLength, int hopLength)
{
    // Calculate number of samples from frames
    int numFrames = frames.size();
    int numSamples = (numFrames * windowLength) - ((numFrames - 1) * hopLength);
    std::vector<float> samples(numSamples);
    
    // Overlap and add frames
    for (int n = 0; n < numSamples; ++n)
    {
        double sum = 0;
        for (int m = 0; m < numFrames; ++m)
        {
            int index = n - (m * hopLength);
            if (index >= 0 && index < windowLength)
            {
                sum += frames[m][index];
            }
        }
        samples[n] = sum;
    }

    return samples;
}

// Calculate the start and end frequencies for each band
void SpectralSubtraction::calculateFrequencyBands()
{
    frequencyBandRanges.clear();

    float width = windowSize / (float)numFrequencyBands;
    for (int i = 0; i < numFrequencyBands; ++i)
    {
        int start = (int)(i * width);
        int end = (int)(start + width);
        std::pair<int, int> range(start, end);
        frequencyBandRanges.push_back(range);
    }
}

// Calculates the segmental SNR. ie. the ratio between the average of the noisy signal and the noise estimate
double SpectralSubtraction::segmentalSNR(const Spectrum& currFrame, const Frame& estimateFrame, const std::pair<int, int>& range)
{
    double a = sumSpectrum(currFrame, range) / sumFrame(estimateFrame, range);
    double snr = 10.0 * std::log10(a);
    return snr;
}

// Sums the magnitude of a given spectrum
double SpectralSubtraction::sumSpectrum(const Spectrum& frame, const std::pair<int, int>& range)
{
    double sum = 0;
    for (int i = range.first; i < range.second; ++i)
    {
        double mag = frame[i].magnitude();
        sum += subtractionDomain == 1 ? mag : mag * mag;
    }
    return sum;
}

// Sums the values in a given frame
double SpectralSubtraction::sumFrame(const Frame& frame, const std::pair<int, int>& range)
{
    double sum = 0;
    for (int i = range.first; i < range.second; ++i)
    {
        sum += frame[i];
    }
    return sum;
}

// Calculates the over subtraction factor based on the segmental SNR
double SpectralSubtraction::calculateOverSubtraction(double snr)
{
    if (snr < SNR_min)
        return subtractionAlpha;
    else if (snr >= SNR_min && snr <= SNR_max)
    {
        float y = (alpha_min - subtractionAlpha) / (SNR_max - SNR_min);
        return subtractionAlpha + (snr - SNR_min) * y;
    }
    else
        return alpha_min;

}

// Converts a spectrum to a frame of power values
Frame SpectralSubtraction::complexToPowerSpectrum(const Spectrum& spectrum)
{
    int size = spectrum.size();
    Frame powerSpectrum(size);
    for (int i = 0; i < size; ++i)
    {
        double mag = spectrum[i].magnitude();
        powerSpectrum[i] = mag * mag;
    }
    return powerSpectrum;
}

// Converts a spectrum to a frame of magnitude values
Frame SpectralSubtraction::complexToMagnitudeSpectrum(const Spectrum& spectrum)
{
    int size = spectrum.size();
    Frame powerSpectrum(size);
    for (int i = 0; i < size; ++i)
    {
        powerSpectrum[i] = spectrum[i].magnitude();
    }
    return powerSpectrum;
}


// Updates the noise estimation by interpolating between the input and running mean
void SpectralSubtraction::updateNoiseEstimation(const Frame& powerSpectrum)
{

    if (frameNumber < noiseProfileFrames)
    {
        noiseEstimation.push_back(powerSpectrum);
        ++frameNumber;
    }
    else
    {
        const Frame& prevFrame = noiseEstimation.front();
        Frame estimation(powerSpectrum.size());
        for (int w = 0; w < windowSize; ++w)
        {
            double snr = aposterioriSNR(powerSpectrum, w);
            a_SNR[w] = snr;
            double smoothing = calculateSmoothingParameter(snr);
            estimationSmoothing[w] = smoothing;
            const double& power = powerSpectrum[w];

            estimation[w] = (smoothing * prevFrame[w]) +
                ((1.0 - smoothing) * power);
        }
        noiseEstimation.pop_front();
        noiseEstimation.push_back(estimation);
    }
    
}

// Resets the estimation frames
void SpectralSubtraction::resetEstimation()
{
    std::fill(a_SNR.begin(), a_SNR.end(), 0.f);
    std::fill(estimationSmoothing.begin(), estimationSmoothing.end(), 0.f);
    frameNumber = 0;
    noiseEstimation.clear();
}

// Calculates the estimation smoothing value based on a-posteriori SNR
double SpectralSubtraction::calculateSmoothingParameter(double snr)
{
    float e = std::exp(-smoothingRate * (snr - smoothingCurve));
    double smoothing = 1.0 / (1.0 + e);
    return smoothing;
}


// Ratio of the noisy signal and the noise estimate
double SpectralSubtraction::aposterioriSNR(const Frame& powerSpectrum, int omega)
{
    double m = 1.0 / (double)noiseProfileFrames;

    double sum = 0;
    for (int p = 0; p < noiseEstimation.size(); ++p)
    {
        sum += noiseEstimation[p][omega];
    }
    double power = powerSpectrum[omega];
    double snr = 10.0 * std::log10(power / (m * sum));

    return snr;
}

// Ratio between the clean speech signal and the noise estimate
double SpectralSubtraction::aprioriSNR(const Frame& speechSpectrum, double apostSNR, int omega)
{
    double sum = 0;
    double m = 1.0 / (double)noiseProfileFrames;
    for (int p = 0; p < noiseEstimation.size(); ++p)
    {
        sum += noiseEstimation[p][omega];
    }
    double snr = 10.0 * std::log10(speechSpectrum[omega] / (m * sum));
    snr += (1.0 - snrQuality) * std::max(apostSNR - 1, 0.0);
    return snr;
}


// Gets the noise estimation based on settings
const Frame* SpectralSubtraction::getNoiseEstimation() const 
{
    if (adaptiveEstimationEnabled)
        return noiseEstimation.size() > 0 ? &noiseEstimation.back() : nullptr;
    else
        return averageNoise.size() > 0 ? &averageNoise : nullptr;
}
