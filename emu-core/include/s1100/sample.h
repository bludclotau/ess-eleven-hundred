#pragma once

#include <string>
#include <vector>

namespace s1100 {

// PCM held by a Program. Voices hold a non-owning pointer into this data.
// Machine double-buffers programs so a live swap kills voices before the
// old buffer is reused.
struct Sample {
    std::string name;
    std::vector<float> left;
    std::vector<float> right; // empty => mono (left is copied to both)
    double sampleRate = 44100.0;
    int rootNote = 60;

    int length() const noexcept { return static_cast<int>(left.size()); }
    bool empty() const noexcept { return left.empty(); }
    bool isStereo() const noexcept { return right.size() == left.size() && !right.empty(); }
};

// One S1100-style keygroup: a sample mapped across a MIDI note and velocity range.
struct Keygroup {
    int sampleIndex = -1;
    int lowNote = 0;
    int highNote = 127;
    int rootNote = 60;
    int lowVelocity = 1;
    int highVelocity = 127;
    int chokeGroup = 0; // 0 = none; matching groups cut each other off
    bool oneShot = false; // ignore note-off; play until the sample ends
    float volume = 1.0f;
    float pan = 0.0f; // -1 left .. +1 right
};

// A loaded program: owned samples plus the keygroups that point at them.
struct Program {
    std::string name;
    std::vector<Sample> samples;
    std::vector<Keygroup> keygroups;

    const Keygroup* keygroupFor(int note, int velocity) const noexcept;
};

} // namespace s1100
