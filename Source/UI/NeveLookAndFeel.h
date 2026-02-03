#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Custom LookAndFeel for Neve-style controls
 */
class NeveLookAndFeel : public juce::LookAndFeel_V4 {
public:
  NeveLookAndFeel() {
    // Set dark color scheme
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffcc4444));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffcc4444));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff3a3a3a));
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::lightgrey);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2d2d2d));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  }

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override {
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    // Background circle (outer ring)
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillEllipse(bounds);

    // Inner circle (knob body)
    auto innerBounds = bounds.reduced(lineW);
    juce::ColourGradient gradient(
        juce::Colour(0xff4a4a4a), bounds.getCentreX(), bounds.getY(),
        juce::Colour(0xff2a2a2a), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillEllipse(innerBounds);

    // Track arc (background)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                                arcRadius, arcRadius, 0.0f, rotaryStartAngle,
                                rotaryEndAngle, true);

    g.setColour(juce::Colour(0xff252525));
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

    // Value arc (filled portion)
    if (slider.isEnabled()) {
      juce::Path valueArc;
      valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius,
                             arcRadius, 0.0f, rotaryStartAngle, toAngle, true);

      g.setColour(juce::Colour(0xffcc4444)); // Neve red
      g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Pointer line
    juce::Path pointer;
    auto pointerLength = radius * 0.6f;
    auto pointerThickness = 3.0f;
    pointer.addRectangle(-pointerThickness * 0.5f, -radius + lineW, pointerThickness,
                         pointerLength);
    pointer.applyTransform(
        juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(),
                                                             bounds.getCentreY()));

    g.setColour(juce::Colours::lightgrey);
    g.fillPath(pointer);

    // Center dot
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillEllipse(bounds.getCentreX() - 4, bounds.getCentreY() - 4, 8, 8);
  }

  void drawToggleButton(juce::Graphics &g, juce::ToggleButton &button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override {
    auto bounds = button.getLocalBounds().toFloat().reduced(2);
    auto isOn = button.getToggleState();

    // Background
    g.setColour(isOn ? juce::Colour(0xffcc4444) : juce::Colour(0xff3a3a3a));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Highlight effect when hovered or down
    if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown) {
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.fillRoundedRectangle(bounds, 4.0f);
    }

    // Border
    g.setColour(juce::Colour(0xff252525));
    g.drawRoundedRectangle(bounds, 4.0f, 1.5f);

    // Text
    g.setColour(isOn ? juce::Colours::white : juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
  }

  void drawComboBox(juce::Graphics &g, int width, int height, bool,
                    int, int, int, int, juce::ComboBox &box) override {
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

    g.setColour(juce::Colour(0xff2d2d2d));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(juce::Colour(0xff5a5a5a));
    g.drawRoundedRectangle(bounds.reduced(0.5f, 0.5f), 4.0f, 1.0f);

    juce::Path arrow;
    auto arrowX = width - 20.0f;
    auto arrowY = height * 0.5f;
    arrow.addTriangle(arrowX, arrowY - 3.0f, arrowX + 6.0f, arrowY - 3.0f,
                      arrowX + 3.0f, arrowY + 3.0f);

    g.setColour(juce::Colours::lightgrey);
    g.fillPath(arrow);
  }

  void drawPopupMenuBackground(juce::Graphics &g, int width, int height) override {
    g.fillAll(juce::Colour(0xff2d2d2d));
    g.setColour(juce::Colour(0xff5a5a5a));
    g.drawRect(0, 0, width, height);
  }

  void drawPopupMenuItem(juce::Graphics &g, const juce::Rectangle<int> &area,
                         bool isSeparator, bool isActive, bool isHighlighted,
                         bool isTicked, bool hasSubMenu, const juce::String &text,
                         const juce::String &shortcutKeyText,
                         const juce::Drawable *icon, const juce::Colour *textColour) override {
    if (isSeparator) {
      auto r = area.reduced(5, 0);
      r.removeFromTop(r.getHeight() / 2 - 1);
      g.setColour(juce::Colour(0xff5a5a5a));
      g.fillRect(r.removeFromTop(1));
    } else {
      auto textColourToUse = juce::Colours::lightgrey;

      if (isHighlighted && isActive) {
        g.setColour(juce::Colour(0xffcc4444).withAlpha(0.3f));
        g.fillRect(area);
        textColourToUse = juce::Colours::white;
      }

      g.setColour(textColourToUse);
      g.setFont(juce::FontOptions(14.0f));

      auto r = area.reduced(8, 0);
      g.drawFittedText(text, r, juce::Justification::centredLeft, 1);
    }
  }
};
