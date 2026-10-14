#pragma once

#include <memory>

#include <juce_audio_utils/juce_audio_utils.h>

#include "s1100/machine.h"

// Standalone shell around the emulator core: audio out, MIDI in (hardware
// ports plus a virtual "S1100 Emulator" port), an on-screen keyboard, and
// the Dreams drum kit loaded from samples/.
class MainComponent : public juce::AudioAppComponent,
                      private juce::Timer {
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    juce::File findSamplesDirectory() const;
    void loadKit();

    s1100::Machine machine;

    // Thread-safe hand-off of MIDI from the UI / MIDI threads to the audio thread.
    juce::MidiMessageCollector midiCollector;
    juce::MidiBuffer incomingMidi;

    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard { keyboardState,
                                           juce::MidiKeyboardComponent::horizontalKeyboard };
    juce::Label title;
    juce::Label status;
    juce::String kitLine;
    std::unique_ptr<juce::MidiInput> virtualInput;
    int loadedSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
