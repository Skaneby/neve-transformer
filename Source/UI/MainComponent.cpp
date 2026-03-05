#include "MainComponent.h"

static const juce::String kInfoText =
    "NEVE BATTRE  —  Pure Phase Signature\n\n"
    "Emulates the cumulative phase response of 6 Marinair transformers "
    "(LO1166 + 10468). Three cascaded 2nd-order allpass biquads shift the "
    "time domain without altering amplitude or frequency content.\n\n"
    "Stage 1 \xe2\x80\x94 LO1166 Resonance:   38 Hz / Q 0.55\n"
    "Stage 2 \xe2\x80\x94 LO1166 Body:        95 Hz / Q 0.45\n"
    "Stage 3 \xe2\x80\x94 10468  Air:      18 500 Hz / Q 0.40\n\n"
    "4\xc3\x97 FIR oversampling at 192 kHz internal rate.\n"
    "MIX = parallel blend of processed and dry signal.\n\n"
    "Click anywhere to dismiss.";

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

  addAndMakeVisible(titleLabel);
  titleLabel.setText("HERRSTROM", juce::dontSendNotification);
  titleLabel.setFont(juce::FontOptions(36.0f, juce::Font::bold));
  titleLabel.setJustificationType(juce::Justification::centred);
  titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));

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

  // Bypass toggle
  addAndMakeVisible(bypassButton);
  bypassButton.setLookAndFeel(&neveLookAndFeel);
  bypassButton.setButtonText("BYPASS");
  bypassButton.setToggleState(false, juce::dontSendNotification);
  bypassButton.onClick = [this]() {
    dsp.setBypassed(bypassButton.getToggleState());
  };

  // Info button
  addAndMakeVisible(infoButton);
  infoButton.setButtonText("?");
  infoButton.setLookAndFeel(&neveLookAndFeel);
  infoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
  infoButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcc4444));
  infoButton.onClick = [this]() { showHelp(&infoButton, kInfoText); };

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

  // File / Transport
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

  addAndMakeVisible(exportButton);
  exportButton.setButtonText("EXPORT");
  exportButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff882222));
  exportButton.setLookAndFeel(&neveLookAndFeel);
  exportButton.setEnabled(false);
  exportButton.onClick = [this] { exportProcessedFile(); };

  addAndMakeVisible(outputLocationLabel);
  outputLocationLabel.setText("Output: herrstrom/", juce::dontSendNotification);
  outputLocationLabel.setFont(juce::FontOptions(10.0f));
  outputLocationLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

  addAndMakeVisible(progressBar);
  progressBar.setTextToDisplay("Ready");

  formatManager.registerBasicFormats();
  setAudioChannels(2, 2);
  updateAudioDeviceSelectors();
  updateStatusLog();
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
  updateLatencyDisplay();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) {
  auto *buffer = bufferToFill.buffer;
  const int numSamples  = bufferToFill.numSamples;
  const int numChannels = buffer->getNumChannels();

  jassert(tempBuffer.getNumSamples() >= numSamples);
  tempBuffer.setSize(2, numSamples, false, false, true);
  dryBuffer.setSize(2, numSamples, false, false, true);
  tempBuffer.clear();

  if (playbackState == PlaybackState::PLAYING && readerSource != nullptr) {
    transportSource.getNextAudioBlock(bufferToFill);
    for (int ch = 0; ch < 2; ++ch) {
      int srcCh = (ch < numChannels) ? ch : 0;
      auto *channelData = buffer->getReadPointer(srcCh, bufferToFill.startSample);
      float peak = 0.0f;
      for (int i = 0; i < numSamples; ++i)
        peak = juce::jmax(peak, std::abs(channelData[i]));
      inputLevel[ch] = peak;
      tempBuffer.copyFrom(ch, 0, channelData, numSamples);
    }
  } else {
    for (int ch = 0; ch < 2; ++ch) {
      int srcCh = (ch < numChannels) ? ch : 0;
      auto *channelData = buffer->getReadPointer(srcCh, bufferToFill.startSample);
      float peak = 0.0f;
      for (int i = 0; i < numSamples; ++i)
        peak = juce::jmax(peak, std::abs(channelData[i]));
      inputLevel[ch] = peak;
      tempBuffer.copyFrom(ch, 0, channelData, numSamples);
    }
  }

  for (int ch = 0; ch < 2; ++ch)
    dryBuffer.copyFrom(ch, 0, tempBuffer, ch, 0, numSamples);

  auto cpuStart = juce::Time::getHighResolutionTicks();
  dsp.processBlock(tempBuffer);

  float mix = mixValue.load(std::memory_order_relaxed);
  if (mix < 1.0f) {
    float dryGain = 1.0f - mix;
    for (int ch = 0; ch < 2; ++ch) {
      auto *wet = tempBuffer.getWritePointer(ch);
      auto *dry = dryBuffer.getReadPointer(ch);
      for (int i = 0; i < numSamples; ++i)
        wet[i] = wet[i] * mix + dry[i] * dryGain;
    }
  }

  auto cpuEnd = juce::Time::getHighResolutionTicks();
  double elapsedSec = juce::Time::highResolutionTicksToSeconds(cpuEnd - cpuStart);
  auto *dev = deviceManager.getCurrentAudioDevice();
  if (dev != nullptr) {
    double budgetSec = (double)numSamples / dev->getCurrentSampleRate();
    float load = (float)(elapsedSec / budgetSec);
    float prev = cpuLoad.load(std::memory_order_relaxed);
    cpuLoad.store(prev * 0.9f + load * 0.1f, std::memory_order_relaxed);
  }

  for (int ch = 0; ch < 2; ++ch) {
    auto *processed = tempBuffer.getReadPointer(ch);
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
      peak = juce::jmax(peak, std::abs(processed[i]));
    outputLevel[ch] = peak;
  }

  for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
    buffer->copyFrom(ch, bufferToFill.startSample, tempBuffer, ch, 0, numSamples);
}

void MainComponent::releaseResources() {
  transportSource.releaseResources();
  dsp.reset();
}

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff1a1a1a));

  if (logoImage.isValid()) {
    auto logoArea = juce::Rectangle<float>(30, 10, (float)(getWidth() - 60), 100);
    g.drawImage(logoImage, logoArea,
                juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
  }

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

  // Level meters
  const int meterX = getWidth() - 55;
  const int meterY = 140;
  const int meterHeight = 200;
  const int meterWidth  = 14;

  for (int ch = 0; ch < 2; ++ch) {
    int x = meterX + ch * 25;
    g.setColour(juce::Colour(0xff444444));
    g.fillRect(x, meterY, meterWidth, meterHeight);
    g.setColour(juce::Colours::green);
    int inputH = static_cast<int>(inputLevel[ch].load() * meterHeight);
    g.fillRect(x, meterY + meterHeight - inputH, meterWidth / 2, inputH);
    g.setColour(juce::Colours::yellow);
    int outputH = static_cast<int>(outputLevel[ch].load() * meterHeight);
    g.fillRect(x + meterWidth / 2, meterY + meterHeight - outputH, meterWidth / 2, outputH);
  }
  g.setColour(juce::Colours::lightgrey);
  g.setFont(10.0f);
  g.drawText("L", meterX,      meterY + meterHeight + 5, 15, 15, juce::Justification::centred);
  g.drawText("R", meterX + 25, meterY + meterHeight + 5, 15, 15, juce::Justification::centred);

  // CPU meter
  {
    int cpuMeterY = meterY + meterHeight + 25;
    int cpuMeterW = meterWidth * 2 + 11;
    int cpuMeterH = 10;
    g.setColour(juce::Colour(0xff444444));
    g.fillRect(meterX, cpuMeterY, cpuMeterW, cpuMeterH);
    float load = juce::jlimit(0.0f, 1.0f, cpuLoad.load(std::memory_order_relaxed));
    int fillW = (int)(load * cpuMeterW);
    g.setColour(load > 0.8f ? juce::Colours::red
              : load > 0.5f ? juce::Colours::yellow
                            : juce::Colours::green);
    g.fillRect(meterX, cpuMeterY, fillW, cpuMeterH);
    g.setColour(juce::Colours::lightgrey);
    g.setFont(9.0f);
    g.drawText("CPU " + juce::String((int)(load * 100)) + "%",
               meterX - 5, cpuMeterY + cpuMeterH + 2, cpuMeterW + 10, 12,
               juce::Justification::centred);
  }

  // Waveform display
  if (!waveformArea.isEmpty()) {
    g.setColour(juce::Colour(0xff222222));
    g.fillRect(waveformArea);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(waveformArea);

    if (thumbnail.getTotalLength() > 0.0) {
      g.setColour(juce::Colour(0xff44aa44));
      thumbnail.drawChannels(g, waveformArea.reduced(2), 0.0, thumbnail.getTotalLength(), 1.0f);

      if (transportSource.getLengthInSeconds() > 0.0) {
        double posRatio = transportSource.getCurrentPosition() / transportSource.getLengthInSeconds();
        int xPos = waveformArea.getX() + 2 + (int)(posRatio * (waveformArea.getWidth() - 4));
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
  area.removeFromTop(110);

  auto mainArea = area.reduced(40, 15);
  mainArea.removeFromRight(70); // meters

  auto rightPanel = mainArea.removeFromRight(290);
  auto leftPanel  = mainArea;

  // === RIGHT PANEL ===
  auto inputDevArea = rightPanel.removeFromTop(28);
  inputDeviceLabel.setBounds(inputDevArea.removeFromLeft(65));
  inputDeviceSelector.setBounds(inputDevArea);
  rightPanel.removeFromTop(6);

  auto outputDevArea = rightPanel.removeFromTop(28);
  outputDeviceLabel.setBounds(outputDevArea.removeFromLeft(65));
  outputDeviceSelector.setBounds(outputDevArea);
  rightPanel.removeFromTop(8);

  statusLogLabel.setBounds(rightPanel.removeFromTop(18));
  rightPanel.removeFromTop(3);
  statusLog.setBounds(rightPanel.removeFromTop(110));
  rightPanel.removeFromTop(8);

  fileProcessingLabel.setBounds(rightPanel.removeFromTop(18));
  rightPanel.removeFromTop(3);
  selectInputButton.setBounds(rightPanel.removeFromTop(26));
  fileNameLabel.setBounds(rightPanel.removeFromTop(16));
  rightPanel.removeFromTop(4);
  waveformArea = rightPanel.removeFromTop(70);
  rightPanel.removeFromTop(4);

  auto transportRow = rightPanel.removeFromTop(30);
  playButton.setBounds(transportRow.removeFromLeft(65));
  transportRow.removeFromLeft(5);
  stopButton.setBounds(transportRow.removeFromLeft(65));
  transportRow.removeFromLeft(5);
  loopToggle.setBounds(transportRow.removeFromLeft(65));
  rightPanel.removeFromTop(6);

  exportButton.setBounds(rightPanel.removeFromTop(32));
  rightPanel.removeFromTop(3);
  outputLocationLabel.setBounds(rightPanel.removeFromTop(14));
  rightPanel.removeFromTop(4);
  progressBar.setBounds(rightPanel.removeFromTop(18));

  // === LEFT PANEL — minimal: info, bypass, mix, latency ===
  auto controlArea = leftPanel.withTrimmedRight(10);

  // Info + Bypass row at top
  controlArea.removeFromTop(20);
  auto topRow = controlArea.removeFromTop(36);
  auto centeredTop = topRow.withSizeKeepingCentre(260, 36);
  infoButton.setBounds(centeredTop.removeFromLeft(36));
  centeredTop.removeFromLeft(10);
  bypassButton.setBounds(centeredTop.removeFromLeft(110));

  // Mix knob centred below
  controlArea.removeFromTop(30);
  auto mixArea = controlArea.removeFromTop(110);
  auto mixBounds = mixArea.withSizeKeepingCentre(90, 90).withTrimmedTop(18);
  mixSlider.setBounds(mixBounds);

  // Latency at bottom
  latencyLabel.setBounds(controlArea.removeFromBottom(22));
}

void MainComponent::timerCallback() {
  const int meterX = getWidth() - 60;
  repaint(meterX - 5, 130, 70, 290);
  if (!waveformArea.isEmpty())
    repaint(waveformArea);
}

void MainComponent::mouseDown(const juce::MouseEvent &e) {
  if (activeBubble != nullptr) {
    activeBubble->dismiss();
    activeBubble.reset();
  }
  if (waveformArea.contains(e.getPosition()) && transportSource.getLengthInSeconds() > 0.0) {
    double clickRatio = (double)(e.x - waveformArea.getX()) / waveformArea.getWidth();
    clickRatio = juce::jlimit(0.0, 1.0, clickRatio);
    transportSource.setPosition(clickRatio * transportSource.getLengthInSeconds());
  }
}

void MainComponent::showHelp(juce::Component *anchor, const juce::String &text) {
  if (activeBubble != nullptr) {
    activeBubble->dismiss();
    activeBubble.reset();
  }
  activeBubble = std::make_unique<HelpBubble>(text, this);
  activeBubble->showAt(anchor);
}

void MainComponent::updateLatencyDisplay() {
  int latencySamples = dsp.getLatencySamples();
  latencyLabel.setText("Latency: " + juce::String(latencySamples) + " smp | Neve Bättre",
                       juce::dontSendNotification);
}

void MainComponent::updateAudioDeviceSelectors() {
  inputDeviceSelector.clear();
  outputDeviceSelector.clear();
  auto *device = deviceManager.getCurrentAudioDevice();
  if (device == nullptr) return;
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
  status << "Rate: "   << juce::String(device->getCurrentSampleRate(), 0) << " Hz\n";
  status << "Buffer: " << juce::String(device->getCurrentBufferSizeSamples()) << " smp\n";
  status << "Latency: "<< juce::String(dsp.getLatencySamples()) << " smp\n\n";
  status << "Mix: "    << juce::String(mixSlider.getValue(), 2) << "\n";
  statusLog.setText(status, false);
}

void MainComponent::selectFileInput() {
  fileChooser = std::make_unique<juce::FileChooser>(
      "Select an audio file...",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory),
      "*.wav;*.aiff;*.aif");
  auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
  fileChooser->launchAsync(flags, [this](const juce::FileChooser &fc) {
    auto file = fc.getResult();
    if (file.existsAsFile()) loadFileForPreview(file);
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

juce::File MainComponent::getOutputDirectory() {
  return juce::File("/Users/macminim1/Library/CloudStorage/"
                    "GoogleDrive-johan.skaneby@gmail.com/My Drive/"
                    "Egna projekt/herrstrom");
}

juce::File MainComponent::getOutputFile(const juce::String &originalName,
                                         const juce::String &extension) {
  auto outputDir = getOutputDirectory();
  if (!outputDir.isDirectory()) outputDir.createDirectory();
  auto now = juce::Time::getCurrentTime();
  juce::String timestamp = now.formatted("%Y-%m-%d_%H-%M-%S");
  return outputDir.getChildFile(originalName + "_" + timestamp + extension);
}

void MainComponent::exportProcessedFile() {
  if (!inputFile.existsAsFile()) return;

  juce::String ext = inputFile.getFileExtension().toLowerCase();
  if (ext != ".aiff" && ext != ".aif") ext = ".wav";

  juce::File outFile = getOutputFile(inputFile.getFileNameWithoutExtension(), ext);
  outputLocationLabel.setText("Export: " + outFile.getFileName(), juce::dontSendNotification);
  outputLocationLabel.setColour(juce::Label::textColourId, juce::Colours::white);

  exportButton.setEnabled(false);
  selectInputButton.setEnabled(false);
  progress = 0.0;
  progressBar.setTextToDisplay("Exporting...");

  statusLog.moveCaretToEnd();
  statusLog.insertTextAtCaret("\n--- Exporting ---\n");
  statusLog.insertTextAtCaret("Input: "  + inputFile.getFileName() + "\n");
  statusLog.insertTextAtCaret("Output: " + outFile.getFileName()   + "\n");

  bool bypassed    = bypassButton.getToggleState();
  float mix        = (float)mixSlider.getValue();
  auto startTime   = juce::Time::getMillisecondCounterHiRes();

  juce::Thread::launch([this, bypassed, mix, outFile, ext, startTime] {
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

    if (outFile.existsAsFile()) outFile.deleteFile();

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
        format->createWriterFor(outStream, reader->sampleRate,
                                (unsigned int)reader->numChannels,
                                (unsigned int)reader->bitsPerSample, {}, 0));
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

      for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        dryBuf.copyFrom(ch, 0, buf, ch, 0, numToRead);

      fileDsp.processBlock(buf);

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

    int64_t totalSamples     = reader->lengthInSamples;
    double  finalSampleRate  = reader->sampleRate;
    writer.reset();
    reader.reset();

    juce::MessageManager::callAsync([this, samplesProcessed, totalSamples,
                                     finalSampleRate, startTime] {
      auto endTime   = juce::Time::getMillisecondCounterHiRes();
      double elapsed = (endTime - startTime) / 1000.0;
      double audioSec = (double)samplesProcessed / finalSampleRate;
      statusLog.insertTextAtCaret(
          juce::String(samplesProcessed) + "/" + juce::String(totalSamples) +
          " samples (" + juce::String((samplesProcessed * 100.0) / totalSamples, 1) + "%)\n");
      statusLog.insertTextAtCaret(
          juce::String(audioSec, 1) + "s in " + juce::String(elapsed, 2) +
          "s (" + juce::String(audioSec / elapsed, 1) + "x RT)\n");
      statusLog.insertTextAtCaret("--- Export complete ---\n\n");
      progressBar.setTextToDisplay("Done!");
      exportButton.setEnabled(true);
      selectInputButton.setEnabled(true);
    });
  });
}
