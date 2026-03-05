#pragma once

#include "BiquadFilter.h"
#include "Neve8026PhaseEngine.h"
#include "Oversampler.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

/**
 * Neve Bättre — pure phase signature processor.
 * Signal chain: float in → double → 4x FIR oversample
 *               → Neve8026PhaseEngine (3-stage allpass)
 *               → downsample → DC block → float out
 */
class NeveTransformerDSP {
public:
  NeveTransformerDSP();

  void prepare(double sampleRate, int maxBlockSize);
  void reset();
  void processBlock(juce::AudioBuffer<float>& buffer);

  void setBypassed(bool shouldBypass);

  int getLatencySamples() const;

private:
  double sampleRate = 48000.0;
  int maxPreparedBlockSize = 0;

  std::atomic<bool> bypassed { false };

  Neve8026PhaseEngine phaseEngine[2];
  BiquadFilter        dcBlocker[2];

  juce::AudioBuffer<double> doubleBuffer;
  Oversampler oversampler;
};
