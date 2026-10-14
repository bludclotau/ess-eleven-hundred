#include "SampleLoader.h"

s1100::Program loadDreamsKit(const juce::File& samplesDir)
{
    s1100::Program prog;
    prog.name = "DREAMS KIT";

    struct Bank {
        const char* relative;
        int note;
        int chokeGroup;
    };
    // GM percussion on an 88-key (A0=21 .. C8=108). Ten variations per drum
    // are velocity zones on one key, not neighbouring chromatic keys.
    static constexpr Bank kBanks[] = {
        {"808s", 32, 0},                  // B0
        {"kicks", 36, 0},                 // C1
        {"percs", 37, 0},                 // C#1
        {"snares", 38, 0},                // D1
        {"claps", 39, 0},                 // D#1
        {"cymbals/hats closed", 42, 1},   // F#1, mute group 1
        {"cymbals/hats open", 46, 1},     // A#1, mute group 1
        {"cymbals/crashes", 49, 0},       // C#2
        {"cymbals/rides", 51, 0},         // D#2
    };

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    for (const auto& bank : kBanks) {
        const juce::File dir = samplesDir.getChildFile(bank.relative);
        auto files = dir.findChildFiles(juce::File::findFiles, false, "*.wav");
        files.sort();
        const int n = files.size();
        if (n <= 0)
            continue;

        for (int i = 0; i < n; ++i) {
            std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(files[i]));
            if (reader == nullptr || reader->lengthInSamples <= 0)
                continue;

            const int frames = static_cast<int>(reader->lengthInSamples);
            juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels), frames);
            reader->read(&buffer, 0, frames, 0, true, true);

            s1100::Sample sample;
            sample.name = files[i].getFileNameWithoutExtension().toStdString();
            sample.sampleRate = reader->sampleRate;
            sample.rootNote = bank.note;
            sample.left.assign(buffer.getReadPointer(0), buffer.getReadPointer(0) + frames);
            if (buffer.getNumChannels() > 1)
                sample.right.assign(buffer.getReadPointer(1), buffer.getReadPointer(1) + frames);

            s1100::Keygroup kg;
            kg.sampleIndex = static_cast<int>(prog.samples.size());
            kg.lowNote = bank.note;
            kg.highNote = bank.note;
            kg.rootNote = bank.note;
            kg.lowVelocity = 1 + (i * 127) / n;
            kg.highVelocity = ((i + 1) * 127) / n;
            if (i == n - 1)
                kg.highVelocity = 127;
            kg.chokeGroup = bank.chokeGroup;
            kg.oneShot = true;

            prog.samples.push_back(std::move(sample));
            prog.keygroups.push_back(kg);
        }
    }

    return prog;
}
