#pragma once

#include <cstdint>

namespace s1100 {

struct Sample;
struct Keygroup;

// One of the S1100's 16 voices.
//
// Plays a mapped sample with linear interpolation and a simple ADSR, or a
// placeholder sine when no sample is assigned so the MIDI path can be heard
// before a program is loaded.
//
// Real-time safe: no allocation, no locks.
class Voice {
public:
    void prepare(double sampleRate) noexcept;
    void start(int note, int velocity, std::uint64_t age, const Sample* sample,
               const Keygroup* keygroup) noexcept;
    void release() noexcept;
    void choke() noexcept; // fast fade; overrides one-shot (mute groups / retrigger)
    void kill() noexcept;

    bool isActive() const noexcept { return stage_ != Stage::Idle; }
    bool isHeld() const noexcept
    {
        return stage_ == Stage::Attack || stage_ == Stage::Decay || stage_ == Stage::Sustain;
    }
    bool isOneShot() const noexcept { return oneShot_; }
    int note() const noexcept { return note_; }
    int chokeGroup() const noexcept { return chokeGroup_; }
    std::uint64_t age() const noexcept { return age_; }

    // Mixes the next frame into left/right (additive).
    void render(float& left, float& right) noexcept;

    // Next mono sample (tests / fallback mix).
    float next() noexcept;

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    bool advanceEnvelope() noexcept;

    double sampleRate_ = 44100.0;
    double phase_ = 0.0;      // sine fallback, radians
    double phaseInc_ = 0.0;
    double position_ = 0.0;   // sample playback index
    double increment_ = 1.0;
    float level_ = 0.0f;
    float amp_ = 0.0f;
    float gainL_ = 1.0f;
    float gainR_ = 1.0f;
    float attackRate_ = 0.0f;
    float decayRate_ = 0.0f;
    float releaseRate_ = 0.0f;
    int note_ = -1;
    int chokeGroup_ = 0;
    std::uint64_t age_ = 0;
    Stage stage_ = Stage::Idle;
    bool oneShot_ = false;
    const Sample* sample_ = nullptr;
};

} // namespace s1100
