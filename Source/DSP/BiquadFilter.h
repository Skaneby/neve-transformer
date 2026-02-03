#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/**
 * High-precision biquad filter for transformer emulation
 * Direct Form II Transposed for numerical stability
 */
class BiquadFilter {
public:
  BiquadFilter() { reset(); }

  void reset() { z1 = z2 = 0.0; }

  // Process single sample
  inline double process(double input) {
    double output = input * b0 + z1;
    z1 = input * b1 - output * a1 + z2;
    z2 = input * b2 - output * a2;
    return output;
  }

  // Factory: Lowpass for LF pole (~60 Hz, magnetization inductance)
  static BiquadFilter makeLowpass(double sampleRate, double fc, double Q) {
    BiquadFilter filter;
    double w0 = juce::MathConstants<double>::twoPi * fc / sampleRate;
    double cosw0 = std::cos(w0);
    double sinw0 = std::sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double a0 = 1.0 + alpha;
    filter.b0 = ((1.0 - cosw0) / 2.0) / a0;
    filter.b1 = (1.0 - cosw0) / a0;
    filter.b2 = ((1.0 - cosw0) / 2.0) / a0;
    filter.a1 = (-2.0 * cosw0) / a0;
    filter.a2 = (1.0 - alpha) / a0;

    return filter;
  }

  // Factory: Peak for HF resonance (~14 kHz, leakage + capacitance)
  static BiquadFilter makePeak(double sampleRate, double fc, double gainDB,
                               double Q) {
    BiquadFilter filter;
    double A = std::pow(10.0, gainDB / 40.0);
    double w0 = juce::MathConstants<double>::twoPi * fc / sampleRate;
    double cosw0 = std::cos(w0);
    double sinw0 = std::sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double a0 = 1.0 + alpha / A;
    filter.b0 = (1.0 + alpha * A) / a0;
    filter.b1 = (-2.0 * cosw0) / a0;
    filter.b2 = (1.0 - alpha * A) / a0;
    filter.a1 = (-2.0 * cosw0) / a0;
    filter.a2 = (1.0 - alpha / A) / a0;

    return filter;
  }

  // Factory: High shelf for post-EQ tilt
  static BiquadFilter makeHighShelf(double sampleRate, double fc, double gainDB,
                                    double Q = 0.707) {
    BiquadFilter filter;
    double A = std::pow(10.0, gainDB / 40.0);
    double w0 = juce::MathConstants<double>::twoPi * fc / sampleRate;
    double cosw0 = std::cos(w0);
    double sinw0 = std::sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double a0 = (A + 1.0) - (A - 1.0) * cosw0 + 2.0 * std::sqrt(A) * alpha;
    filter.b0 =
        (A * ((A + 1.0) + (A - 1.0) * cosw0 + 2.0 * std::sqrt(A) * alpha)) / a0;
    filter.b1 = (-2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0)) / a0;
    filter.b2 =
        (A * ((A + 1.0) + (A - 1.0) * cosw0 - 2.0 * std::sqrt(A) * alpha)) / a0;
    filter.a1 = (2.0 * ((A - 1.0) - (A + 1.0) * cosw0)) / a0;
    filter.a2 =
        ((A + 1.0) - (A - 1.0) * cosw0 - 2.0 * std::sqrt(A) * alpha) / a0;

    return filter;
  }

  // Factory: Low shelf for "Iron" control (LF boost pre-saturation)
  static BiquadFilter makeLowShelf(double sampleRate, double fc, double gainDB,
                                   double Q = 0.707) {
    BiquadFilter filter;
    double A = std::pow(10.0, gainDB / 40.0);
    double w0 = juce::MathConstants<double>::twoPi * fc / sampleRate;
    double cosw0 = std::cos(w0);
    double sinw0 = std::sin(w0);
    double alpha = sinw0 / (2.0 * Q);

    double a0 = (A + 1.0) + (A - 1.0) * cosw0 + 2.0 * std::sqrt(A) * alpha;
    filter.b0 =
        (A * ((A + 1.0) - (A - 1.0) * cosw0 + 2.0 * std::sqrt(A) * alpha)) / a0;
    filter.b1 = (2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0)) / a0;
    filter.b2 =
        (A * ((A + 1.0) - (A - 1.0) * cosw0 - 2.0 * std::sqrt(A) * alpha)) / a0;
    filter.a1 = (-2.0 * ((A - 1.0) + (A + 1.0) * cosw0)) / a0;
    filter.a2 =
        ((A + 1.0) + (A - 1.0) * cosw0 - 2.0 * std::sqrt(A) * alpha) / a0;

    return filter;
  }

private:
  // Direct Form II state
  double z1 = 0.0;
  double z2 = 0.0;

  // Coefficients
  double b0 = 1.0, b1 = 0.0, b2 = 0.0;
  double a1 = 0.0, a2 = 0.0;
};
