#include "NeveTransformerDSP.h"

NeveTransformerDSP::NeveTransformerDSP() {}

void NeveTransformerDSP::prepare(double newSampleRate, int maxBlockSize) {
  sampleRate = newSampleRate;
  maxPreparedBlockSize = maxBlockSize;

  // Prepare oversampler (4x = 192 kHz for 48 kHz input)
  oversampler.prepare(sampleRate, maxBlockSize);

  // Allocate double buffer
  doubleBuffer.setSize(2, maxBlockSize);

  // Prepare dynamic components
  for (int ch = 0; ch < 2; ++ch) {
    // Phase engine runs at 4x oversampled rate for maximum accuracy
    phaseEngine[ch].prepare(sampleRate * 4.0);
    waveshaper[ch].setDrive(driveParam.getTargetValue());
  }

  // Prepare smoothed params (0.05s ramp)
  driveParam.reset(sampleRate, 0.05);
  ironParam.reset(sampleRate, 0.05);
  hfRollParam.reset(sampleRate, 0.05);

  filtersDirty.store(true, std::memory_order_release);
  reset();
}

void NeveTransformerDSP::reset() {
  for (int ch = 0; ch < 2; ++ch) {
    lfPoleFilter[ch].reset();
    hfResonanceFilter[ch].reset();
    ironFilter[ch].reset();
    hfRollFilter[ch].reset();
    postShelfFilter[ch].reset();
    dcBlocker[ch].reset();
    phaseEngine[ch].reset();
    waveshaper[ch].reset();
  }
  oversampler.reset();
}

void NeveTransformerDSP::updateFilters() {
  double lfCutoff = micMode.load(std::memory_order_relaxed) ? 50.0 : 60.0;
  double lfQ = 0.7;

  double hfPeakFreq = 14000.0;
  double hfPeakGain = 1.5;
  double hfPeakQ = 1.2;

  double hfRollFreq = 20000.0 + hfRollParam.getCurrentValue() * 10000.0;

  double maxFreq = sampleRate * 0.48;
  hfRollFreq = juce::jmin(hfRollFreq, maxFreq);
  hfPeakFreq = juce::jmin(hfPeakFreq, maxFreq);

  double ironGain = ironParam.getCurrentValue() * 2.0;
  double postThickGain = 0.2;

  if (!highZLoad.load(std::memory_order_relaxed))
    hfPeakQ = 0.8;

  for (int ch = 0; ch < 2; ++ch) {
    lfPoleFilter[ch].setHighpass(sampleRate, lfCutoff, lfQ);
    hfResonanceFilter[ch].setPeak(sampleRate, hfPeakFreq, hfPeakGain, hfPeakQ);
    ironFilter[ch].setLowShelf(sampleRate, 100.0, ironGain, 0.707);
    hfRollFilter[ch].setLowpass(sampleRate, hfRollFreq, 0.707);
    postShelfFilter[ch].setLowShelf(sampleRate, 80.0, postThickGain, 0.707);
    dcBlocker[ch].setHighpass(sampleRate, 5.0, 0.707);
  }
}

void NeveTransformerDSP::processBlock(juce::AudioBuffer<float> &buffer) {
  if (bypassed.load(std::memory_order_relaxed))
    return;

  const int numSamples = buffer.getNumSamples();

  // Advance smoothed filter parameters and recalculate coefficients
  // when they are still ramping or when mode/zLoad changed (dirty flag).
  // This prevents harsh transients from instant coefficient jumps.
  bool needFilterUpdate = filtersDirty.exchange(false, std::memory_order_acquire);
  if (ironParam.isSmoothing() || hfRollParam.isSmoothing())
    needFilterUpdate = true;
  if (needFilterUpdate) {
    ironParam.skip(numSamples);
    hfRollParam.skip(numSamples);
    updateFilters();
  }

  // Apply pending sound type change (set from message thread via setSoundType)
  int pending = pendingSoundType.exchange(-1, std::memory_order_acquire);
  if (pending >= 0) {
    auto type = static_cast<SoundType>(pending);
    for (int ch = 0; ch < 2; ++ch)
      phaseEngine[ch].setSound(type);
  }
  const int inputChannels = buffer.getNumChannels();

  // Safety: skip processing if block exceeds pre-allocated capacity
  jassert(numSamples <= maxPreparedBlockSize);
  if (numSamples > doubleBuffer.getNumSamples())
    return;

  doubleBuffer.clear();

  for (int ch = 0; ch < 2; ++ch) {
    int sourceCh = (ch < inputChannels) ? ch : 0;

    if (sourceCh >= inputChannels) {
      doubleBuffer.clear(ch, 0, numSamples);
      continue;
    }

    auto *input = buffer.getReadPointer(sourceCh);
    auto *output = doubleBuffer.getWritePointer(ch);

    for (int i = 0; i < numSamples; ++i) {
      double sample = static_cast<double>(input[i]);

      sample = ironFilter[ch].process(sample);
      sample = lfPoleFilter[ch].process(sample);
      sample = hfResonanceFilter[ch].process(sample);
      sample = hfRollFilter[ch].process(sample);

      output[i] = sample;
    }
  }

  // Safety: skip oversampling if block exceeds prepared size
  jassert(numSamples <= oversampler.getPreparedBlockSize());
  if (numSamples > oversampler.getPreparedBlockSize())
    return;

  juce::dsp::AudioBlock<double> block(doubleBuffer.getArrayOfWritePointers(), 2, (size_t)numSamples);
  juce::dsp::AudioBlock<double> oversampledBlock = oversampler.upsample(block);

  const int oversampledSamples =
      static_cast<int>(oversampledBlock.getNumSamples());

  // Step driveParam once per base-rate sample to maintain correct smoothing rate.
  // Update waveshaper drive from the smoothed value (not target) to avoid zipper noise.
  const int oversampleFactor = 4;
  for (int baseSample = 0; baseSample < numSamples; ++baseSample) {
    double currentDrive = driveParam.getNextValue();

    // Update waveshaper drive from smoothed value
    for (int ch = 0; ch < 2; ++ch)
      waveshaper[ch].setDrive(currentDrive);

    for (int os = 0; os < oversampleFactor; ++os) {
      int i = baseSample * oversampleFactor + os;
      if (i >= oversampledSamples) break;

      for (int ch = 0; ch < 2; ++ch) {
        auto *samples = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
        if (samples == nullptr) continue;

        double sample = samples[i];

        // Saturation → Neve 8026 Marinair phase character
        sample = waveshaper[ch].process(sample);
        sample = phaseEngine[ch].processSample(sample);

        samples[i] = sample;
      }
    }
  }

  oversampler.downsample(block);

  for (int ch = 0; ch < inputChannels && ch < 2; ++ch) {
    auto *input = doubleBuffer.getReadPointer(ch);
    auto *output = buffer.getWritePointer(ch);

    for (int i = 0; i < numSamples; ++i) {
      double sample = input[i];
      sample = postShelfFilter[ch].process(sample);
      sample = dcBlocker[ch].process(sample);
      // Soft limit to prevent DAC clipping
      if (sample > 1.0)
        sample = 1.0 - std::exp(-(sample - 1.0));
      else if (sample < -1.0)
        sample = -(1.0 - std::exp(-(-sample - 1.0)));
      output[i] = static_cast<float>(sample);
    }
  }
}

int NeveTransformerDSP::getLatencySamples() const {
  return oversampler.getLatencySamples();
}

void NeveTransformerDSP::setDrive(double value) {
  driveParam.setTargetValue(juce::jlimit(0.0, 1.0, value));
}

void NeveTransformerDSP::setIron(double value) {
  ironParam.setTargetValue(juce::jlimit(0.0, 1.0, value));
  filtersDirty.store(true, std::memory_order_release);
}

void NeveTransformerDSP::setHFRoll(double value) {
  hfRollParam.setTargetValue(juce::jlimit(0.0, 1.0, value));
  filtersDirty.store(true, std::memory_order_release);
}

void NeveTransformerDSP::setMode(bool isMic) {
  micMode.store(isMic, std::memory_order_release);
  filtersDirty.store(true, std::memory_order_release);
}

void NeveTransformerDSP::setZLoad(bool isHigh) {
  highZLoad.store(isHigh, std::memory_order_release);
  filtersDirty.store(true, std::memory_order_release);
}

void NeveTransformerDSP::setBypassed(bool shouldBypass) {
  bypassed.store(shouldBypass, std::memory_order_release);
  if (shouldBypass)
    reset();
}

void NeveTransformerDSP::setSoundType(SoundType type) {
  pendingSoundType.store(static_cast<int>(type), std::memory_order_release);
}
