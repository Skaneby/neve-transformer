#pragma once

#include <juce_dsp/juce_dsp.h>

/**
 * Wrapper around JUCE's oversampling for anti-aliasing
 * 4x polyphase FIR
 */
class Oversampler {
public:
  Oversampler() {
    // 4x oversampling, using steep filter (quality preset)
    oversampler = std::make_unique<juce::dsp::Oversampling<double>>(
        2, // num channels
        2, // factor exponent (4x)
        juce::dsp::Oversampling<double>::filterHalfBandPolyphaseIIR,
        true, // isStagedOffsetActive
        true  // useSteepFilter
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
