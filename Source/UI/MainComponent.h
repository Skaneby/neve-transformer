#pragma once

#include "../DSP/NeveTransformerDSP.h"
#include "NeveLookAndFeel.h"
#include "PresetManager.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

class MainComponent : public juce::AudioAppComponent, public juce::Timer {
public:
  MainComponent();
  ~MainComponent() override;

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;
  void releaseResources() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;
  void mouseDown(const juce::MouseEvent &e) override;

private:
  // DSP processor
  NeveTransformerDSP dsp;

  // Custom styling
  NeveLookAndFeel neveLookAndFeel;

  // Preset management
  PresetManager presetManager;

  // UI Components — Knobs
  juce::Slider driveSlider;
  juce::Label driveLabel;
  juce::Slider ironSlider;
  juce::Label ironLabel;
  juce::Slider hfRollSlider;
  juce::Label hfRollLabel;

  // Wet/Dry mix
  juce::Slider mixSlider;
  juce::Label mixLabel;
  std::atomic<float> mixValue { 1.0f };

  // Toggle buttons
  juce::ToggleButton modeButton;
  juce::ToggleButton zLoadButton;
  juce::ToggleButton bypassButton;

  // A/B comparison
  juce::TextButton abButton;
  Preset snapshotA, snapshotB;
  bool isSnapshotA = true;

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

  // File / Transport UI
  juce::TextButton selectInputButton;
  juce::TextButton exportButton;
  juce::Label fileProcessingLabel;
  juce::Label fileNameLabel;
  juce::Label outputLocationLabel;
  juce::ProgressBar progressBar;

  // Transport controls
  juce::TextButton playButton;
  juce::TextButton stopButton;
  juce::ToggleButton loopToggle;

  // Waveform display area
  juce::Rectangle<int> waveformArea;

  // Playback state
  enum class PlaybackState { IDLE, PLAYING, STOPPED };
  PlaybackState playbackState = PlaybackState::IDLE;

  // File playback source chain
  std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
  juce::AudioTransportSource transportSource;
  juce::AudioThumbnailCache thumbnailCache { 5 };
  juce::AudioThumbnail thumbnail;

  // File members
  juce::File inputFile;
  std::unique_ptr<juce::FileChooser> fileChooser;
  juce::AudioFormatManager formatManager;
  double progress = 0.0;

  // Level meters
  std::atomic<float> inputLevel[2];
  std::atomic<float> outputLevel[2];

  // Pre-allocated buffers for audio thread
  juce::AudioBuffer<float> tempBuffer;
  juce::AudioBuffer<float> dryBuffer;

  // Fixed output directory
  static juce::File getOutputDirectory();
  juce::File getOutputFile(const juce::String &originalName, const juce::String &extension);

  void updateLatencyDisplay();
  void loadPreset(int index);
  void saveCurrentPreset();
  void updatePresetSelector();
  void updateAudioDeviceSelectors();
  void updateStatusLog();
  void selectFileInput();
  void loadFileForPreview(const juce::File &file);
  void startPlayback();
  void stopPlayback();
  void exportProcessedFile();
  void captureSnapshot(bool isA);
  void loadSnapshot(bool isA);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
