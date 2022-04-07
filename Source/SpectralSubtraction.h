/*
  ==============================================================================

    SpectralSubtraction.h
    Created: 7 Feb 2022 2:37:57pm
    Author:  Bennett

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "CircularBuffer.h"




struct Complex
{
    Complex() : real(0), imag(0) {}
    Complex(double r, double i) : real(r), imag(i) {}

    double magnitude() const {
        return std::sqrt((real * real) + (imag * imag));
    }

    double phase() const {
        if (real > 0)
            return std::atan(imag / real);
        else if (real < 0)
            return std::atan(imag / real) + juce::MathConstants<double>::pi;
        return 0.0;
    }

    std::pair<double, double> polar() {
        return std::make_pair<double, double>(magnitude(), phase());
    }

    Complex conjugate() const {
        return Complex(real, -imag);
    }

    double real;
    double imag;
};

typedef std::vector<double> Frame;
typedef std::vector<Complex> Spectrum;
typedef std::vector<Frame> Matrix;
typedef std::vector<Spectrum> SpectrumMatrix;
typedef juce::dsp::WindowingFunction<double> Window;


class SpectralSubtraction
{
    public:
        SpectralSubtraction(int fft_order);
        ~SpectralSubtraction();

        // Processing
        void processBuffer(float* buffer, int size);
        Matrix processSubtraction(const std::vector<Spectrum>& frequencyData);

        // Signal
        void setSignal(juce::AudioSampleBuffer* buffer);
        juce::AudioSampleBuffer* getSignal() const { return signal; }
        bool isSignalSet() { return signal != nullptr; }
        void setSampleRate(float rate) { sampleRate = rate; }

        // Noise profile
        Frame computeFileNoiseProfile();
        Frame bufferToNoiseProfile(const std::vector<float>& buffer);
        const Frame& getAverageNoise() const { return averageNoise; }
        void setAverageNoise(const Frame& noise) { averageNoise = noise; }
        void setNoiseProfileFrames(int numFrames) { noiseProfileFrames = numFrames; }
        int getNoiseProfileSize() { return (noiseProfileFrames * windowSize) - ((noiseProfileFrames - 1) * hopSize); }
        const Frame* getNoiseEstimation() const;
        void updateNoiseEstimation(const Frame& powerSpectrum);
        const Frame& getAPosSNR() const { return a_SNR; }
        const Frame& getEstimationSmoothing() const { return estimationSmoothing; }
        void resetEstimation();
        const Frame& getNoiseSubtracted() { return noiseSubtracted; }

        // Window
        const Frame& getWindow() const { return windows[windowType]; }
        const std::vector<Frame>& getWindows() const { return windows; }
        int getWindowSize() const { return windowSize;}
        Window::WindowingMethod getWindowType() const { return windowType; }
        void setWindowType(Window::WindowingMethod windowMethod) { windowType = windowMethod; }
        int numberFrames(int size, int windowLength, int hopLength) { return 1 + std::floor((size - windowLength) / (float)hopLength);}
        void setWindowOverlap(float overlap);
        float getWindowOverlap() const { return windowOverlap; }

        // FFT Order
        int getFFTOrder() const { return order; }
        void setFFTOrder(int fft_order);

        // Subtraction Constant
        const double& getSubtractionConstant() { return subtractionAlpha; }
        void setSubtractionConstant(const double& constant) { subtractionAlpha = constant; }

        // Subtraction Floor
        const double& getSubtractionFloor() { return subtractionFloor; }
        void setSubtractionFloor(const double& floor) { subtractionFloor = floor; }

        // Subtraction Domain
        int getSubtractionDomain() const { return subtractionDomain; }
        void setSubtractionDomain(const int& domain) { subtractionDomain = domain; }

        // Noise Estimation Enabled
        bool getNoiseEstimationEnabled() const { return noiseEstimationEnabled; }
        void setNoiseEstimationEnabled(bool enabled) { noiseEstimationEnabled = enabled; }

        // Subtraction Enabled
        bool getSubtractionEnabled() const { return subtractionEnabled; }
        void setSubtractionEnabled(bool enabled) { subtractionEnabled = enabled; }

        // Adaptive Estimation Enabled
        bool getAdaptivateEstimationEnabled() const { return adaptiveEstimationEnabled; }
        void setAdaptiveEstimationEnabled(bool enabled) { adaptiveEstimationEnabled = enabled; }

        // Smoothing Rate
        float getSmoothingRate() const { return smoothingRate; }
        void setSmoothingRate(float a) { smoothingRate = a; }

        // Smoothing Curve
        float getSmoothingCurve() const { return smoothingCurve; }
        void setSmoothingCurve(float T) { smoothingCurve = T; }
        
        // Frequency Bands
        int getNumFrequencyBands() { return numFrequencyBands; }
        void setNumFrequencyBands(int num) { numFrequencyBands = num; calculateFrequencyBands(); }

        // Band Weight
        void setBandWeight(int index, double weight) { bandWeights[index] = weight; }

    private:
        bool noiseEstimationEnabled = false;
        bool subtractionEnabled = false;
        bool adaptiveEstimationEnabled = false;

        float sampleRate = 48000;
        juce::AudioSampleBuffer* signal = nullptr;

        // Subtraction Parameters
        double subtractionAlpha = 4;
        double subtractionFloor = 0.03;
        int subtractionDomain = 1;

        // FFT
        juce::dsp::FFT* fft = nullptr;
        int order;
        int NFFT;
        int noiseProfileFrames = 10;

        // Noise Estimation
        std::deque<Frame> noiseEstimation;
        Frame averageNoise;
        Frame noiseSubtracted;
        Frame a_SNR;
        Frame estimationSmoothing;
        float SNR_min = -5;
        float SNR_max = 20;
        float snrQuality = 0.98f;
        float alpha_min = 1;
        float smoothingCurve = 3; // T
        float smoothingRate = 3;
        int frameNumber = 0;

        // Window
        std::vector<Frame> windows;
        Window::WindowingMethod windowType = Window::hamming;
        int windowSize;
        float windowOverlap = 0.5f;
        int hopSize;

        // Frequency Bands
        int numFrequencyBands = 1;
        std::vector<double> bandWeights;
        std::vector<std::pair<int, int>> frequencyBandRanges;




        // Helper functions
        SpectrumMatrix createFrequencyData(const Matrix& frames);
        std::vector<float> createSamplesFromFrames(const Matrix& frames, int windowLength, int hopLength);
        Matrix createSignalFrames(const float* buffer, int size, int windowLength, int hopLength);
        Frame noiseAverageSpectrum(const Matrix& frames);
        Spectrum frequencySpectrum(const Frame& frame);
        void createWindow();
        Spectrum calculateFFT(Frame& fft_input);
        Frame calculateIFFT(Spectrum& ifft_input);
        double segmentalSNR(const Spectrum& currFrame, const Frame& estimateFrame, const std::pair<int, int>& range);
        double aposterioriSNR(const Frame& powerSpectrum, int omega);
        double aprioriSNR(const Frame& speechSpectrum, double apostSNR, int omega);
        double calculateOverSubtraction(double snr);
        double calculateSmoothingParameter(double snr);
        double sumSpectrum(const Spectrum& frame, const std::pair<int, int>& range);
        double sumFrame(const Frame& frame, const std::pair<int, int>& range);
        void calculateFrequencyBands();
        Frame complexToPowerSpectrum(const Spectrum& spectrum);
        Frame complexToMagnitudeSpectrum(const Spectrum& spectrum);       
};