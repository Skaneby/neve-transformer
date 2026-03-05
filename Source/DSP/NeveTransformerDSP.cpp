#include "NeveTransformerDSP.h"

NeveTransformerDSP::NeveTransformerDSP() {}

void NeveTransformerDSP::prepare(double newSampleRate, int maxBlockSize) {
  sampleRate = newSampleRate;
  maxPreparedBlockSize = maxBlockSize;

  oversampler.prepare(sampleRate, maxBlockSize);
  doubleBuffer.setSize(2, maxBlockSize);

  for (int ch = 0; ch < 2; ++ch) {
    phaseEngine[ch].prepare(sampleRate * 4.0); // 4x oversampled rate
    dcBlocker[ch].setHighpass(sampleRate, 5.0, 0.707);
  }

  reset();
}

void NeveTransformerDSP::reset() {
  for (int ch = 0; ch < 2; ++ch) {
    phaseEngine[ch].reset();
    dcBlocker[ch].reset();
  }
  oversampler.reset();
}

void NeveTransformerDSP::processBlock(juce::AudioBuffer<float>& buffer) {
  if (bypassed.load(std::memory_order_relaxed))
    return;

  const int numSamples    = buffer.getNumSamples();
  const int inputChannels = buffer.getNumChannels();

  jassert(numSamples <= maxPreparedBlockSize);
  if (numSamples > doubleBuffer.getNumSamples())
    return;

  doubleBuffer.clear();

  // Float → double, duplicate mono to both channels if needed
  for (int ch = 0; ch < 2; ++ch) {
    int sourceCh = (ch < inputChannels) ? ch : 0;
    auto* src = buffer.getReadPointer(sourceCh);
    auto* dst = doubleBuffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i)
      dst[i] = static_cast<double>(src[i]);
  }

  jassert(numSamples <= oversampler.getPreparedBlockSize());
  if (numSamples > oversampler.getPreparedBlockSize())
    return;

  juce::dsp::AudioBlock<double> block(
      doubleBuffer.getArrayOfWritePointers(), 2, (size_t)numSamples);
  juce::dsp::AudioBlock<double> oversampledBlock = oversampler.upsample(block);

  const int oversampledSamples =
      static_cast<int>(oversampledBlock.getNumSamples());

  // Phase engine runs at 4x oversampled rate
  for (int ch = 0; ch < 2; ++ch) {
    auto* samples = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
    if (samples == nullptr) continue;
    for (int i = 0; i < oversampledSamples; ++i)
      samples[i] = phaseEngine[ch].processSample(samples[i]);
  }

  oversampler.downsample(block);

  // Double → float, DC block, soft clip at ±1
  for (int ch = 0; ch < inputChannels && ch < 2; ++ch) {
    auto* src = doubleBuffer.getReadPointer(ch);
    auto* dst = buffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      double s = dcBlocker[ch].process(src[i]);
      if      (s >  1.0) s =  1.0 - std::exp(-(s  - 1.0));
      else if (s < -1.0) s = -(1.0 - std::exp(-(-s - 1.0)));
      dst[i] = static_cast<float>(s);
    }
  }
}

int NeveTransformerDSP::getLatencySamples() const {
  return oversampler.getLatencySamples();
}

void NeveTransformerDSP::setBypassed(bool shouldBypass) {
  bypassed.store(shouldBypass, std::memory_order_release);
  if (shouldBypass)
    reset();
}
