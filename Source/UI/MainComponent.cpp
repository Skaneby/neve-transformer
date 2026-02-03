#include "MainComponent.h"

MainComponent::MainComponent() {
  // Set size - larger for more dramatic hardware aesthetic
  setSize(1000, 700);
  setOpaque(true);

  // Apply custom LookAndFeel
  setLookAndFeel(&neveLookAndFeel);

  // Load logo image
  auto logoFile = juce::File(__FILE__).getParentDirectory().getChildFile("logo.png");
  if (logoFile.existsAsFile()) {
    logoImage = juce::ImageCache::getFromFile(logoFile);
  }

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

  // Request audio permissions and setup
  setAudioChannels(2, 2); // Stereo in/out
  
  // Update device lists and status
  updateAudioDeviceSelectors();
  updateStatusLog();
}


MainComponent::~MainComponent() {
  setLookAndFeel(nullptr);
  shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected,
                                  double sampleRate) {
  // Ensure a reasonable minimum block size for pre-allocations
  const int safeBlockSize = juce::jmax(samplesPerBlockExpected, 512);

  dsp.prepare(sampleRate, safeBlockSize);

  // Pre-allocate temp buffer for processing
  tempBuffer.setSize(2, safeBlockSize);

  // Set initial parameters
  dsp.setDrive(driveSlider.getValue());
  dsp.setIron(ironSlider.getValue());
  dsp.setHFRoll(hfRollSlider.getValue());
  dsp.setMode(modeButton.getToggleState());
  dsp.setZLoad(zLoadButton.getToggleState());

  updateLatencyDisplay();
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  // Get input buffer
  auto *buffer = bufferToFill.buffer;
  const int numSamples = bufferToFill.numSamples;
  const int numChannels = buffer->getNumChannels();

  // Ensure temp buffer is large enough (safety check)
  if (tempBuffer.getNumSamples() < numSamples)
    tempBuffer.setSize(2, numSamples, false, true, true);

  // Clear temp buffer to ensure no garbage if processing fewer than 2 channels
  tempBuffer.clear();

  // Measure input level and copy to temp buffer (capped at 2 channels)
  for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
    auto *channelData = buffer->getReadPointer(ch, bufferToFill.startSample);

    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
      peak = juce::jmax(peak, std::abs(channelData[i]));

    inputLevel[ch] = peak;
    tempBuffer.copyFrom(ch, 0, channelData, numSamples);
  }

  // Handle remaining meter if input is mono
  if (numChannels == 1)
    inputLevel[1] = 0.0f;

  // Process through DSP
  dsp.processBlock(tempBuffer);

  // Copy back to output (capped at 2 channels)
  for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
    buffer->copyFrom(ch, bufferToFill.startSample, tempBuffer, ch, 0,
                     numSamples);

    // Measure output level
    auto *channelData = buffer->getReadPointer(ch, bufferToFill.startSample);
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
      peak = juce::jmax(peak, std::abs(channelData[i]));
    outputLevel[ch] = peak;
  }

  // Handle remaining meter if output is mono
  if (numChannels == 1)
    outputLevel[1] = 0.0f;
}

void MainComponent::releaseResources() { dsp.reset(); }

void MainComponent::paint(juce::Graphics &g) {
  // Dark background (Neve-style)
  g.fillAll(juce::Colour(0xff1a1a1a));

  // Draw logo if loaded - MUCH LARGER
  if (logoImage.isValid()) {
    auto logoArea = juce::Rectangle<float>(30, 10, getWidth() - 60, 100);
    g.drawImage(logoImage, logoArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
  }

  // Main panel background with gradient for depth
  auto panelBounds = juce::Rectangle<float>(30, 120, getWidth() - 60, getHeight() - 150);
  
  // Add subtle gradient for vintage hardware look
  juce::ColourGradient panelGradient(
      juce::Colour(0xff323232), panelBounds.getCentreX(), panelBounds.getY(),
      juce::Colour(0xff282828), panelBounds.getCentreX(), panelBounds.getBottom(), false);
  g.setGradientFill(panelGradient);
  g.fillRoundedRectangle(panelBounds, 12.0f);

  // Inner shadow for depth
  g.setColour(juce::Colour(0xff0f0f0f).withAlpha(0.6f));
  g.drawRoundedRectangle(panelBounds.reduced(2), 10.0f, 4.0f);

  // Panel border highlight
  g.setColour(juce::Colour(0xff4a4a4a).withAlpha(0.3f));
  g.drawRoundedRectangle(panelBounds.expanded(1), 12.0f, 1.5f);

  // Simple level meters
  const int meterX = getWidth() - 80;
  const int meterY = 80;
  const int meterHeight = 200;
  const int meterWidth = 15;

  for (int ch = 0; ch < 2; ++ch) {
    int x = meterX + ch * 25;

    // Background
    g.setColour(juce::Colour(0xff444444));
    g.fillRect(x, meterY, meterWidth, meterHeight);

    // Input meter (left side of bar)
    g.setColour(juce::Colours::green);
    int inputHeight = static_cast<int>(inputLevel[ch] * meterHeight);
    g.fillRect(x, meterY + meterHeight - inputHeight, meterWidth / 2,
               inputHeight);

    // Output meter (right side of bar)
    g.setColour(juce::Colours::yellow);
    int outputHeight = static_cast<int>(outputLevel[ch] * meterHeight);
    g.fillRect(x + meterWidth / 2, meterY + meterHeight - outputHeight,
               meterWidth / 2, outputHeight);
  }

  // Meter labels
  g.setColour(juce::Colours::lightgrey);
  g.setFont(10.0f);
  g.drawText("L", meterX, meterY + meterHeight + 5, 15, 15,
             juce::Justification::centred);
  g.drawText("R", meterX + 25, meterY + meterHeight + 5, 15, 15,
             juce::Justification::centred);
}

void MainComponent::resized() {
  auto area = getLocalBounds();

  // Hide text label if we have a logo image
  titleLabel.setVisible(!logoImage.isValid());

  // Logo/Title area at top (MUCH LARGER - 110px for logo)
  area.removeFromTop(110);

  // Preset controls row - LARGER
  auto presetArea = area.removeFromTop(45).reduced(50, 8);
  presetLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  presetLabel.setBounds(presetArea.removeFromLeft(90));
  presetSelector.setBounds(presetArea.removeFromLeft(320));
  presetArea.removeFromLeft(20);
  savePresetButton.setBounds(presetArea.removeFromLeft(150));

  // Split into left (controls) and right (device selection + status)
  auto mainArea = area.reduced(60, 20);
  auto rightPanel = mainArea.removeFromRight(300);
  auto leftPanel = mainArea;

  // === RIGHT PANEL: Device selection + Status ===
  
  // Input device selector
  auto inputDevArea = rightPanel.removeFromTop(30);
  inputDeviceLabel.setBounds(inputDevArea.removeFromLeft(65));
  inputDeviceSelector.setBounds(inputDevArea);

  rightPanel.removeFromTop(8);

  // Output device selector
  auto outputDevArea = rightPanel.removeFromTop(30);
  outputDeviceLabel.setBounds(outputDevArea.removeFromLeft(65));
  outputDeviceSelector.setBounds(outputDevArea);

  rightPanel.removeFromTop(15);

  // Status log
  statusLogLabel.setBounds(rightPanel.removeFromTop(20));
  rightPanel.removeFromTop(5);
  statusLog.setBounds(rightPanel.removeFromTop(330));

  // === LEFT PANEL: Knobs and buttons ===
  auto controlArea = leftPanel.withTrimmedRight(20);

  // MUCH LARGER Knobs - 200px!
  const int knobWidth = 200;
  const int knobHeight = 200;
  const int spacing = 50;

  auto knobArea = controlArea.removeFromTop(knobHeight + 60);

  driveSlider.setBounds(knobArea.removeFromLeft(knobWidth).withTrimmedTop(30));
  knobArea.removeFromLeft(spacing);
  ironSlider.setBounds(knobArea.removeFromLeft(knobWidth).withTrimmedTop(30));
  knobArea.removeFromLeft(spacing);
  hfRollSlider.setBounds(knobArea.removeFromLeft(knobWidth).withTrimmedTop(30));

  // LARGER Buttons
  auto buttonArea = controlArea.removeFromTop(50).reduced(0, 5);
  const int buttonWidth = 160;

  modeButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
  buttonArea.removeFromLeft(spacing);
  zLoadButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
  buttonArea.removeFromLeft(spacing);
  bypassButton.setBounds(buttonArea.removeFromLeft(buttonWidth));

  // Latency label at bottom
  latencyLabel.setBounds(controlArea.removeFromBottom(25).withTrimmedLeft(300));
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
  // Skip separator items
  auto itemText = presetSelector.getItemText(index);
  if (itemText.startsWith("---"))
    return;
  
  auto preset = presetManager.getPreset(index);
  if (preset.name.isEmpty())
    return;

  // Update UI
  driveSlider.setValue(preset.drive, juce::dontSendNotification);
  ironSlider.setValue(preset.iron, juce::dontSendNotification);
  hfRollSlider.setValue(preset.hfRoll, juce::dontSendNotification);
  modeButton.setToggleState(preset.micMode, juce::dontSendNotification);
  zLoadButton.setToggleState(preset.hiZLoad, juce::dontSendNotification);

  // Update DSP
  dsp.setDrive(preset.drive);
  dsp.setIron(preset.iron);
  dsp.setHFRoll(preset.hfRoll);
  dsp.setMode(preset.micMode);
  dsp.setZLoad(preset.hiZLoad);
}

void MainComponent::saveCurrentPreset() {
  auto *alertWindow = new juce::AlertWindow("Save Preset", "Enter a name for this preset:",
                       juce::MessageBoxIconType::QuestionIcon);
                       
  alertWindow->addTextEditor("name", "My Preset");
  alertWindow->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
  alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

  alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([this, alertWindow](int result) {
    if (result == 1) {
      auto name = alertWindow->getTextEditorContents("name");
      if (name.isNotEmpty()) {
        Preset preset;
        preset.name = name;
        preset.drive = static_cast<float>(driveSlider.getValue());
        preset.iron = static_cast<float>(ironSlider.getValue());
        preset.hfRoll = static_cast<float>(hfRollSlider.getValue());
        preset.micMode = modeButton.getToggleState();
        preset.hiZLoad = zLoadButton.getToggleState();

        presetManager.addPreset(preset);
        updatePresetSelector();

        // Select the newly added preset
        presetSelector.setSelectedItemIndex(presetSelector.getNumItems() - 1);
      }
    }
    delete alertWindow;
  }));
}

void MainComponent::updateAudioDeviceSelectors() {
  inputDeviceSelector.clear();
  outputDeviceSelector.clear();

  auto* device = deviceManager.getCurrentAudioDevice();
  if (device == nullptr)
    return;

  auto* deviceType = deviceManager.getCurrentDeviceTypeObject();
  
  // Populate input devices
  auto inputNames = deviceType->getDeviceNames(true);
  for (int i = 0; i < inputNames.size(); ++i) {
    inputDeviceSelector.addItem(inputNames[i], i + 1);
    if (inputNames[i] == device->getName())
      inputDeviceSelector.setSelectedItemIndex(i, juce::dontSendNotification);
  }

  // Populate output devices
  auto outputNames = deviceType->getDeviceNames(false);
  for (int i = 0; i < outputNames.size(); ++i) {
    outputDeviceSelector.addItem(outputNames[i], i + 1);
    if (outputNames[i] == device->getName())
      outputDeviceSelector.setSelectedItemIndex(i, juce::dontSendNotification);
  }
}

void MainComponent::updateStatusLog() {
  juce::String status;
  
  auto* device = deviceManager.getCurrentAudioDevice();
  if (device == nullptr) {
    status << "[ERROR] No audio device available\n";
    statusLog.setText(status, false);
    return;
  }

  status << "=== AUDIO DEVICE INFO ===\n";
  status << "Device: " << device->getName() << "\n";
  status << "Type: " << device->getTypeName() << "\n\n";

  status << "=== CONFIGURATION ===\n";
  status << "Sample Rate: " << juce::String(device->getCurrentSampleRate(), 0) << " Hz\n";
  status << "Buffer Size: " << juce::String(device->getCurrentBufferSizeSamples()) << " samples\n";
  status << "Latency: " << juce::String(dsp.getLatencySamples()) << " samples\n\n";

  status << "=== INPUT ===\n";
  auto inputChannels = device->getActiveInputChannels();
  status << "Active Channels: " << inputChannels.countNumberOfSetBits() << "\n";
  auto inputChannelNames = device->getInputChannelNames();
  for (int i = 0; i < inputChannelNames.size() && i < 4; ++i) {
    if (inputChannels[i])
      status << "  [" << juce::String(i + 1) << "] " << inputChannelNames[i] << "\n";
  }

  status << "\n=== OUTPUT ===\n";
  auto outputChannels = device->getActiveOutputChannels();
  status << "Active Channels: " << outputChannels.countNumberOfSetBits() << "\n";
  auto outputChannelNames = device->getOutputChannelNames();
  for (int i = 0; i < outputChannelNames.size() && i < 4; ++i) {
    if (outputChannels[i])
      status << "  [" << juce::String(i + 1) << "] " << outputChannelNames[i] << "\n";
  }

  status << "\n=== DSP STATUS ===\n";
  status << "Drive: " << juce::String(driveSlider.getValue(), 2) << "\n";
  status << "Iron: " << juce::String(ironSlider.getValue(), 2) << "\n";
  status << "HF Roll: " << juce::String(hfRollSlider.getValue(), 2) << "\n";
  status << "Mode: " << (modeButton.getToggleState() ? "MIC" : "LINE") << "\n";
  status << "Hi-Z Load: " << (zLoadButton.getToggleState() ? "ON" : "OFF") << "\n";
  status << "Bypassed: " << (bypassButton.getToggleState() ? "YES" : "NO") << "\n";

  statusLog.setText(status, false);
}
