#include "MainComponent.h"

MainComponent::MainComponent() {
  // Set size
  setSize(600, 400);
  setOpaque(true);

  // Title
  addAndMakeVisible(titleLabel);
  titleLabel.setText("NEVE TRANSFORMER", juce::dontSendNotification);
  titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
  titleLabel.setJustificationType(juce::Justification::centred);
  titleLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

  // Drive slider
  addAndMakeVisible(driveSlider);
  driveSlider.setRange(0.0, 1.0, 0.01);
  driveSlider.setValue(0.3);
  driveSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  driveSlider.onValueChange = [this]() {
    dsp.setDrive(driveSlider.getValue());
  };

  addAndMakeVisible(driveLabel);
  driveLabel.setText("DRIVE", juce::dontSendNotification);
  driveLabel.setJustificationType(juce::Justification::centred);
  driveLabel.attachToComponent(&driveSlider, false);

  // Iron slider
  addAndMakeVisible(ironSlider);
  ironSlider.setRange(0.0, 1.0, 0.01);
  ironSlider.setValue(0.5);
  ironSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  ironSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  ironSlider.onValueChange = [this]() { dsp.setIron(ironSlider.getValue()); };

  addAndMakeVisible(ironLabel);
  ironLabel.setText("IRON", juce::dontSendNotification);
  ironLabel.setJustificationType(juce::Justification::centred);
  ironLabel.attachToComponent(&ironSlider, false);

  // HF Roll slider
  addAndMakeVisible(hfRollSlider);
  hfRollSlider.setRange(0.0, 1.0, 0.01);
  hfRollSlider.setValue(0.7);
  hfRollSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  hfRollSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
  hfRollSlider.onValueChange = [this]() {
    dsp.setHFRoll(hfRollSlider.getValue());
  };

  addAndMakeVisible(hfRollLabel);
  hfRollLabel.setText("HF ROLL", juce::dontSendNotification);
  hfRollLabel.setJustificationType(juce::Justification::centred);
  hfRollLabel.attachToComponent(&hfRollSlider, false);

  // Mode toggle (Mic/Line)
  addAndMakeVisible(modeButton);
  modeButton.setButtonText("MIC MODE");
  modeButton.setToggleState(false, juce::dontSendNotification);
  modeButton.onClick = [this]() { dsp.setMode(modeButton.getToggleState()); };

  // Z Load toggle (Hi/Lo)
  addAndMakeVisible(zLoadButton);
  zLoadButton.setButtonText("HI-Z LOAD");
  zLoadButton.setToggleState(true, juce::dontSendNotification);
  zLoadButton.onClick = [this]() {
    dsp.setZLoad(zLoadButton.getToggleState());
  };

  // Bypass toggle
  addAndMakeVisible(bypassButton);
  bypassButton.setButtonText("BYPASS");
  bypassButton.setToggleState(false, juce::dontSendNotification);
  bypassButton.onClick = [this]() {
    dsp.setBypassed(bypassButton.getToggleState());
  };

  // Latency display
  addAndMakeVisible(latencyLabel);
  latencyLabel.setText("Latency: -- samples", juce::dontSendNotification);
  latencyLabel.setFont(juce::Font(12.0f));
  latencyLabel.setJustificationType(juce::Justification::centredRight);
  latencyLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

  // Request audio permissions and setup
  setAudioChannels(2, 2); // Stereo in/out
}

MainComponent::~MainComponent() { shutdownAudio(); }

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

  // Panel background
  g.setColour(juce::Colour(0xff2d2d2d));
  g.fillRoundedRectangle(20, 60, getWidth() - 40, getHeight() - 80, 8.0f);

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

  // Title at top
  titleLabel.setBounds(area.removeFromTop(50).reduced(20));

  // Controls area
  auto controlArea = area.reduced(40, 20);

  // Knobs in a row
  const int knobWidth = 120;
  const int knobHeight = 120;
  const int spacing = 20;

  auto knobArea = controlArea.removeFromTop(knobHeight + 40);

  driveSlider.setBounds(knobArea.removeFromLeft(knobWidth).withTrimmedTop(20));
  knobArea.removeFromLeft(spacing);
  ironSlider.setBounds(knobArea.removeFromLeft(knobWidth).withTrimmedTop(20));
  knobArea.removeFromLeft(spacing);
  hfRollSlider.setBounds(knobArea.removeFromLeft(knobWidth).withTrimmedTop(20));

  // Buttons
  auto buttonArea = controlArea.removeFromTop(40).reduced(0, 5);
  const int buttonWidth = 120;

  modeButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
  buttonArea.removeFromLeft(spacing);
  zLoadButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
  buttonArea.removeFromLeft(spacing);
  bypassButton.setBounds(buttonArea.removeFromLeft(buttonWidth));

  // Latency label at bottom
  latencyLabel.setBounds(controlArea.removeFromBottom(20).withTrimmedLeft(200));
}

void MainComponent::updateLatencyDisplay() {
  int latencySamples = dsp.getLatencySamples();
  latencyLabel.setText("Latency: " + juce::String(latencySamples) + " samples",
                       juce::dontSendNotification);
}
