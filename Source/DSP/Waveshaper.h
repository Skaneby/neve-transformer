#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

/**
 * Transformer saturation waveshaper
 * Asymmetric tanh-based transfer function with 3rd harmonic bias
 */
class Waveshaper {
public:
  Waveshaper() { setDrive(0.5); }

  void setDrive(double driveAmount) {
    // Drive: 0-1 → scales input intensity
    drive = juce::jlimit(0.0, 1.0, driveAmount);
    inputScale = 1.0 + drive * 2.0; // 1.0 to 3.0 range
  }

  inline double process(double input) {
    // Scale by drive
    double theta = input * inputScale;

    // Asymmetric saturation (different curves for pos/neg)
    if (theta >= 0.0) {
      // Positive: tanh(1.5θ + 0.3θ³) - emphasizes 3rd harmonic
      double theta3 = theta * theta * theta;
      return std::tanh(1.5 * theta + 0.3 * theta3);
    } else {
      // Negative: slightly compressed for asymmetry (even harmonics)
      double thetaNeg = -theta * 0.95;
      double theta3 = thetaNeg * thetaNeg * thetaNeg;
      return -std::tanh(1.5 * thetaNeg + 0.3 * theta3);
    }
  }

  // Process with hysteresis state (adds memory to saturation)
  inline double processWithHysteresis(double input, double coreState) {
    // Core state: 0-1, represents magnetization memory
    double scale = 1.0 + 0.2 * coreState; // Mild hysteresis effect
    double theta = input * inputScale * scale;

    if (theta >= 0.0) {
      double theta3 = theta * theta * theta;
      return std::tanh(1.5 * theta + 0.3 * theta3);
    } else {
      double thetaNeg = -theta * 0.95;
      double theta3 = thetaNeg * thetaNeg * thetaNeg;
      return -std::tanh(1.5 * thetaNeg + 0.3 * theta3);
    }
  }

private:
  double drive = 0.5;
  double inputScale = 2.0;
};
