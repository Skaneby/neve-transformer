#pragma once

#include "../DSP/NeveTransformerDSP.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Main UI component for Neve Transformer standalone app
 */
class MainComponent : public juce::AudioAppComponent {
public:
  MainComponent();
  ~MainComponent() override;

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;
  void releaseResources() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  // DSP processor
  NeveTransformerDSP dsp;

  // UI Components
  juce::Slider driveSlider;
  juce::Label driveLabel;

  juce::Slider ironSlider;
  juce::Label ironLabel;

  juce::Slider hfRollSlider;
  juce::Label hfRollLabel;

  juce::ToggleButton modeButton;
  juce::ToggleButton zLoadButton;
  juce::ToggleButton bypassButton;

  juce::Label titleLabel;
  juce::Label latencyLabel;

  // Level meters (simple peak detection)
  float inputLevel[2] = {0.0f, 0.0f};
  float outputLevel[2] = {0.0f, 0.0f};

  // Pre-allocated buffer for audio thread
  juce::AudioBuffer<float> tempBuffer;

  void updateLatencyDisplay();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
