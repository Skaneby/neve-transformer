#include "NeveTransformerDSP.h"

NeveTransformerDSP::NeveTransformerDSP() {}

void NeveTransformerDSP::prepare(double newSampleRate, int maxBlockSize) {
  sampleRate = newSampleRate;

  // Prepare oversampler (4x = 192 kHz for 48 kHz input)
  oversampler.prepare(sampleRate, maxBlockSize);

  // Allocate oversampled buffer
  doubleBuffer.setSize(2, maxBlockSize);

  // Prepare dynamic components
  for (int ch = 0; ch < 2; ++ch) {
    allpass[ch].prepare(sampleRate * 4.0); // Oversampled rate
    waveshaper[ch].setDrive(driveParam);
  }

  updateFilters();
  reset();
}

void NeveTransformerDSP::reset() {
  for (int ch = 0; ch < 2; ++ch) {
    lfPoleFilter[ch].reset();
    hfResonanceFilter[ch].reset();
    ironFilter[ch].reset();
    postShelfFilter[ch].reset();
    allpass[ch].reset();
  }
  oversampler.reset();
}

void NeveTransformerDSP::updateFilters() {
  // LF pole: 60 Hz for Line, 50 Hz for Mic (harder pole)
  double lfCutoff = micMode ? 50.0 : 60.0;
  double lfQ = 0.7;

  // HF resonance: 14 kHz peak, +1.5 dB
  double hfPeakFreq = 14000.0;
  double hfPeakGain = 1.5;
  double hfPeakQ = 1.2;

  // HF Roll: 20-30 kHz based on parameter
  double hfRollFreq = 20000.0 + hfRollParam * 10000.0;

  // Iron control: 100 Hz shelf, 0-2 dB boost
  double ironGain = ironParam * 2.0;

  // Post-transformer: subtle 80 Hz boost for "thick" character
  double postThickGain = 0.2;

  // Z Load: adjusts HF resonance Q (Hi-Z = sharper peak)
  if (!highZLoad)
    hfPeakQ = 0.8; // Lower Q for Lo-Z

  for (int ch = 0; ch < 2; ++ch) {
    lfPoleFilter[ch] = BiquadFilter::makeLowpass(sampleRate, lfCutoff, lfQ);
    hfResonanceFilter[ch] =
        BiquadFilter::makePeak(sampleRate, hfPeakFreq, hfPeakGain, hfPeakQ);
    ironFilter[ch] =
        BiquadFilter::makeLowShelf(sampleRate, 100.0, ironGain, 0.707);
    postShelfFilter[ch] =
        BiquadFilter::makeLowShelf(sampleRate, 80.0, postThickGain, 0.707);
  }
}

void NeveTransformerDSP::processBlock(juce::AudioBuffer<float> &buffer) {
  if (bypassed)
    return;

  const int numSamples = buffer.getNumSamples();
  const int inputChannels = buffer.getNumChannels(); // Actual input channels

  // Always process in stereo (2 channels) internally
  // The DSP architecture (filters, waveshapers, oversampler) is fixed to 2
  // channels.
  if (doubleBuffer.getNumSamples() < numSamples)
    doubleBuffer.setSize(2, numSamples, false, true, true);

  doubleBuffer.clear();

  // Convert to double and apply linear pre-filters
  // If input is mono, we verify the first channel and duplicate logical
  // processing or silence
  for (int ch = 0; ch < 2; ++ch) {
    // Map input: If input is mono (1 ch), use ch 0 for both internal channels
    // If input is stereo, map 1:1. If input has >2 channels, ignore extra.
    int sourceCh = (ch < inputChannels) ? ch : 0;

    // Safety check just in case buffer is somehow 0 channels (shouldn't happen)
    if (sourceCh >= inputChannels) {
      doubleBuffer.clear(ch, 0, numSamples);
      continue;
    }

    auto *input = buffer.getReadPointer(sourceCh);
    auto *output = doubleBuffer.getWritePointer(ch);

    for (int i = 0; i < numSamples; ++i) {
      double sample = static_cast<double>(input[i]);

      // Pre-transformer linear chain
      sample = ironFilter[ch].process(sample);        // Iron boost
      sample = lfPoleFilter[ch].process(sample);      // LF pole
      sample = hfResonanceFilter[ch].process(sample); // HF resonance

      output[i] = sample;
    }
  }

  // Oversample for nonlinear processing
  // Ensure oversampler is prepared for current block size
  if (numSamples > oversampler.getPreparedBlockSize())
    oversampler.prepare(sampleRate, numSamples);

  juce::dsp::AudioBlock<double> block(doubleBuffer);
  juce::dsp::AudioBlock<double> oversampledBlock = oversampler.upsample(block);

  const int oversampledSamples =
      static_cast<int>(oversampledBlock.getNumSamples());

  // Nonlinear core at 4x sample rate
  for (int ch = 0; ch < 2; ++ch) {
    auto *samples = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));

    if (samples == nullptr)
      continue;

    for (int i = 0; i < oversampledSamples; ++i) {
      double sample = samples[i];

      // Waveshaper with hysteresis
      double coreState = allpass[ch].getCoreState();
      sample = waveshaper[ch].processWithHysteresis(sample, coreState);

      // Dynamic allpass (AM/PM)
      sample = allpass[ch].process(sample, driveParam);

      samples[i] = sample;
    }
  }

  // Downsample back to original rate
  oversampler.downsample(oversampledBlock);

  // Post-transformer filter and convert back to float
  // Map internal stereo back to available output channels
  for (int ch = 0; ch < inputChannels && ch < 2; ++ch) {
    auto *input = doubleBuffer.getReadPointer(ch);
    auto *output = buffer.getWritePointer(ch);

    for (int i = 0; i < numSamples; ++i) {
      double sample = input[i];
      sample = postShelfFilter[ch].process(sample); // Post-EQ tilt
      output[i] = static_cast<float>(sample);
    }
  }
}

int NeveTransformerDSP::getLatencySamples() const {
  return oversampler.getLatencySamples();
}

// Parameter setters
void NeveTransformerDSP::setDrive(double value) {
  driveParam = juce::jlimit(0.0, 1.0, value);
  for (int ch = 0; ch < 2; ++ch)
    waveshaper[ch].setDrive(driveParam);
}

void NeveTransformerDSP::setIron(double value) {
  ironParam = juce::jlimit(0.0, 1.0, value);
  updateFilters();
}

void NeveTransformerDSP::setHFRoll(double value) {
  hfRollParam = juce::jlimit(0.0, 1.0, value);
  updateFilters();
}

void NeveTransformerDSP::setMode(bool isMic) {
  micMode = isMic;
  updateFilters();
}

void NeveTransformerDSP::setZLoad(bool isHigh) {
  highZLoad = isHigh;
  updateFilters();
}

void NeveTransformerDSP::setBypassed(bool shouldBypass) {
  bypassed = shouldBypass;
  if (bypassed)
    reset();
}
