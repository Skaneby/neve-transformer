#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

/**
 * Transformer saturation waveshaper
 * Asymmetric tanh-based transfer function with 3rd harmonic bias.
 * Output gain-compensated to maintain unity at small signal levels.
 */
class Waveshaper {
public:
  Waveshaper() { setDrive(0.5); }

  void setDrive(double driveAmount) {
    drive = juce::jlimit(0.0, 1.0, driveAmount);
    inputScale = 1.0 + drive * 3.0; // 1.0 to 4.0 range
    outputGain = 1.0 / (1.5 * inputScale);
  }

  void reset() {}

  inline double process(double input) {
    double x = input * inputScale;
    return transferFunction(x) * outputGain;
  }

  inline double processWithHysteresis(double input, double coreState, int /*channel*/) {
    double scale = 1.0 + 0.2 * coreState;
    double x = input * inputScale * scale;
    double normGain = 1.0 / (1.5 * inputScale * scale);
    return transferFunction(x) * normGain;
  }

private:
  inline double transferFunction(double x) const {
    if (x >= 0.0) {
      double x3 = x * x * x;
      return std::tanh(1.5 * x + 0.3 * x3);
    } else {
      double xNeg = -x * 0.95;
      double xNeg3 = xNeg * xNeg * xNeg;
      return -std::tanh(1.5 * xNeg + 0.3 * xNeg3);
    }
  }

  double drive = 0.5;
  double inputScale = 2.0;
  double outputGain = 1.0 / (1.5 * 2.0);
};
