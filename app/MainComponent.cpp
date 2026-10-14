#include "MainComponent.h"
#include "SampleLoader.h"

MainComponent::MainComponent()
{
    title.setText("ESS ELEVEN HUNDRED  /  AKAI S1100", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centredLeft);
    title.setColour(juce::Label::textColourId, juce::Colour(0xff7dffb1));
    title.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    addAndMakeVisible(title);

    status.setJustificationType(juce::Justification::centredLeft);
    status.setColour(juce::Label::textColourId, juce::Colour(0xffc8d4c8));
    status.setFont(juce::FontOptions(14.0f));
    addAndMakeVisible(status);

    addAndMakeVisible(keyboard);
    keyboard.setWantsKeyboardFocus(true);
    keyboard.setAvailableRange(21, 108); // 88-key: A0 .. C8
    keyboard.setLowestVisibleKey(32);

    loadKit();

    // On-screen keyboard -> collector.
    midiCollector.reset(44100.0); // replaced with the real rate in prepareToPlay
    keyboardState.addListener(&midiCollector);

    // Every hardware MIDI input that exists -> collector.
    for (const auto& device : juce::MidiInput::getAvailableDevices())
        deviceManager.setMidiInputDeviceEnabled(device.identifier, true);
    deviceManager.addMidiInputDeviceCallback({}, &midiCollector);

    // A virtual MIDI port other software can connect to (no hardware needed).
    virtualInput = juce::MidiInput::createNewDevice("S1100 Emulator", &midiCollector);
    if (virtualInput != nullptr)
        virtualInput->start();

    startTimerHz(20);
    setSize(980, 280);
    setAudioChannels(0, 2);
}

MainComponent::~MainComponent()
{
    stopTimer();
    shutdownAudio();
    keyboardState.removeListener(&midiCollector);
    deviceManager.removeMidiInputDeviceCallback({}, &midiCollector);
    if (virtualInput != nullptr)
        virtualInput->stop();
}

juce::File MainComponent::findSamplesDirectory() const
{
    const juce::File compiledIn(S1100_SAMPLES_DIR);
    if (compiledIn.isDirectory())
        return compiledIn;

    const juce::File cwd = juce::File::getCurrentWorkingDirectory().getChildFile("samples");
    if (cwd.isDirectory())
        return cwd;

    const juce::File exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                               .getParentDirectory()
                               .getChildFile("samples");
    return exe;
}

void MainComponent::loadKit()
{
    const auto dir = findSamplesDirectory();
    auto program = loadDreamsKit(dir);
    loadedSamples = static_cast<int>(program.samples.size());
    const juce::String where = dir.getFullPathName();
    if (loadedSamples > 0) {
        kitLine = "PROG  " + juce::String(program.name) + "    " + juce::String(loadedSamples)
                  + " samples    GM  B0 808  C1 kick  C#1 perc  D1 snare  D#1 clap  F#1/A#1 hats  C#2 crash  D#2 ride  (vel layers)";
    } else {
        kitLine = "NO SAMPLES at " + where
                  + "  —  sine fallback. Copy the Dreams kit into samples/ (see README).";
    }
    status.setText(kitLine, juce::dontSendNotification);
    machine.setProgram(std::move(program));
}

void MainComponent::prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate)
{
    machine.prepare(sampleRate);
    machine.reset();
    midiCollector.reset(sampleRate);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    auto* buffer = info.buffer;
    if (buffer == nullptr || buffer->getNumChannels() == 0) {
        return;
    }

    // MIDI is applied at the start of each block; sample-accurate timing comes later.
    incomingMidi.clear();
    midiCollector.removeNextBlockOfMessages(incomingMidi, info.numSamples);
    for (const auto metadata : incomingMidi) {
        const auto message = metadata.getMessage();
        const auto* data = message.getRawData();
        const int size = message.getRawDataSize();
        if (size > 0) {
            machine.midiIn(data[0],
                           size > 1 ? data[1] : static_cast<juce::uint8>(0),
                           size > 2 ? data[2] : static_cast<juce::uint8>(0));
        }
    }

    float* left = buffer->getWritePointer(0, info.startSample);
    float* right = buffer->getNumChannels() > 1 ? buffer->getWritePointer(1, info.startSample)
                                                : nullptr;
    machine.render(left, right, info.numSamples);

    for (int ch = 2; ch < buffer->getNumChannels(); ++ch)
        buffer->clear(ch, info.startSample, info.numSamples);
}

void MainComponent::releaseResources() {}

void MainComponent::timerCallback()
{
    const int voices = machine.activeVoicesSnapshot();
    juce::String leds;
    for (int i = 0; i < s1100::kNumVoices; ++i)
        leds += (i < voices) ? juce::String::fromUTF8("●") : juce::String::fromUTF8("○");

    status.setText(kitLine + "    VOICES " + juce::String(voices).paddedLeft('0', 2) + "/"
                       + juce::String(s1100::kNumVoices) + "  " + leds,
                   juce::dontSendNotification);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1d1a));
    g.setColour(juce::Colour(0xff2a332a));
    g.fillRect(getLocalBounds().removeFromTop(72));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    title.setBounds(area.removeFromTop(28));
    status.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    keyboard.setBounds(area);
}
