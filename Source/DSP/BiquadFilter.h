#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/**
 * High-precision biquad filter for transformer emulation
 */
class BiquadFilter {
public:
  BiquadFilter() { reset(); }

  void reset() { z1 = z2 = 0.0; }

  inline double process(double input) {
    double output = input * b0 + z1;
    z1 = input * b1 - output * a1 + z2;
    z2 = input * b2 - output * a2;
    // Flush denormals to zero to prevent CPU spikes on quiet signals
    if (!(z1 > 1e-15 || z1 < -1e-15)) z1 = 0.0;
    if (!(z2 > 1e-15 || z2 < -1e-15)) z2 = 0.0;
    return output;
  }

  void setLowpass(double sampleRate, double fc, double Q) {
    double w0 = juce::MathConstants<double>::twoPi * fc / sampleRate;
    double alpha = std::sin(w0) / (2.0 * Q);
    double cosw0 = std::cos(w0);

    double a0 = 1.0 + alpha;
    b0 = ((1.0 - cosw0) / 2.0) / a0;
    b1 = (1.0 - cosw0) / a0;
    b2 = ((1.0 - cosw0) / 2.0) / a0;
    a1 = (-2.0 * cosw0) / a0;
    a2 = (1.0 - alpha) / a0;
  }

  void setHighpass(double sampleRate, double fc, double Q) {
    double w0 = juce::MathConstants<double>::twoPi * fc / sampleRate;
    double alpha = std::sin(w0) / (2.0 * Q);
    double cosw0 = std::cos(w0);

    double a0 = 1.0 + alpha;
    b0 = ((1.0 + cosw0) / 2.0) / a0;
    b1 = -(1.0 + cosw0) / a0;
    b2 = ((1.0 + cosw0) / 2.0) / a0;
    a1 = (-2.0 * cosw0) / a0;
    a2 = (1.0 - alpha) / a0;
  }

  void setPeak(double sampleRate, double fc, double gainDB, double Q) {
    double A = std::pow(10.0, gainDB / 40.0);
    double w0 = juce::MathConstants<double>::twoPi * fc / sampleRate;
    double alpha = std::sin(w0) / (2.0 * Q);
    double cosw0 = std::cos(w0);

    double a0 = 1.0 + alpha / A;
    b0 = (1.0 + alpha * A) / a0;
    b1 = (-2.0 * cosw0) / a0;
    b2 = (1.0 - alpha * A) / a0;
    a1 = (-2.0 * cosw0) / a0;
    a2 = (1.0 - alpha / A) / a0;
  }

  void setLowShelf(double sampleRate, double fc, double gainDB, double Q = 0.707) {
    double A = std::pow(10.0, gainDB / 40.0);
    double w0 = juce::MathConstants<double>::twoPi * fc / sampleRate;
    double alpha = std::sin(w0) / (2.0 * Q);
    double cosw0 = std::cos(w0);
    double sqrtA2alpha = 2.0 * std::sqrt(A) * alpha;

    double a0 = (A + 1.0) + (A - 1.0) * cosw0 + sqrtA2alpha;
    b0 = (A * ((A + 1.0) - (A - 1.0) * cosw0 + sqrtA2alpha)) / a0;
    b1 = (2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0)) / a0;
    b2 = (A * ((A + 1.0) - (A - 1.0) * cosw0 - sqrtA2alpha)) / a0;
    a1 = (-2.0 * ((A - 1.0) + (A + 1.0) * cosw0)) / a0;
    a2 = ((A + 1.0) + (A - 1.0) * cosw0 - sqrtA2alpha) / a0;
  }

  void setHighShelf(double sampleRate, double fc, double gainDB, double Q = 0.707) {
    double A = std::pow(10.0, gainDB / 40.0);
    double w0 = juce::MathConstants<double>::twoPi * fc / sampleRate;
    double alpha = std::sin(w0) / (2.0 * Q);
    double cosw0 = std::cos(w0);
    double sqrtA2alpha = 2.0 * std::sqrt(A) * alpha;

    double a0 = (A + 1.0) - (A - 1.0) * cosw0 + sqrtA2alpha;
    b0 = (A * ((A + 1.0) + (A - 1.0) * cosw0 + sqrtA2alpha)) / a0;
    b1 = (-2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0)) / a0;
    b2 = (A * ((A + 1.0) - (A - 1.0) * cosw0 - sqrtA2alpha)) / a0;
    a1 = (2.0 * ((A - 1.0) - (A + 1.0) * cosw0)) / a0;
    a2 = ((A + 1.0) - (A - 1.0) * cosw0 - sqrtA2alpha) / a0;
  }

private:
  double z1 = 0.0, z2 = 0.0;
  double b0 = 1.0, b1 = 0.0, b2 = 0.0;
  double a1 = 0.0, a2 = 0.0;
};
