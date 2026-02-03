#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

/**
 * Dynamic allpass filter for AM/PM simulation
 * Level-dependent phase shift: +3° @ +6dBu, +10° @ +12dBu
 */
class DynamicAllpass {
public:
  DynamicAllpass() { reset(); }

  void reset() { z1 = 0.0; }

  void prepare(double sampleRate) {
    // Envelope follower time constants
    attackCoeff = std::exp(-1.0 / (sampleRate * 0.005));  // 5ms attack
    releaseCoeff = std::exp(-1.0 / (sampleRate * 0.020)); // 20ms release
    coreStateCoeff =
        std::exp(-1.0 / (sampleRate * 0.020)); // 20ms for hysteresis
  }

  inline double process(double input, double drive) {
    // Envelope follower
    double inputAbs = std::abs(input);
    if (inputAbs > envelope)
      envelope = attackCoeff * envelope + (1.0 - attackCoeff) * inputAbs;
    else
      envelope = releaseCoeff * envelope + (1.0 - releaseCoeff) * inputAbs;

    // Update core state (slow magnetization memory)
    coreState = coreStateCoeff * coreState + (1.0 - coreStateCoeff) * envelope;

    // Allpass depth: 0-0.3 radians based on level and drive
    // 0.3 radians ≈ 17°, scaled by envelope and drive
    double depth = 0.3 * envelope * drive;
    double a = std::tanh(depth); // Clamp to stable range

    // First-order allpass: H(z) = (a + z^-1) / (1 + a*z^-1)
    double output = -a * input + z1;
    z1 = input + a * output;

    return output;
  }

  double getCoreState() const { return coreState; }

private:
  double z1 = 0.0;
  double envelope = 0.0;
  double coreState = 0.0;

  double attackCoeff = 0.99;
  double releaseCoeff = 0.995;
  double coreStateCoeff = 0.995;
};
