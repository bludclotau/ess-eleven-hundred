#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "s1100/machine.h"

static int failures = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FAIL: %s (line %d)\n", #cond, __LINE__);        \
            ++failures;                                                  \
        }                                                                \
    } while (0)

static constexpr int kBlock = 4410; // 0.1 s at 44.1 kHz

static float renderBlocks(s1100::Machine& m, int blocks, bool& finite)
{
    std::vector<float> l(kBlock), r(kBlock);
    float peak = 0.0f;
    for (int b = 0; b < blocks; ++b) {
        m.render(l.data(), r.data(), kBlock);
        for (int i = 0; i < kBlock; ++i) {
            if (!std::isfinite(l[i]) || !std::isfinite(r[i]))
                finite = false;
            peak = std::max(peak, std::max(std::fabs(l[i]), std::fabs(r[i])));
        }
    }
    return peak;
}

static s1100::Sample makeDc(int n, float level, int root = 60)
{
    s1100::Sample s;
    s.name = "dc";
    s.sampleRate = 44100.0;
    s.rootNote = root;
    s.left.assign(static_cast<std::size_t>(n), level);
    return s;
}

static s1100::Keygroup makeKg(int sampleIndex, int note, bool oneShot, int chokeGroup = 0,
                              int lowVel = 1, int highVel = 127)
{
    s1100::Keygroup kg;
    kg.sampleIndex = sampleIndex;
    kg.lowNote = note;
    kg.highNote = note;
    kg.rootNote = note;
    kg.lowVelocity = lowVel;
    kg.highVelocity = highVel;
    kg.oneShot = oneShot;
    kg.chokeGroup = chokeGroup;
    return kg;
}

int main()
{
    s1100::Machine m;
    m.prepare(44100.0);
    bool finite = true;

    // A note makes sound, and the sound is sane.
    m.midiIn(0x90, 60, 100);
    const float peak = renderBlocks(m, 1, finite);
    CHECK(finite);
    CHECK(peak > 0.05f);
    CHECK(peak <= 1.0f);
    CHECK(m.activeVoices() == 1);

    // Note off releases the sine fallback and the voice frees itself.
    m.midiIn(0x80, 60, 0);
    renderBlocks(m, 20, finite);
    CHECK(m.activeVoices() == 0);

    // Playing more notes than voices steals instead of overflowing.
    for (int n = 0; n < 24; ++n)
        m.noteOn(0, 40 + n, 100);
    CHECK(m.activeVoices() == s1100::kNumVoices);
    renderBlocks(m, 1, finite);

    // CC 123 (all notes off) releases everything (sines are not one-shot).
    m.midiIn(0xB0, 123, 0);
    renderBlocks(m, 20, finite);
    CHECK(m.activeVoices() == 0);

    // Note-on with velocity 0 acts as note-off.
    m.midiIn(0x90, 60, 100);
    m.midiIn(0x90, 60, 0);
    renderBlocks(m, 20, finite);
    CHECK(m.activeVoices() == 0);

    // Mono render (no right channel) is allowed.
    std::vector<float> mono(kBlock);
    m.midiIn(0x90, 64, 100);
    m.render(mono.data(), nullptr, kBlock);
    m.allSoundOff();

    // Velocity zones pick the matching keygroup.
    {
        s1100::Program prog;
        prog.samples.push_back(makeDc(8, 1.0f));
        prog.samples.push_back(makeDc(8, 1.0f));
        prog.keygroups.push_back(makeKg(0, 36, true, 0, 1, 63));
        prog.keygroups.push_back(makeKg(1, 36, true, 0, 64, 127));
        CHECK(prog.keygroupFor(36, 1)->sampleIndex == 0);
        CHECK(prog.keygroupFor(36, 63)->sampleIndex == 0);
        CHECK(prog.keygroupFor(36, 64)->sampleIndex == 1);
        CHECK(prog.keygroupFor(36, 127)->sampleIndex == 1);
        CHECK(prog.keygroupFor(37, 127) == nullptr);
    }

    // One-shot sample: note-off does not cut it short.
    {
        s1100::Program prog;
        prog.name = "oneshot";
        prog.samples.push_back(makeDc(44100, 0.8f)); // 1 s
        prog.keygroups.push_back(makeKg(0, 60, true));
        m.setProgram(std::move(prog));
        m.midiIn(0x90, 60, 127);
        renderBlocks(m, 1, finite);
        m.midiIn(0x80, 60, 0);
        renderBlocks(m, 2, finite);
        CHECK(m.activeVoices() == 1);
        renderBlocks(m, 10, finite);
        CHECK(m.activeVoices() == 0);
    }

    // A full-scale hit is loud; 16 stacked hits stay on the rail.
    {
        s1100::Program prog;
        prog.name = "levels";
        for (int n = 0; n < 16; ++n) {
            prog.samples.push_back(makeDc(2048, 1.0f, 40 + n));
            prog.keygroups.push_back(makeKg(n, 40 + n, true));
        }
        m.setProgram(std::move(prog));
        m.midiIn(0x90, 40, 127);
        const float one = renderBlocks(m, 1, finite);
        CHECK(finite);
        CHECK(one > 0.5f);
        CHECK(one <= 1.0f);
        m.allSoundOff();
        for (int n = 0; n < 16; ++n)
            m.noteOn(0, 40 + n, 127);
        const float stacked = renderBlocks(m, 1, finite);
        CHECK(finite);
        CHECK(stacked <= 1.0f);
        m.allSoundOff();
    }

    // Same-note retrigger does not layer.
    {
        s1100::Program prog;
        prog.samples.push_back(makeDc(44100, 0.5f));
        prog.keygroups.push_back(makeKg(0, 60, true));
        m.setProgram(std::move(prog));
        m.noteOn(0, 60, 100);
        m.noteOn(0, 60, 100);
        renderBlocks(m, 1, finite);
        CHECK(m.activeVoices() == 1);
        m.allSoundOff();
    }

    // Open/closed hats share a choke group.
    {
        s1100::Program prog;
        prog.samples.push_back(makeDc(44100, 0.5f));
        prog.samples.push_back(makeDc(44100, 0.5f));
        prog.keygroups.push_back(makeKg(0, 42, true, 1));
        prog.keygroups.push_back(makeKg(1, 46, true, 1));
        m.setProgram(std::move(prog));
        m.noteOn(0, 42, 100);
        m.noteOn(0, 46, 100);
        renderBlocks(m, 1, finite);
        CHECK(m.activeVoices() == 1);
        m.allSoundOff();
    }

    // Swapping programs while a voice is sounding is safe (pointers die with the swap).
    {
        s1100::Program a;
        a.name = "a";
        a.samples.push_back(makeDc(44100, 0.8f));
        a.keygroups.push_back(makeKg(0, 60, true));
        m.setProgram(std::move(a));
        m.noteOn(0, 60, 127);
        renderBlocks(m, 1, finite);
        CHECK(m.activeVoices() == 1);

        s1100::Program b;
        b.name = "b";
        b.samples.push_back(makeDc(64, 0.4f));
        b.keygroups.push_back(makeKg(0, 61, true));
        m.setProgram(std::move(b));
        renderBlocks(m, 1, finite);
        CHECK(m.activeVoices() == 0);
        m.noteOn(0, 61, 127);
        const float after = renderBlocks(m, 1, finite);
        CHECK(after > 0.01f);
        CHECK(m.program().name == "b");
        m.allSoundOff();
    }

    CHECK(finite);

    if (failures == 0)
        std::printf("emu-core smoke test: OK\n");
    return failures == 0 ? 0 : 1;
}
