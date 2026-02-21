#include "MainComponent.h"

MainComponent::MainComponent()
    : progressBar(progress),
      thumbnail(512, formatManager, thumbnailCache) {
  setSize(1000, 750);
  setOpaque(true);
  setLookAndFeel(&neveLookAndFeel);

  // Load logo image
  auto logoFile = juce::File(__FILE__).getParentDirectory().getChildFile("logo.png");
  if (logoFile.existsAsFile())
    logoImage = juce::ImageCache::getFromFile(logoFile);

  // Title/Logo (fallback text if image doesn't load)
  addAndMakeVisible(titleLabel);
  titleLabel.setText("HERRSTROM", juce::dontSendNotification);
  titleLabel.setFont(juce::FontOptions(36.0f, juce::Font::bold));
  titleLabel.setJustificationType(juce::Justification::centred);
  titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));

  // Preset section
  addAndMakeVisible(presetLabel);
  presetLabel.setText("PRESET:", juce::dontSendNotification);
  presetLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
  presetLabel.setJustificationType(juce::Justification::centredLeft);
  presetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

  addAndMakeVisible(presetSelector);
  presetSelector.setLookAndFeel(&neveLookAndFeel);
  updatePresetSelector();
  presetSelector.onChange = [this]() {
    loadPreset(presetSelector.getSelectedItemIndex());
  };

  addAndMakeVisible(savePresetButton);
  savePresetButton.setButtonText("SAVE PRESET");
  savePresetButton.setLookAndFeel(&neveLookAndFeel);
  savePresetButton.onClick = [this]() { saveCurrentPreset(); };

  addAndMakeVisible(openPresetsButton);
  openPresetsButton.setButtonText("PRESETS FOLDER");
  openPresetsButton.setLookAndFeel(&neveLookAndFeel);
  openPresetsButton.onClick = [this]() {
    presetManager.getPresetsFolder().revealToUser();
  };

  // Drive slider
  addAndMakeVisible(driveSlider);
  driveSlider.setLookAndFeel(&neveLookAndFeel);
  driveSlider.setRange(0.0, 1.0, 0.01);
  driveSlider.setValue(0.3);
  driveSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 28);
  driveSlider.onValueChange = [this]() {
    dsp.setDrive(driveSlider.getValue());
  };

  addAndMakeVisible(driveLabel);
  driveLabel.setText("DRIVE", juce::dontSendNotification);
  driveLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  driveLabel.setJustificationType(juce::Justification::centred);
  driveLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
  driveLabel.attachToComponent(&driveSlider, false);

  // Iron slider
  addAndMakeVisible(ironSlider);
  ironSlider.setLookAndFeel(&neveLookAndFeel);
  ironSlider.setRange(0.0, 1.0, 0.01);
  ironSlider.setValue(0.5);
  ironSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  ironSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 28);
  ironSlider.onValueChange = [this]() { dsp.setIron(ironSlider.getValue()); };

  addAndMakeVisible(ironLabel);
  ironLabel.setText("IRON", juce::dontSendNotification);
  ironLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  ironLabel.setJustificationType(juce::Justification::centred);
  ironLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
  ironLabel.attachToComponent(&ironSlider, false);

  // HF Roll slider
  addAndMakeVisible(hfRollSlider);
  hfRollSlider.setLookAndFeel(&neveLookAndFeel);
  hfRollSlider.setRange(0.0, 1.0, 0.01);
  hfRollSlider.setValue(0.7);
  hfRollSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  hfRollSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 28);
  hfRollSlider.onValueChange = [this]() {
    dsp.setHFRoll(hfRollSlider.getValue());
  };

  addAndMakeVisible(hfRollLabel);
  hfRollLabel.setText("HF ROLL", juce::dontSendNotification);
  hfRollLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  hfRollLabel.setJustificationType(juce::Justification::centred);
  hfRollLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
  hfRollLabel.attachToComponent(&hfRollSlider, false);

  // Mix slider (wet/dry)
  addAndMakeVisible(mixSlider);
  mixSlider.setLookAndFeel(&neveLookAndFeel);
  mixSlider.setRange(0.0, 1.0, 0.01);
  mixSlider.setValue(1.0);
  mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 22);
  mixSlider.onValueChange = [this]() {
    mixValue.store((float)mixSlider.getValue(), std::memory_order_relaxed);
  };

  addAndMakeVisible(mixLabel);
  mixLabel.setText("MIX", juce::dontSendNotification);
  mixLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  mixLabel.setJustificationType(juce::Justification::centred);
  mixLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
  mixLabel.attachToComponent(&mixSlider, false);

  // Mode toggle (Mic/Line)
  addAndMakeVisible(modeButton);
  modeButton.setLookAndFeel(&neveLookAndFeel);
  modeButton.setButtonText("MIC MODE");
  modeButton.setToggleState(false, juce::dontSendNotification);
  modeButton.onClick = [this]() { dsp.setMode(modeButton.getToggleState()); };

  // Z Load toggle (Hi/Lo)
  addAndMakeVisible(zLoadButton);
  zLoadButton.setLookAndFeel(&neveLookAndFeel);
  zLoadButton.setButtonText("HI-Z LOAD");
  zLoadButton.setToggleState(true, juce::dontSendNotification);
  zLoadButton.onClick = [this]() {
    dsp.setZLoad(zLoadButton.getToggleState());
  };

  // Bypass toggle
  addAndMakeVisible(bypassButton);
  bypassButton.setLookAndFeel(&neveLookAndFeel);
  bypassButton.setButtonText("BYPASS");
  bypassButton.setToggleState(false, juce::dontSendNotification);
  bypassButton.onClick = [this]() {
    dsp.setBypassed(bypassButton.getToggleState());
  };

  // A/B comparison button
  addAndMakeVisible(abButton);
  abButton.setButtonText("A");
  abButton.setLookAndFeel(&neveLookAndFeel);
  abButton.onClick = [this]() {
    captureSnapshot(isSnapshotA);
    isSnapshotA = !isSnapshotA;
    loadSnapshot(isSnapshotA);
    abButton.setButtonText(isSnapshotA ? "A" : "B");
  };

  // Latency display
  addAndMakeVisible(latencyLabel);
  latencyLabel.setText("Latency: -- | Phase Shifter", juce::dontSendNotification);
  latencyLabel.setFont(juce::FontOptions(11.0f));
  latencyLabel.setJustificationType(juce::Justification::centredRight);
  latencyLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));

  // Audio device selectors
  addAndMakeVisible(inputDeviceLabel);
  inputDeviceLabel.setText("INPUT:", juce::dontSendNotification);
  inputDeviceLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
  inputDeviceLabel.setJustificationType(juce::Justification::centredLeft);
  inputDeviceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

  addAndMakeVisible(inputDeviceSelector);
  inputDeviceSelector.setLookAndFeel(&neveLookAndFeel);
  inputDeviceSelector.onChange = [this]() {
    auto setup = deviceManager.getAudioDeviceSetup();
    setup.inputDeviceName = inputDeviceSelector.getText();
    deviceManager.setAudioDeviceSetup(setup, true);
    updateStatusLog();
  };

  addAndMakeVisible(outputDeviceLabel);
  outputDeviceLabel.setText("OUTPUT:", juce::dontSendNotification);
  outputDeviceLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
  outputDeviceLabel.setJustificationType(juce::Justification::centredLeft);
  outputDeviceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

  addAndMakeVisible(outputDeviceSelector);
  outputDeviceSelector.setLookAndFeel(&neveLookAndFeel);
  outputDeviceSelector.onChange = [this]() {
    auto setup = deviceManager.getAudioDeviceSetup();
    setup.outputDeviceName = outputDeviceSelector.getText();
    deviceManager.setAudioDeviceSetup(setup, true);
    updateStatusLog();
  };

  // Status log
  addAndMakeVisible(statusLogLabel);
  statusLogLabel.setText("AUDIO STATUS:", juce::dontSendNotification);
  statusLogLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
  statusLogLabel.setJustificationType(juce::Justification::centredLeft);
  statusLogLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

  addAndMakeVisible(statusLog);
  statusLog.setMultiLine(true);
  statusLog.setReadOnly(true);
  statusLog.setScrollbarsShown(true);
  statusLog.setCaretVisible(false);
  statusLog.setPopupMenuEnabled(false);
  statusLog.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1a1a1a));
  statusLog.setColour(juce::TextEditor::textColourId, juce::Colour(0xff88cc88));
  statusLog.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3a3a3a));
  statusLog.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
  statusLog.setText("Initializing audio...\n", false);

  // File / Transport Section
  addAndMakeVisible(fileProcessingLabel);
  fileProcessingLabel.setText("AUDIO FILE:", juce::dontSendNotification);
  fileProcessingLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
  fileProcessingLabel.setJustificationType(juce::Justification::centredLeft);
  fileProcessingLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

  addAndMakeVisible(selectInputButton);
  selectInputButton.setButtonText("LOAD FILE...");
  selectInputButton.setLookAndFeel(&neveLookAndFeel);
  selectInputButton.onClick = [this] { selectFileInput(); };

  addAndMakeVisible(fileNameLabel);
  fileNameLabel.setText("No file loaded", juce::dontSendNotification);
  fileNameLabel.setFont(juce::FontOptions(10.0f));
  fileNameLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

  // Transport controls
  addAndMakeVisible(playButton);
  playButton.setButtonText("PLAY");
  playButton.setLookAndFeel(&neveLookAndFeel);
  playButton.setEnabled(false);
  playButton.onClick = [this]() {
    if (playbackState == PlaybackState::PLAYING) {
      transportSource.stop();
      playbackState = PlaybackState::STOPPED;
      playButton.setButtonText("PLAY");
    } else {
      startPlayback();
    }
  };

  addAndMakeVisible(stopButton);
  stopButton.setButtonText("STOP");
  stopButton.setLookAndFeel(&neveLookAndFeel);
  stopButton.setEnabled(false);
  stopButton.onClick = [this]() { stopPlayback(); };

  addAndMakeVisible(loopToggle);
  loopToggle.setButtonText("LOOP");
  loopToggle.setLookAndFeel(&neveLookAndFeel);
  loopToggle.setToggleState(true, juce::dontSendNotification);
  loopToggle.onClick = [this]() {
    if (readerSource != nullptr)
      readerSource->setLooping(loopToggle.getToggleState());
  };

  // Export button
  addAndMakeVisible(exportButton);
  exportButton.setButtonText("EXPORT");
  exportButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff882222));
  exportButton.setLookAndFeel(&neveLookAndFeel);
  exportButton.setEnabled(false);
  exportButton.onClick = [this] { exportProcessedFile(); };

  // Output location display
  addAndMakeVisible(outputLocationLabel);
  outputLocationLabel.setText("Output: herrstrom/", juce::dontSendNotification);
  outputLocationLabel.setFont(juce::FontOptions(10.0f));
  outputLocationLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

  addAndMakeVisible(progressBar);
  progressBar.setTextToDisplay("Ready");

  formatManager.registerBasicFormats();

  // Request audio permissions and setup
  setAudioChannels(2, 2);

  // Update device lists and status
  updateAudioDeviceSelectors();
  updateStatusLog();

  // Initialize A/B snapshots with defaults
  captureSnapshot(true);
  captureSnapshot(false);

  // Start UI timer (30 Hz)
  startTimerHz(30);
}

MainComponent::~MainComponent() {
  stopPlayback();
  transportSource.setSource(nullptr);
  readerSource.reset();
  setLookAndFeel(nullptr);
  shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  const int safeBlockSize = juce::jmax(samplesPerBlockExpected, 8192);

  dsp.prepare(sampleRate, safeBlockSize);

  tempBuffer.setSize(2, safeBlockSize);
  dryBuffer.setSize(2, safeBlockSize);

  transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);

  dsp.setDrive(driveSlider.getValue());
  dsp.setIron(ironSlider.getValue());
  dsp.setHFRoll(hfRollSlider.getValue());
  dsp.setMode(modeButton.getToggleState());
  dsp.setZLoad(zLoadButton.getToggleState());

  updateLatencyDisplay();
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  auto *buffer = bufferToFill.buffer;
  const int numSamples = bufferToFill.numSamples;
  const int numChannels = buffer->getNumChannels();

  jassert(tempBuffer.getNumSamples() >= numSamples);
  tempBuffer.clear();

  if (playbackState == PlaybackState::PLAYING && readerSource != nullptr) {
    // --- FILE PLAYBACK PATH ---
    transportSource.getNextAudioBlock(bufferToFill);

    for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
      auto *channelData = buffer->getReadPointer(ch, bufferToFill.startSample);

      float peak = 0.0f;
      for (int i = 0; i < numSamples; ++i)
        peak = juce::jmax(peak, std::abs(channelData[i]));
      inputLevel[ch] = peak;

      tempBuffer.copyFrom(ch, 0, channelData, numSamples);
    }
  } else {
    // --- MIC INPUT PATH ---
    for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
      auto *channelData = buffer->getReadPointer(ch, bufferToFill.startSample);

      float peak = 0.0f;
      for (int i = 0; i < numSamples; ++i)
        peak = juce::jmax(peak, std::abs(channelData[i]));
      inputLevel[ch] = peak;

      tempBuffer.copyFrom(ch, 0, channelData, numSamples);
    }
  }

  if (numChannels == 1)
    inputLevel[1] = 0.0f;

  // Store dry copy for wet/dry mix
  for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
    dryBuffer.copyFrom(ch, 0, tempBuffer, ch, 0, numSamples);

  // Process through DSP
  dsp.processBlock(tempBuffer);

  // Apply wet/dry mix
  float mix = mixValue.load(std::memory_order_relaxed);
  if (mix < 1.0f) {
    float dryGain = 1.0f - mix;
    for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
      auto *wet = tempBuffer.getWritePointer(ch);
      auto *dry = dryBuffer.getReadPointer(ch);
      for (int i = 0; i < numSamples; ++i)
        wet[i] = wet[i] * mix + dry[i] * dryGain;
    }
  }

  // Copy back to output
  for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
    buffer->copyFrom(ch, bufferToFill.startSample, tempBuffer, ch, 0, numSamples);

    auto *channelData = buffer->getReadPointer(ch, bufferToFill.startSample);
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
      peak = juce::jmax(peak, std::abs(channelData[i]));
    outputLevel[ch] = peak;
  }

  if (numChannels == 1)
    outputLevel[1] = 0.0f;
}

void MainComponent::releaseResources() {
  transportSource.releaseResources();
  dsp.reset();
}

void MainComponent::paint(juce::Graphics &g) {
  // Dark background
  g.fillAll(juce::Colour(0xff1a1a1a));

  // Draw logo if loaded
  if (logoImage.isValid()) {
    auto logoArea = juce::Rectangle<float>(30, 10, (float)(getWidth() - 60), 100);
    g.drawImage(logoImage, logoArea,
                juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
  }

  // Main panel background with gradient
  auto panelBounds = juce::Rectangle<float>(30, 120, (float)(getWidth() - 60), (float)(getHeight() - 150));

  juce::ColourGradient panelGradient(
      juce::Colour(0xff323232), panelBounds.getCentreX(), panelBounds.getY(),
      juce::Colour(0xff282828), panelBounds.getCentreX(), panelBounds.getBottom(), false);
  g.setGradientFill(panelGradient);
  g.fillRoundedRectangle(panelBounds, 12.0f);

  g.setColour(juce::Colour(0xff0f0f0f).withAlpha(0.6f));
  g.drawRoundedRectangle(panelBounds.reduced(2), 10.0f, 4.0f);

  g.setColour(juce::Colour(0xff4a4a4a).withAlpha(0.3f));
  g.drawRoundedRectangle(panelBounds.expanded(1), 12.0f, 1.5f);

  // Level meters (in the reserved 70px strip on the far right)
  const int meterX = getWidth() - 55;
  const int meterY = 140;
  const int meterHeight = 200;
  const int meterWidth = 14;

  for (int ch = 0; ch < 2; ++ch) {
    int x = meterX + ch * 25;

    g.setColour(juce::Colour(0xff444444));
    g.fillRect(x, meterY, meterWidth, meterHeight);

    g.setColour(juce::Colours::green);
    int inputH = static_cast<int>(inputLevel[ch].load() * meterHeight);
    g.fillRect(x, meterY + meterHeight - inputH, meterWidth / 2, inputH);

    g.setColour(juce::Colours::yellow);
    int outputH = static_cast<int>(outputLevel[ch].load() * meterHeight);
    g.fillRect(x + meterWidth / 2, meterY + meterHeight - outputH,
               meterWidth / 2, outputH);
  }

  g.setColour(juce::Colours::lightgrey);
  g.setFont(10.0f);
  g.drawText("L", meterX, meterY + meterHeight + 5, 15, 15, juce::Justification::centred);
  g.drawText("R", meterX + 25, meterY + meterHeight + 5, 15, 15, juce::Justification::centred);

  // Waveform display
  if (!waveformArea.isEmpty()) {
    g.setColour(juce::Colour(0xff222222));
    g.fillRect(waveformArea);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(waveformArea);

    if (thumbnail.getTotalLength() > 0.0) {
      g.setColour(juce::Colour(0xff44aa44));
      thumbnail.drawChannels(g, waveformArea.reduced(2),
                             0.0, thumbnail.getTotalLength(), 1.0f);

      // Draw playback position
      if (transportSource.getLengthInSeconds() > 0.0) {
        double posRatio = transportSource.getCurrentPosition()
                        / transportSource.getLengthInSeconds();
        int xPos = waveformArea.getX() + 2
                 + (int)(posRatio * (waveformArea.getWidth() - 4));
        g.setColour(juce::Colours::white);
        g.drawLine((float)xPos, (float)waveformArea.getY(),
                   (float)xPos, (float)waveformArea.getBottom(), 2.0f);
      }
    } else {
      g.setColour(juce::Colours::grey);
      g.drawText("No file loaded", waveformArea, juce::Justification::centred);
    }
  }
}

void MainComponent::resized() {
  auto area = getLocalBounds();

  titleLabel.setVisible(!logoImage.isValid());

  // Logo area at top
  area.removeFromTop(110);

  // Preset row
  auto presetArea = area.removeFromTop(45).reduced(40, 8);
  presetLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  presetLabel.setBounds(presetArea.removeFromLeft(80));
  presetSelector.setBounds(presetArea.removeFromLeft(280));
  presetArea.removeFromLeft(15);
  savePresetButton.setBounds(presetArea.removeFromLeft(130));
  presetArea.removeFromLeft(10);
  openPresetsButton.setBounds(presetArea.removeFromLeft(130));

  // Main content area with margins
  auto mainArea = area.reduced(40, 15);

  // Reserve space for meters on the far right (painted in paint())
  mainArea.removeFromRight(70);

  // Split into left (controls) and right (device selection + status + transport)
  auto rightPanel = mainArea.removeFromRight(290);
  auto leftPanel = mainArea;

  // === RIGHT PANEL ===

  // Input device selector
  auto inputDevArea = rightPanel.removeFromTop(28);
  inputDeviceLabel.setBounds(inputDevArea.removeFromLeft(65));
  inputDeviceSelector.setBounds(inputDevArea);
  rightPanel.removeFromTop(6);

  // Output device selector
  auto outputDevArea = rightPanel.removeFromTop(28);
  outputDeviceLabel.setBounds(outputDevArea.removeFromLeft(65));
  outputDeviceSelector.setBounds(outputDevArea);
  rightPanel.removeFromTop(8);

  // Status log
  statusLogLabel.setBounds(rightPanel.removeFromTop(18));
  rightPanel.removeFromTop(3);
  statusLog.setBounds(rightPanel.removeFromTop(110));
  rightPanel.removeFromTop(8);

  // Audio file section
  fileProcessingLabel.setBounds(rightPanel.removeFromTop(18));
  rightPanel.removeFromTop(3);
  selectInputButton.setBounds(rightPanel.removeFromTop(26));
  fileNameLabel.setBounds(rightPanel.removeFromTop(16));
  rightPanel.removeFromTop(4);

  // Waveform display
  waveformArea = rightPanel.removeFromTop(70);
  rightPanel.removeFromTop(4);

  // Transport controls row
  auto transportRow = rightPanel.removeFromTop(30);
  playButton.setBounds(transportRow.removeFromLeft(65));
  transportRow.removeFromLeft(5);
  stopButton.setBounds(transportRow.removeFromLeft(65));
  transportRow.removeFromLeft(5);
  loopToggle.setBounds(transportRow.removeFromLeft(65));
  rightPanel.removeFromTop(6);

  // Export button + output location + progress
  exportButton.setBounds(rightPanel.removeFromTop(32));
  rightPanel.removeFromTop(3);
  outputLocationLabel.setBounds(rightPanel.removeFromTop(14));
  rightPanel.removeFromTop(4);
  progressBar.setBounds(rightPanel.removeFromTop(18));

  // === LEFT PANEL: Knobs and buttons ===
  auto controlArea = leftPanel.withTrimmedRight(10);

  // Scale knobs to fit available width
  const int availableWidth = controlArea.getWidth();
  const int knobSpacing = 20;
  const int knobSize = juce::jmin(180, (availableWidth - knobSpacing * 2) / 3);

  const int totalKnobWidth = (knobSize * 3) + (knobSpacing * 2);
  auto knobArea = controlArea.removeFromTop(knobSize + 50);
  auto centeredKnobArea = knobArea.withSizeKeepingCentre(totalKnobWidth, knobSize + 50);

  driveSlider.setBounds(centeredKnobArea.removeFromLeft(knobSize).withTrimmedTop(25));
  centeredKnobArea.removeFromLeft(knobSpacing);
  ironSlider.setBounds(centeredKnobArea.removeFromLeft(knobSize).withTrimmedTop(25));
  centeredKnobArea.removeFromLeft(knobSpacing);
  hfRollSlider.setBounds(centeredKnobArea.removeFromLeft(knobSize).withTrimmedTop(25));

  // Buttons row
  auto buttonArea = controlArea.removeFromTop(45).reduced(0, 5);
  const int buttonWidth = 110;
  const int btnSpacing = 10;

  // Center the button row
  const int totalButtonWidth = buttonWidth * 3 + btnSpacing * 3 + 50;
  auto centeredBtnArea = buttonArea.withSizeKeepingCentre(totalButtonWidth, buttonArea.getHeight());

  modeButton.setBounds(centeredBtnArea.removeFromLeft(buttonWidth));
  centeredBtnArea.removeFromLeft(btnSpacing);
  zLoadButton.setBounds(centeredBtnArea.removeFromLeft(buttonWidth));
  centeredBtnArea.removeFromLeft(btnSpacing);
  bypassButton.setBounds(centeredBtnArea.removeFromLeft(buttonWidth));
  centeredBtnArea.removeFromLeft(btnSpacing);
  abButton.setBounds(centeredBtnArea.removeFromLeft(50));

  // Mix knob below buttons
  controlArea.removeFromTop(5);
  auto mixArea = controlArea.removeFromTop(110);
  mixSlider.setBounds(mixArea.withSizeKeepingCentre(90, 90).withTrimmedTop(18));

  // Latency label at bottom
  latencyLabel.setBounds(controlArea.removeFromBottom(22));
}

void MainComponent::timerCallback() {
  // Only repaint the meter area and waveform area
  const int meterX = getWidth() - 60;
  repaint(meterX, 130, 60, 240);

  if (!waveformArea.isEmpty())
    repaint(waveformArea);
}

void MainComponent::mouseDown(const juce::MouseEvent &e) {
  if (waveformArea.contains(e.getPosition()) && transportSource.getLengthInSeconds() > 0.0) {
    double clickRatio = (double)(e.x - waveformArea.getX()) / waveformArea.getWidth();
    clickRatio = juce::jlimit(0.0, 1.0, clickRatio);
    transportSource.setPosition(clickRatio * transportSource.getLengthInSeconds());
  }
}

void MainComponent::updateLatencyDisplay() {
  int latencySamples = dsp.getLatencySamples();
  latencyLabel.setText("Latency: " + juce::String(latencySamples) + " | Phase Shifter",
                       juce::dontSendNotification);
}

void MainComponent::updatePresetSelector() {
  presetSelector.clear();
  auto names = presetManager.getPresetNames();
  for (int i = 0; i < names.size(); ++i) {
    presetSelector.addItem(names[i], i + 1);
  }
  if (presetSelector.getNumItems() > 0)
    presetSelector.setSelectedItemIndex(0, juce::dontSendNotification);
}

void MainComponent::loadPreset(int index) {
  auto itemText = presetSelector.getItemText(index);
  if (itemText.startsWith("---"))
    return;

  auto preset = presetManager.getPreset(index);
  if (preset.name.isEmpty())
    return;

  driveSlider.setValue(preset.drive, juce::dontSendNotification);
  ironSlider.setValue(preset.iron, juce::dontSendNotification);
  hfRollSlider.setValue(preset.hfRoll, juce::dontSendNotification);
  mixSlider.setValue(preset.mix, juce::dontSendNotification);
  modeButton.setToggleState(preset.micMode, juce::dontSendNotification);
  zLoadButton.setToggleState(preset.hiZLoad, juce::dontSendNotification);

  dsp.setDrive(preset.drive);
  dsp.setIron(preset.iron);
  dsp.setHFRoll(preset.hfRoll);
  dsp.setMode(preset.micMode);
  dsp.setZLoad(preset.hiZLoad);
  mixValue.store(preset.mix, std::memory_order_relaxed);
}

void MainComponent::saveCurrentPreset() {
  auto alertWindow = std::make_shared<juce::AlertWindow>(
      "Save Preset", "Enter a name for this preset:",
      juce::MessageBoxIconType::QuestionIcon);

  alertWindow->addTextEditor("name", "My Preset");
  alertWindow->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
  alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

  alertWindow->enterModalState(true,
      juce::ModalCallbackFunction::create([this, alertWindow](int result) {
    if (result == 1) {
      auto name = alertWindow->getTextEditorContents("name");
      if (name.isNotEmpty()) {
        Preset preset;
        preset.name = name;
        preset.drive = static_cast<float>(driveSlider.getValue());
        preset.iron = static_cast<float>(ironSlider.getValue());
        preset.hfRoll = static_cast<float>(hfRollSlider.getValue());
        preset.mix = static_cast<float>(mixSlider.getValue());
        preset.micMode = modeButton.getToggleState();
        preset.hiZLoad = zLoadButton.getToggleState();

        presetManager.addPreset(preset);
        updatePresetSelector();
        presetSelector.setSelectedItemIndex(presetSelector.getNumItems() - 1);
      }
    }
  }));
}

void MainComponent::updateAudioDeviceSelectors() {
  inputDeviceSelector.clear();
  outputDeviceSelector.clear();

  auto *device = deviceManager.getCurrentAudioDevice();
  if (device == nullptr)
    return;

  auto *deviceType = deviceManager.getCurrentDeviceTypeObject();

  auto inputNames = deviceType->getDeviceNames(true);
  for (int i = 0; i < inputNames.size(); ++i) {
    inputDeviceSelector.addItem(inputNames[i], i + 1);
    if (inputNames[i] == device->getName())
      inputDeviceSelector.setSelectedItemIndex(i, juce::dontSendNotification);
  }

  auto outputNames = deviceType->getDeviceNames(false);
  for (int i = 0; i < outputNames.size(); ++i) {
    outputDeviceSelector.addItem(outputNames[i], i + 1);
    if (outputNames[i] == device->getName())
      outputDeviceSelector.setSelectedItemIndex(i, juce::dontSendNotification);
  }
}

void MainComponent::updateStatusLog() {
  juce::String status;

  auto *device = deviceManager.getCurrentAudioDevice();
  if (device == nullptr) {
    status << "[ERROR] No audio device available\n";
    statusLog.setText(status, false);
    return;
  }

  status << "Device: " << device->getName() << "\n";
  status << "Rate: " << juce::String(device->getCurrentSampleRate(), 0) << " Hz\n";
  status << "Buffer: " << juce::String(device->getCurrentBufferSizeSamples()) << " smp\n";
  status << "Latency: " << juce::String(dsp.getLatencySamples()) << " smp\n\n";

  status << "Drive: " << juce::String(driveSlider.getValue(), 2) << "\n";
  status << "Iron: " << juce::String(ironSlider.getValue(), 2) << "\n";
  status << "HF Roll: " << juce::String(hfRollSlider.getValue(), 2) << "\n";
  status << "Mix: " << juce::String(mixSlider.getValue(), 2) << "\n";
  status << "Mode: " << (modeButton.getToggleState() ? "MIC" : "LINE") << "\n";
  status << "Hi-Z: " << (zLoadButton.getToggleState() ? "ON" : "OFF") << "\n";

  statusLog.setText(status, false);
}

// --- File Loading & Transport ---

void MainComponent::selectFileInput() {
  fileChooser = std::make_unique<juce::FileChooser>(
      "Select an audio file...",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory),
      "*.wav;*.aiff;*.aif");

  auto flags = juce::FileBrowserComponent::openMode
             | juce::FileBrowserComponent::canSelectFiles;

  fileChooser->launchAsync(flags, [this](const juce::FileChooser &fc) {
    auto file = fc.getResult();
    if (file.existsAsFile())
      loadFileForPreview(file);
  });
}

void MainComponent::loadFileForPreview(const juce::File &file) {
  stopPlayback();
  transportSource.setSource(nullptr);
  readerSource.reset();

  auto *reader = formatManager.createReaderFor(file);
  if (reader == nullptr) {
    statusLog.moveCaretToEnd();
    statusLog.insertTextAtCaret("[ERROR] Cannot read: " + file.getFileName() + "\n");
    return;
  }

  readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
  readerSource->setLooping(loopToggle.getToggleState());

  transportSource.setSource(readerSource.get(), 0, nullptr,
                             reader->sampleRate, (int)reader->numChannels);

  thumbnail.setSource(new juce::FileInputSource(file));

  inputFile = file;
  fileNameLabel.setText(file.getFileName(), juce::dontSendNotification);
  fileNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);

  playButton.setEnabled(true);
  stopButton.setEnabled(true);
  exportButton.setEnabled(true);
  playbackState = PlaybackState::STOPPED;

  statusLog.moveCaretToEnd();
  statusLog.insertTextAtCaret("Loaded: " + file.getFileName() + " (" +
      juce::String(reader->sampleRate) + " Hz, " +
      juce::String(reader->numChannels) + " ch)\n");
}

void MainComponent::startPlayback() {
  if (readerSource != nullptr) {
    readerSource->setLooping(loopToggle.getToggleState());
    transportSource.start();
    playbackState = PlaybackState::PLAYING;
    playButton.setButtonText("PAUSE");
  }
}

void MainComponent::stopPlayback() {
  transportSource.stop();
  transportSource.setPosition(0.0);
  playbackState = PlaybackState::STOPPED;
  playButton.setButtonText("PLAY");
}

// --- Fixed Output Location ---

juce::File MainComponent::getOutputDirectory() {
  return juce::File("/Users/macminim1/Library/CloudStorage/"
                    "GoogleDrive-johan.skaneby@gmail.com/My Drive/"
                    "Egna projekt/herrstrom");
}

juce::File MainComponent::getOutputFile(const juce::String &originalName,
                                         const juce::String &extension) {
  auto outputDir = getOutputDirectory();

  if (!outputDir.isDirectory())
    outputDir.createDirectory();

  auto now = juce::Time::getCurrentTime();
  juce::String timestamp = now.formatted("%Y-%m-%d_%H-%M-%S");
  juce::String filename = originalName + "_" + timestamp + extension;

  return outputDir.getChildFile(filename);
}

// --- Export ---

void MainComponent::exportProcessedFile() {
  if (!inputFile.existsAsFile()) return;

  // Determine output format from input extension
  juce::String ext = inputFile.getFileExtension().toLowerCase();
  if (ext != ".aiff" && ext != ".aif")
    ext = ".wav";

  juce::String baseName = inputFile.getFileNameWithoutExtension();
  juce::File outFile = getOutputFile(baseName, ext);

  outputLocationLabel.setText("Export: " + outFile.getFileName(), juce::dontSendNotification);
  outputLocationLabel.setColour(juce::Label::textColourId, juce::Colours::white);

  exportButton.setEnabled(false);
  selectInputButton.setEnabled(false);
  progress = 0.0;
  progressBar.setTextToDisplay("Exporting...");

  statusLog.moveCaretToEnd();
  statusLog.insertTextAtCaret("\n--- Exporting ---\n");
  statusLog.insertTextAtCaret("Input: " + inputFile.getFileName() + "\n");
  statusLog.insertTextAtCaret("Output: " + outFile.getFileName() + "\n");

  // Capture current parameters
  double drive = driveSlider.getValue();
  double iron = ironSlider.getValue();
  double hfRoll = hfRollSlider.getValue();
  bool mode = modeButton.getToggleState();
  bool zLoad = zLoadButton.getToggleState();
  bool bypassed = bypassButton.getToggleState();
  float mix = (float)mixSlider.getValue();

  auto startTime = juce::Time::getMillisecondCounterHiRes();

  juce::Thread::launch([this, drive, iron, hfRoll, mode, zLoad, bypassed,
                        mix, outFile, ext, startTime] {
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputFile));

    if (reader == nullptr) {
      juce::MessageManager::callAsync([this] {
        statusLog.insertTextAtCaret("[ERROR] Could not read input file\n");
        exportButton.setEnabled(true);
        selectInputButton.setEnabled(true);
      });
      return;
    }

    double fileDuration = (double)reader->lengthInSamples / reader->sampleRate;
    juce::MessageManager::callAsync([this, fileDuration] {
      statusLog.insertTextAtCaret("Duration: " + juce::String(fileDuration, 1) + "s\n");
    });

    if (outFile.existsAsFile())
      outFile.deleteFile();

    // Choose format based on extension
    std::unique_ptr<juce::AudioFormat> format;
    if (ext == ".aiff" || ext == ".aif")
      format = std::make_unique<juce::AiffAudioFormat>();
    else
      format = std::make_unique<juce::WavAudioFormat>();

    auto *outStream = outFile.createOutputStream().release();

    if (outStream == nullptr) {
      juce::MessageManager::callAsync([this] {
        statusLog.insertTextAtCaret("[ERROR] Could not create output stream\n");
        exportButton.setEnabled(true);
        selectInputButton.setEnabled(true);
      });
      return;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer(
        format->createWriterFor(outStream,
                                reader->sampleRate,
                                (unsigned int)reader->numChannels,
                                (unsigned int)reader->bitsPerSample,
                                {},
                                0));

    if (writer == nullptr) {
      juce::MessageManager::callAsync([this] {
        statusLog.insertTextAtCaret("[ERROR] Could not create output writer\n");
        exportButton.setEnabled(true);
        selectInputButton.setEnabled(true);
      });
      return;
    }

    NeveTransformerDSP fileDsp;
    fileDsp.prepare(reader->sampleRate, 4096);
    fileDsp.setDrive(drive);
    fileDsp.setIron(iron);
    fileDsp.setHFRoll(hfRoll);
    fileDsp.setMode(mode);
    fileDsp.setZLoad(zLoad);
    fileDsp.setBypassed(bypassed);

    const int blockSize = 4096;
    juce::AudioBuffer<float> buf((int)reader->numChannels, blockSize);
    juce::AudioBuffer<float> dryBuf((int)reader->numChannels, blockSize);
    int64_t samplesProcessed = 0;

    while (samplesProcessed < reader->lengthInSamples) {
      int numToRead = (int)juce::jmin((int64_t)blockSize,
                                       reader->lengthInSamples - samplesProcessed);

      buf.setSize((int)reader->numChannels, numToRead, false, false, false);
      dryBuf.setSize((int)reader->numChannels, numToRead, false, false, false);

      bool hasRight = reader->numChannels > 1;
      if (!reader->read(&buf, 0, numToRead, samplesProcessed, true, hasRight)) {
        juce::MessageManager::callAsync([this, samplesProcessed] {
          statusLog.insertTextAtCaret("[ERROR] Read failure at sample " +
                                     juce::String(samplesProcessed) + "\n");
        });
        break;
      }

      // Store dry copy
      for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        dryBuf.copyFrom(ch, 0, buf, ch, 0, numToRead);

      fileDsp.processBlock(buf);

      // Apply wet/dry mix
      if (mix < 1.0f) {
        float dryGain = 1.0f - mix;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
          auto *wet = buf.getWritePointer(ch);
          auto *dry = dryBuf.getReadPointer(ch);
          for (int i = 0; i < numToRead; ++i)
            wet[i] = wet[i] * mix + dry[i] * dryGain;
        }
      }

      if (!writer->writeFromAudioSampleBuffer(buf, 0, numToRead)) {
        juce::MessageManager::callAsync([this] {
          statusLog.insertTextAtCaret("[ERROR] Failed to write output\n");
        });
        break;
      }

      samplesProcessed += numToRead;
      progress = (double)samplesProcessed / (double)reader->lengthInSamples;
    }

    int64_t totalSamples = reader->lengthInSamples;
    double finalSampleRate = reader->sampleRate;
    writer.reset();
    reader.reset();

    juce::MessageManager::callAsync([this, samplesProcessed, totalSamples,
                                     finalSampleRate, startTime] {
      auto endTime = juce::Time::getMillisecondCounterHiRes();
      double elapsedSec = (endTime - startTime) / 1000.0;
      double audioSec = (double)samplesProcessed / finalSampleRate;

      statusLog.insertTextAtCaret(
          juce::String(samplesProcessed) + "/" + juce::String(totalSamples) +
          " samples (" + juce::String((samplesProcessed * 100.0) / totalSamples, 1) + "%)\n");

      statusLog.insertTextAtCaret(
          juce::String(audioSec, 1) + "s in " + juce::String(elapsedSec, 2) +
          "s (" + juce::String(audioSec / elapsedSec, 1) + "x RT)\n");
      statusLog.insertTextAtCaret("--- Export complete ---\n\n");
      progressBar.setTextToDisplay("Done!");
      exportButton.setEnabled(true);
      selectInputButton.setEnabled(true);
    });
  });
}

// --- A/B Comparison ---

void MainComponent::captureSnapshot(bool isA) {
  Preset &snap = isA ? snapshotA : snapshotB;
  snap.drive = (float)driveSlider.getValue();
  snap.iron = (float)ironSlider.getValue();
  snap.hfRoll = (float)hfRollSlider.getValue();
  snap.mix = (float)mixSlider.getValue();
  snap.micMode = modeButton.getToggleState();
  snap.hiZLoad = zLoadButton.getToggleState();
}

void MainComponent::loadSnapshot(bool isA) {
  const Preset &snap = isA ? snapshotA : snapshotB;
  driveSlider.setValue(snap.drive, juce::dontSendNotification);
  ironSlider.setValue(snap.iron, juce::dontSendNotification);
  hfRollSlider.setValue(snap.hfRoll, juce::dontSendNotification);
  mixSlider.setValue(snap.mix, juce::dontSendNotification);
  modeButton.setToggleState(snap.micMode, juce::dontSendNotification);
  zLoadButton.setToggleState(snap.hiZLoad, juce::dontSendNotification);

  dsp.setDrive(snap.drive);
  dsp.setIron(snap.iron);
  dsp.setHFRoll(snap.hfRoll);
  dsp.setMode(snap.micMode);
  dsp.setZLoad(snap.hiZLoad);
  mixValue.store(snap.mix, std::memory_order_relaxed);
}
