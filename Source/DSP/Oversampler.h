#pragma once

#include <juce_dsp/juce_dsp.h>

/**
 * Wrapper around JUCE's oversampling for anti-aliasing
 * 4x polyphase FIR
 */
class Oversampler {
public:
  Oversampler() {
    // 4x oversampling with linear-phase equiripple FIR for best alias rejection
    oversampler = std::make_unique<juce::dsp::Oversampling<double>>(
        2, // num channels
        2, // factor exponent (4x)
        juce::dsp::Oversampling<double>::filterHalfBandFIREquiripple,
        true, // isMaximumQuality
        true  // useIntegerLatency
    );
  }

  void prepare(double sampleRate, int maxBlockSize) {
    // Prepare oversampler
    currentMaxBlockSize = maxBlockSize;
    oversampler->initProcessing(maxBlockSize);
    oversampler->reset();
  }

  int getPreparedBlockSize() const { return currentMaxBlockSize; }

  void reset() { oversampler->reset(); }

  int getLatencySamples() const {
    return static_cast<int>(oversampler->getLatencyInSamples());
  }

  // Upsample input block
  juce::dsp::AudioBlock<double>
  upsample(juce::dsp::AudioBlock<double> &inputBlock) {
    return oversampler->processSamplesUp(inputBlock);
  }

  // Downsample processed block
  void downsample(juce::dsp::AudioBlock<double> &outputBlock) {
    oversampler->processSamplesDown(outputBlock);
  }

private:
  std::unique_ptr<juce::dsp::Oversampling<double>> oversampler;
  int currentMaxBlockSize = 0;
};
