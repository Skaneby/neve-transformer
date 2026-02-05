#pragma once

#include "../DSP/NeveTransformerDSP.h"
#include "NeveLookAndFeel.h"
#include "PresetManager.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Main UI component for Herrstrom Phase Shifter (Neve Transformer) standalone app
 */
class MainComponent : public juce::AudioAppComponent, public juce::Timer {
public:
  MainComponent();
  ~MainComponent() override;

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;
  void releaseResources() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

private:
  // DSP processor
  NeveTransformerDSP dsp;

  // Custom styling
  NeveLookAndFeel neveLookAndFeel;

  // Preset management
  PresetManager presetManager;

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

  // Preset controls
  juce::ComboBox presetSelector;
  juce::TextButton savePresetButton;
  juce::TextButton openPresetsButton;
  juce::Label presetLabel;

  // Logo image
  juce::Image logoImage;

  // Audio device selection
  juce::ComboBox inputDeviceSelector;
  juce::ComboBox outputDeviceSelector;
  juce::Label inputDeviceLabel;
  juce::Label outputDeviceLabel;

  // Status log
  juce::TextEditor statusLog;
  juce::Label statusLogLabel;

  // File Processing UI
  juce::TextButton selectInputButton;
  juce::TextButton selectOutputButton;
  juce::TextButton processButton;
  juce::Label inputPathLabel;
  juce::Label outputPathLabel;
  juce::Label fileProcessingLabel;
  juce::ProgressBar progressBar;

  // File Processing Members
  juce::File inputFile;
  juce::File outputFile;
  std::unique_ptr<juce::FileChooser> fileChooser;
  juce::AudioFormatManager formatManager;
  double progress = 0.0;

  // Level meters (simple peak detection)
  std::atomic<float> inputLevel[2];
  std::atomic<float> outputLevel[2];

  // Pre-allocated buffer for audio thread
  juce::AudioBuffer<float> tempBuffer;

  void updateLatencyDisplay();
  void loadPreset(int index);
  void saveCurrentPreset();
  void updatePresetSelector();
  void updateAudioDeviceSelectors();
  void updateStatusLog();
  void selectFileInput();
  void selectFileOutput();
  void processAudioFile();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
