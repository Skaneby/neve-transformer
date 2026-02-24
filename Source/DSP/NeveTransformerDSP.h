#pragma once

#include "BiquadFilter.h"
#include "Neve8026PhaseEngine.h"
#include "Oversampler.h"
#include "Waveshaper.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

/**
 * Complete Neve transformer emulation DSP processor
 * Signal chain: Pre-filter (IIR) -> Oversample -> Nonlinear Core -> Downsample ->
 * Post-filter
 */
class NeveTransformerDSP {
public:
  NeveTransformerDSP();

  void prepare(double sampleRate, int maxBlockSize);
  void reset();
  void processBlock(juce::AudioBuffer<float> &buffer);

  // Parameter setters (0-1 normalized)
  void setDrive(double value);
  void setIron(double value);
  void setHFRoll(double value);
  void setMode(bool isMic);
  void setZLoad(bool isHigh);
  void setBypassed(bool shouldBypass);
  void setSoundType(SoundType type);

  int getLatencySamples() const;

private:
  void updateFilters();

  double sampleRate = 48000.0;
  int maxPreparedBlockSize = 0;

  // Smoothed Parameters
  juce::LinearSmoothedValue<double> driveParam { 0.3 };
  juce::LinearSmoothedValue<double> ironParam { 0.5 };
  juce::LinearSmoothedValue<double> hfRollParam { 0.7 };

  std::atomic<bool> micMode { false };
  std::atomic<bool> highZLoad { true };
  std::atomic<bool> bypassed { false };
  std::atomic<int>  pendingSoundType { 0 }; // SoundType index, -1 = no change pending

  // Atomic dirty flag for thread-safe filter updates
  std::atomic<bool> filtersDirty { true };

  // Linear pre-transformer filters (per channel)
  BiquadFilter lfPoleFilter[2];
  BiquadFilter hfResonanceFilter[2];
  BiquadFilter ironFilter[2];
  BiquadFilter hfRollFilter[2];

  // Linear post-transformer filter
  BiquadFilter postShelfFilter[2];
  BiquadFilter dcBlocker[2];

  // Nonlinear core
  Waveshaper waveshaper[2];
  Neve8026PhaseEngine phaseEngine[2]; // Neve 8026 Marinair 3-stage allpass chain

  // Pre-allocated buffers for audio thread
  juce::AudioBuffer<double> doubleBuffer;

  // Oversampling
  Oversampler oversampler;
};
