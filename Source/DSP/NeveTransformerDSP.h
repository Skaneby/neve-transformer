#pragma once

#include "BiquadFilter.h"
#include "DynamicAllpass.h"
#include "Oversampler.h"
#include "Waveshaper.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

/**
 * Complete Neve transformer emulation DSP processor
 * Signal chain: Pre-filter (IIR) → Oversample → Nonlinear Core → Downsample →
 * Post-filter
 */
class NeveTransformerDSP {
public:
  NeveTransformerDSP();

  void prepare(double sampleRate, int maxBlockSize);
  void reset();
  void processBlock(juce::AudioBuffer<float> &buffer);

  // Parameter setters (0-1 normalized)
  void setDrive(double value);  // 0-1: saturation intensity
  void setIron(double value);   // 0-1: LF boost (magnetization)
  void setHFRoll(double value); // 0-1: HF cutoff (20-30 kHz)
  void setMode(bool isMic);     // Mic/Line impedance
  void setZLoad(bool isHigh);   // Hi/Lo load
  void setBypassed(bool shouldBypass);

  int getLatencySamples() const;

private:
  void updateFilters();
  double processSample(double input, int channel);

  // Sample rate
  double sampleRate = 48000.0;

  // Parameters
  double driveParam = 0.3;
  double ironParam = 0.5;
  double hfRollParam = 0.7; // Default ~25 kHz
  bool micMode = false;
  bool highZLoad = true;
  bool bypassed = false;

  // Linear pre-transformer filters (per channel)
  BiquadFilter lfPoleFilter[2];      // ~60 Hz lowpass (magnetization)
  BiquadFilter hfResonanceFilter[2]; // ~14 kHz peak (leakage)
  BiquadFilter ironFilter[2];        // 100 Hz shelf (Iron control)

  // Linear post-transformer filter
  BiquadFilter postShelfFilter[2]; // 80 Hz shelf (+0.2 dB for "thick")

  // Nonlinear core
  Waveshaper waveshaper[2];
  DynamicAllpass allpass[2];

  // Pre-allocated buffers for audio thread
  juce::AudioBuffer<double> doubleBuffer;

  // Oversampling
  Oversampler oversampler;
};
