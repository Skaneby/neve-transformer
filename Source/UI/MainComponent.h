#pragma once

#include "../DSP/NeveTransformerDSP.h"
#include "NeveLookAndFeel.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Floating help overlay that appears near a "?" button
class HelpBubble : public juce::Component {
public:
  HelpBubble(const juce::String &helpText, juce::Component *parent)
      : text(helpText), owner(parent) {
    setAlwaysOnTop(true);
    setSize(260, 10);
  }

  void paint(juce::Graphics &g) override {
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xf0222222));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xffcc4444));
    g.drawRoundedRectangle(bounds, 6.0f, 1.5f);
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(12.0f));
    g.drawFittedText(text, getLocalBounds().reduced(10, 8),
                     juce::Justification::topLeft, 20);
  }

  void mouseDown(const juce::MouseEvent &) override { dismiss(); }

  void dismiss() {
    if (owner != nullptr)
      owner->removeChildComponent(this);
  }

  void showAt(juce::Component *anchor) {
    if (owner == nullptr || anchor == nullptr) return;
    juce::Font font(juce::FontOptions(12.0f));
    int textWidth = 240;
    auto layout = juce::AttributedString(text);
    layout.setFont(font);
    juce::TextLayout tl;
    tl.createLayout(layout, (float)textWidth);
    int neededHeight = (int)tl.getHeight() + 20;
    setSize(260, juce::jmax(40, neededHeight));

    auto anchorBounds = owner->getLocalArea(anchor, anchor->getLocalBounds());
    int x = anchorBounds.getCentreX() - getWidth() / 2;
    int y = anchorBounds.getBottom() + 4;
    if (x + getWidth() > owner->getWidth() - 10) x = owner->getWidth() - getWidth() - 10;
    if (x < 10) x = 10;
    if (y + getHeight() > owner->getHeight() - 10) y = anchorBounds.getY() - getHeight() - 4;
    setTopLeftPosition(x, y);
    owner->addAndMakeVisible(this);
  }

private:
  juce::String text;
  juce::Component *owner;
};

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
  NeveTransformerDSP dsp;
  NeveLookAndFeel neveLookAndFeel;

  // Wet/dry mix
  juce::Slider mixSlider;
  juce::Label mixLabel;
  std::atomic<float> mixValue { 1.0f };

  // Controls
  juce::ToggleButton bypassButton;
  juce::TextButton infoButton;

  juce::Label titleLabel;
  juce::Label latencyLabel;

  // Logo
  juce::Image logoImage;

  // Audio device selection
  juce::ComboBox inputDeviceSelector;
  juce::ComboBox outputDeviceSelector;
  juce::Label inputDeviceLabel;
  juce::Label outputDeviceLabel;

  // Status log
  juce::TextEditor statusLog;
  juce::Label statusLogLabel;

  // File / Transport
  juce::TextButton selectInputButton;
  juce::TextButton exportButton;
  juce::Label fileProcessingLabel;
  juce::Label fileNameLabel;
  juce::Label outputLocationLabel;
  juce::ProgressBar progressBar;
  juce::TextButton playButton;
  juce::TextButton stopButton;
  juce::ToggleButton loopToggle;
  juce::Rectangle<int> waveformArea;

  enum class PlaybackState { IDLE, PLAYING, STOPPED };
  PlaybackState playbackState = PlaybackState::IDLE;

  std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
  juce::AudioTransportSource transportSource;
  juce::AudioThumbnailCache thumbnailCache { 5 };
  juce::AudioThumbnail thumbnail;
  juce::File inputFile;
  std::unique_ptr<juce::FileChooser> fileChooser;
  juce::AudioFormatManager formatManager;
  double progress = 0.0;

  // Meters
  std::atomic<float> inputLevel[2];
  std::atomic<float> outputLevel[2];
  std::atomic<float> cpuLoad { 0.0f };

  // Buffers
  juce::AudioBuffer<float> tempBuffer;
  juce::AudioBuffer<float> dryBuffer;

  // Help/info popup
  std::unique_ptr<HelpBubble> activeBubble;
  void showHelp(juce::Component *anchor, const juce::String &text);

  static juce::File getOutputDirectory();
  juce::File getOutputFile(const juce::String &originalName, const juce::String &extension);

  void updateLatencyDisplay();
  void updateAudioDeviceSelectors();
  void updateStatusLog();
  void selectFileInput();
  void loadFileForPreview(const juce::File &file);
  void startPlayback();
  void stopPlayback();
  void exportProcessedFile();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
