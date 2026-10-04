#include <juce_gui_basics/juce_gui_basics.h>
#include "MainComponent.h"
#include "MainWindow.h"

class AutosamplerApplication : public juce::JUCEApplication
{
public:
    AutosamplerApplication() = default;

    const juce::String getApplicationName() override    { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise(const juce::String&) override
    {
        auto title = getApplicationName();
        const auto buildId = juce::String(AUTOSAMPLER_BUILD_ID);
        if (buildId.isNotEmpty())
            title << " " << buildId;

        mainWindow = std::make_unique<MainWindow>(title);
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    std::unique_ptr<MainWindow> mainWindow;
};

// MainWindow implementation
MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name,
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new MainComponent(), true);
    setResizable(true, true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

START_JUCE_APPLICATION(AutosamplerApplication)
