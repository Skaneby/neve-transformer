#include "UI/MainComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

/**
 * Neve Transformer - Standalone Application
 */
class NeveTransformerApplication : public juce::JUCEApplication {
public:
  NeveTransformerApplication() {}

  const juce::String getApplicationName() override {
    return "Neve Transformer";
  }
  const juce::String getApplicationVersion() override { return "1.0.0"; }
  bool moreThanOneInstanceAllowed() override { return false; }

  void initialise(const juce::String &commandLine) override {
    juce::ignoreUnused(commandLine);
    mainWindow.reset(new MainWindow(getApplicationName()));
  }

  void shutdown() override { mainWindow = nullptr; }

  void systemRequestedQuit() override { quit(); }

  void anotherInstanceStarted(const juce::String &commandLine) override {
    juce::ignoreUnused(commandLine);
  }

  class MainWindow : public juce::DocumentWindow {
  public:
    MainWindow(juce::String name)
        : DocumentWindow(name, juce::Colour(0xff1a1a1a),
                         DocumentWindow::allButtons) {
      setUsingNativeTitleBar(true);
      setContentOwned(new MainComponent(), true);

#if JUCE_IOS || JUCE_ANDROID
      setFullScreen(true);
#else
      setResizable(false, false);
      centreWithSize(getWidth(), getHeight());
#endif

      setVisible(true);
    }

    void closeButtonPressed() override {
      JUCEApplication::getInstance()->systemRequestedQuit();
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
  };

private:
  std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(NeveTransformerApplication)
