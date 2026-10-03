#include "MainComponent.h"

MainComponent::MainComponent()
    : samplerEngine(midiOutputManager, recordingManager)
{
    // Audio device setup
    auto result = audioDeviceManager.initialiseWithDefaultDevices(2, 2);
    if (result.isNotEmpty()) {
        DBG("Audio device init error: " + result);
    }
    audioDeviceManager.addAudioCallback(this);

    // Title
    titleLabel.setText("JUCE AUTOSAMPLER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Start Note
    startNoteLabel.setText("Start Note:", juce::dontSendNotification);
    addAndMakeVisible(startNoteLabel);
    startNoteSlider.setRange(0, 127, 1);
    startNoteSlider.setValue(48);
    startNoteSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    startNoteSlider.onValueChange = [this] {
        startNoteNameLabel.setText(getMidiNoteDisplayName((int) startNoteSlider.getValue()),
                                   juce::dontSendNotification);
    };
    addAndMakeVisible(startNoteSlider);
    startNoteNameLabel.setText(getMidiNoteDisplayName(48), juce::dontSendNotification);
    addAndMakeVisible(startNoteNameLabel);

    // End Note
    endNoteLabel.setText("End Note:", juce::dontSendNotification);
    addAndMakeVisible(endNoteLabel);
    endNoteSlider.setRange(0, 127, 1);
    endNoteSlider.setValue(72);
    endNoteSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    endNoteSlider.onValueChange = [this] {
        endNoteNameLabel.setText(getMidiNoteDisplayName((int) endNoteSlider.getValue()),
                                  juce::dontSendNotification);
    };
    addAndMakeVisible(endNoteSlider);
    endNoteNameLabel.setText(getMidiNoteDisplayName(72), juce::dontSendNotification);
    addAndMakeVisible(endNoteNameLabel);

    // Duration
    durationLabel.setText("Duration (s):", juce::dontSendNotification);
    addAndMakeVisible(durationLabel);
    durationSlider.setRange(0.1, 60.0, 0.1);
    durationSlider.setValue(3.0);
    durationSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible(durationSlider);

    // Velocity
    velocityLabel.setText("Velocity:", juce::dontSendNotification);
    addAndMakeVisible(velocityLabel);
    velocitySlider.setRange(1, 127, 1);
    velocitySlider.setValue(100);
    velocitySlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    addAndMakeVisible(velocitySlider);

    // MIDI Channel
    midiChannelLabel.setText("MIDI Channel:", juce::dontSendNotification);
    addAndMakeVisible(midiChannelLabel);
    for (int i = 1; i <= 16; i++) {
        midiChannelCombo.addItem("Ch " + juce::String(i), i);
    }
    midiChannelCombo.setSelectedId(1);
    addAndMakeVisible(midiChannelCombo);

    // MIDI Output Device
    midiOutputLabel.setText("MIDI Output:", juce::dontSendNotification);
    addAndMakeVisible(midiOutputLabel);
    addAndMakeVisible(midiOutputCombo);
    refreshMidiButton.onClick = [this] { refreshMidiDevices(); };
    addAndMakeVisible(refreshMidiButton);
    refreshMidiDevices();

    // Sample Pack Name
    samplePackLabel.setText("Pack Name:", juce::dontSendNotification);
    addAndMakeVisible(samplePackLabel);
    samplePackNameEditor.setText("MySamples", false);
    samplePackNameEditor.setMultiLine(false);
    samplePackNameEditor.setReturnKeyStartsNewLine(false);
    addAndMakeVisible(samplePackNameEditor);

    // Audio Settings
    audioSettingsButton.onClick = [this] { showAudioSettings(); };
    addAndMakeVisible(audioSettingsButton);

    // Run/Stop buttons
    runButton.onClick = [this] { startSampling(); };
    runButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    addAndMakeVisible(runButton);

    stopButton.onClick = [this] { stopSampling(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    stopButton.setEnabled(false);
    addAndMakeVisible(stopButton);

    // Export controls
    exportFormatLabel.setText("Export Format:", juce::dontSendNotification);
    addAndMakeVisible(exportFormatLabel);
    exportFormatCombo.addItem("SFZ", (int) ExportFormat::SFZ + 1);
    exportFormatCombo.addItem("SF2", (int) ExportFormat::SF2 + 1);
    exportFormatCombo.addItem("Decent Sampler", (int) ExportFormat::DecentSampler + 1);
    exportFormatCombo.setSelectedId((int) ExportFormat::SFZ + 1);
    addAndMakeVisible(exportFormatCombo);

    exportButton.onClick = [this] { exportSamples(); };
    exportButton.setEnabled(false);
    addAndMakeVisible(exportButton);

    exportStatusLabel.setText("", juce::dontSendNotification);
    addAndMakeVisible(exportStatusLabel);

    // Status
    statusLabel.setText("Status: Idle", juce::dontSendNotification);
    statusLabel.setFont(juce::Font(16.0f));
    addAndMakeVisible(statusLabel);

    addAndMakeVisible(progressBar);
    progress = 0.0;

    // Sample start and attack
    sampleStartLabel.setText("Start:", juce::dontSendNotification);
    addAndMakeVisible(sampleStartLabel);
    sampleStartSlider.setRange(0.0, 0.0, 1.0);
    sampleStartSlider.setValue(0.0, juce::dontSendNotification);
    sampleStartSlider.setTextValueSuffix(" ms");
    sampleStartSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 72, 20);
    sampleStartSlider.onValueChange = [this] {
        sampleEdit.setStartMs(sampleStartSlider.getValue());
        refreshStartAutoLabel();
    };
    sampleStartSlider.setEnabled(false);
    addAndMakeVisible(sampleStartSlider);
    sampleStartAutoLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(sampleStartAutoLabel);
    autoStartButton.onClick = [this] {
        sampleStartSlider.setValue(detectedStartMs, juce::sendNotification);
    };
    autoStartButton.setEnabled(false);
    addAndMakeVisible(autoStartButton);

    attackLabel.setText("Attack:", juce::dontSendNotification);
    addAndMakeVisible(attackLabel);
    attackSlider.setRange(0.0, 50.0, 1.0);
    attackSlider.setValue(SampleEdit::defaultAttackMs, juce::dontSendNotification);
    attackSlider.setTextValueSuffix(" ms");
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 72, 20);
    attackSlider.onValueChange = [this] {
        sampleEdit.setAttackMs(attackSlider.getValue());
    };
    attackSlider.setEnabled(false);
    addAndMakeVisible(attackSlider);

    // MIDI Keyboard for sample preview
    midiKeyboard.setAvailableRange(36, 84); // C2 to C6
    midiKeyboard.setEnabled(false);
    addAndMakeVisible(midiKeyboard);
    keyboardState.addListener(this);

    // Sampler Engine callbacks
    samplerEngine.setStatusCallback([this](const juce::String& status, int completed, int total) {
        juce::MessageManager::callAsync([this, status, completed, total] {
            statusLabel.setText("Status: " + status, juce::dontSendNotification);
            progress = total > 0 ? static_cast<double>(completed) / total : 0.0;
            repaint();
        });
    });

    samplerEngine.setCompletionCallback([this](const std::map<int, SampleData>& samples) {
        juce::MessageManager::callAsync([this, samples] {
            capturedSamples = samples;

            // New takes get a fresh detection. Previous slider values are not reused.
            sampleEdit.setAttackMs(SampleEdit::defaultAttackMs);
            attackSlider.setValue(SampleEdit::defaultAttackMs, juce::dontSendNotification);
            const double maxStartMs = std::max(0.0, SampleEdit::getShortestSampleMs(capturedSamples));
            const double interval = maxStartMs >= 1.0 ? 1.0 : 0.0;
            sampleStartSlider.setRange(0.0, maxStartMs, interval);
            detectedStartMs = std::min(startDetector.detectStartMs(capturedSamples, sampleEdit.getAttackMs()), maxStartMs);
            sampleEdit.setStartMs(detectedStartMs);
            sampleStartSlider.setValue(detectedStartMs, juce::dontSendNotification);
            refreshStartAutoLabel();

            previewManager.setSamples(&capturedSamples);
            progress = 1.0;
            updateControlsEnabled();
        });
    });

    setSize(500, 920);
}

MainComponent::~MainComponent()
{
    keyboardState.removeListener(this);
    audioDeviceManager.removeAudioCallback(this);
    samplerEngine.stopSampling();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::grey);
    g.drawHorizontalLine(50, 10.0f, (float) getWidth() - 10.0f);
    g.drawHorizontalLine(370, 10.0f, (float) getWidth() - 10.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(15);
    int rowHeight = 30;
    int labelWidth = 100;
    int spacing = 8;

    // Title
    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(spacing);

    // Start Note row
    auto row = area.removeFromTop(rowHeight);
    startNoteLabel.setBounds(row.removeFromLeft(labelWidth));
    startNoteNameLabel.setBounds(row.removeFromRight(50));
    startNoteSlider.setBounds(row);
    area.removeFromTop(spacing);

    // End Note row
    row = area.removeFromTop(rowHeight);
    endNoteLabel.setBounds(row.removeFromLeft(labelWidth));
    endNoteNameLabel.setBounds(row.removeFromRight(50));
    endNoteSlider.setBounds(row);
    area.removeFromTop(spacing);

    // Duration row
    row = area.removeFromTop(rowHeight);
    durationLabel.setBounds(row.removeFromLeft(labelWidth));
    durationSlider.setBounds(row);
    area.removeFromTop(spacing);

    // Velocity row
    row = area.removeFromTop(rowHeight);
    velocityLabel.setBounds(row.removeFromLeft(labelWidth));
    velocitySlider.setBounds(row);
    area.removeFromTop(spacing);

    // MIDI Channel row
    row = area.removeFromTop(rowHeight);
    midiChannelLabel.setBounds(row.removeFromLeft(labelWidth));
    midiChannelCombo.setBounds(row.removeFromLeft(120));
    area.removeFromTop(spacing);

    // MIDI Output row
    row = area.removeFromTop(rowHeight);
    midiOutputLabel.setBounds(row.removeFromLeft(labelWidth));
    refreshMidiButton.setBounds(row.removeFromRight(70));
    row.removeFromRight(5);
    midiOutputCombo.setBounds(row);
    area.removeFromTop(spacing);

    // Sample Pack Name row
    row = area.removeFromTop(rowHeight);
    samplePackLabel.setBounds(row.removeFromLeft(labelWidth));
    samplePackNameEditor.setBounds(row);
    area.removeFromTop(spacing);

    // Audio Settings button
    audioSettingsButton.setBounds(area.removeFromTop(rowHeight));
    area.removeFromTop(spacing + 5);

    // Run/Stop buttons
    row = area.removeFromTop(40);
    runButton.setBounds(row.removeFromLeft(row.getWidth() / 2 - 5));
    row.removeFromLeft(10);
    stopButton.setBounds(row);
    area.removeFromTop(spacing);

    // Status
    statusLabel.setBounds(area.removeFromTop(rowHeight));
    area.removeFromTop(spacing);

    // Progress bar
    progressBar.setBounds(area.removeFromTop(20));
    area.removeFromTop(spacing + 5);

    // Export format row
    row = area.removeFromTop(rowHeight);
    exportFormatLabel.setBounds(row.removeFromLeft(labelWidth));
    exportFormatCombo.setBounds(row.removeFromLeft(160));
    area.removeFromTop(spacing);

    // Export button
    exportButton.setBounds(area.removeFromTop(35));
    area.removeFromTop(spacing);

    // Export status
    exportStatusLabel.setBounds(area.removeFromTop(rowHeight));
    area.removeFromTop(spacing);

    // Sample start
    row = area.removeFromTop(rowHeight);
    sampleStartLabel.setBounds(row.removeFromLeft(labelWidth));
    autoStartButton.setBounds(row.removeFromRight(70));
    row.removeFromRight(5);
    sampleStartAutoLabel.setBounds(row.removeFromRight(120));
    sampleStartSlider.setBounds(row);
    area.removeFromTop(spacing);

    // Attack
    row = area.removeFromTop(rowHeight);
    attackLabel.setBounds(row.removeFromLeft(labelWidth));
    attackSlider.setBounds(row);
    area.removeFromTop(spacing);

    // MIDI Keyboard at bottom
    midiKeyboard.setBounds(area.removeFromBottom(120));
}

void MainComponent::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    recordingManager.recordBlock(inputChannelData, numInputChannels, numSamples);

    // Clear output first
    for (int ch = 0; ch < numOutputChannels; ch++) {
        if (outputChannelData[ch] != nullptr) {
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
        }
    }

    // Mix preview playback into output
    previewManager.processBlock(outputChannelData, numOutputChannels, numSamples);
}

void MainComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr) {
        recordingManager.prepareToRecord(
            device->getCurrentSampleRate(),
            device->getActiveInputChannels().countNumberOfSetBits(),
            10.5);
        previewManager.setSampleRate(device->getCurrentSampleRate());
    }
}

void MainComponent::audioDeviceStopped()
{
    recordingManager.stopRecording();
}

void MainComponent::refreshMidiDevices()
{
    midiOutputCombo.clear();
    auto devices = midiOutputManager.getAvailableDevices();
    for (int i = 0; i < devices.size(); i++) {
        midiOutputCombo.addItem(devices[i], i + 1);
    }

    if (devices.size() > 0) {
        midiOutputCombo.setSelectedId(1);
    }
}

void MainComponent::startSampling()
{
    int startNote = (int) startNoteSlider.getValue();
    int endNote = (int) endNoteSlider.getValue();

    if (startNote > endNote) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Invalid Range", "Start note must be less than or equal to end note.");
        return;
    }

    if (midiOutputCombo.getSelectedId() == 0) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "No MIDI Output", "Please select a MIDI output device.");
        return;
    }

    auto deviceName = midiOutputCombo.getText();
    if (!midiOutputManager.openDevice(deviceName)) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "MIDI Error", "Could not open MIDI device: " + deviceName);
        return;
    }

    progress = 0.0;
    samplerEngine.startSampling(
        startNote,
        endNote,
        durationSlider.getValue(),
        midiChannelCombo.getSelectedId(),
        (int) velocitySlider.getValue()
    );
    updateControlsEnabled();
}

void MainComponent::stopSampling()
{
    samplerEngine.stopSampling();
    midiOutputManager.closeDevice();
    updateControlsEnabled();
}

void MainComponent::showAudioSettings()
{
    auto* selector = new juce::AudioDeviceSelectorComponent(
        audioDeviceManager,
        1,
        2,
        1,
        2,
        false,
        false,
        true,
        false
    );
    selector->setSize(500, 300);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector);
    options.dialogTitle = "Audio Settings";
    options.dialogBackgroundColour =
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

void MainComponent::exportSamples()
{
    if (capturedSamples.empty()) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "No Samples", "No samples have been recorded yet. Run sampling first.");
        return;
    }

    auto packName = samplePackNameEditor.getText().trim();
    if (packName.isEmpty()) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "No Pack Name", "Please enter a sample pack name.");
        return;
    }

    auto chooser = std::make_shared<juce::FileChooser>(
        "Select Export Directory", juce::File::getSpecialLocation(juce::File::userDesktopDirectory));

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser, packName](const juce::FileChooser& fc) {
            auto dir = fc.getResult();
            if (dir == juce::File()) {
                return;
            }

            auto outputDir = dir.getChildFile(packName);

            exportStatusLabel.setText("Exporting...", juce::dontSendNotification);
            exportButton.setEnabled(false);

            std::map<int, SampleData> processed;
            for (const auto& entry : capturedSamples) {
                processed.emplace(entry.first, sampleEdit.render(entry.second));
            }

            auto format = static_cast<ExportFormat>(exportFormatCombo.getSelectedId() - 1);
            bool success = false;
            juce::String formatName;

            switch (format)
            {
                case ExportFormat::SFZ:
                    success = SFZExporter::exportSamples(processed, packName, outputDir);
                    formatName = "SFZ";
                    break;
                case ExportFormat::SF2:
                    success = SF2Exporter::exportSamples(processed, packName, outputDir);
                    formatName = "SF2";
                    break;
                case ExportFormat::DecentSampler:
                    success = DecentSamplerExporter::exportSamples(processed, packName, outputDir);
                    formatName = "Decent Sampler";
                    break;
            }

            if (success) {
                exportStatusLabel.setText("Exported " + juce::String(processed.size()) +
                    " samples (" + formatName + ") to: " + outputDir.getFullPathName(),
                    juce::dontSendNotification);
            } else {
                exportStatusLabel.setText("Export failed!", juce::dontSendNotification);
            }
            exportButton.setEnabled(true);
        });
}

void MainComponent::updateControlsEnabled()
{
    const bool isSampling = samplerEngine.isSampling();
    const bool hasSamples = !capturedSamples.empty();
    const bool canEditSamples = !isSampling && hasSamples;

    runButton.setEnabled(!isSampling);
    stopButton.setEnabled(isSampling);
    exportButton.setEnabled(canEditSamples);
    midiKeyboard.setEnabled(canEditSamples);
    sampleStartSlider.setEnabled(canEditSamples);
    sampleStartAutoLabel.setEnabled(canEditSamples);
    autoStartButton.setEnabled(canEditSamples);
    attackSlider.setEnabled(canEditSamples);
}

void MainComponent::refreshStartAutoLabel()
{
    if (std::abs(sampleStartSlider.getValue() - detectedStartMs) >= 0.5) {
        sampleStartAutoLabel.setText("(auto " + juce::String(juce::roundToInt(detectedStartMs)) + " ms)",
                                     juce::dontSendNotification);
    } else {
        sampleStartAutoLabel.setText({}, juce::dontSendNotification);
    }
}

void MainComponent::handleNoteOn(juce::MidiKeyboardState*, int /*midiChannel*/, int midiNoteNumber, float /*velocity*/)
{
    previewManager.noteOn(midiNoteNumber, sampleEdit);
}

void MainComponent::handleNoteOff(juce::MidiKeyboardState*, int /*midiChannel*/, int midiNoteNumber, float /*velocity*/)
{
    previewManager.noteOff(midiNoteNumber);
}
