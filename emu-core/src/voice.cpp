#include "s1100/voice.h"
#include "s1100/sample.h"

#include <algorithm>
#include <cmath>

namespace s1100 {

namespace {
constexpr double kTwoPi = 6.283185307179586;

// Placeholder envelope times, in seconds.
constexpr float kAttack = 0.003f;
constexpr float kDecay = 0.080f;
constexpr float kSustain = 1.000f;
constexpr float kRelease = 0.120f;
constexpr float kChoke = 0.005f;

float interpolate(const std::vector<float>& data, double pos) noexcept
{
    const int n = static_cast<int>(data.size());
    if (n <= 0 || pos < 0.0)
        return 0.0f;
    if (pos >= static_cast<double>(n - 1))
        return data[n - 1];
    const int i = static_cast<int>(pos);
    const float frac = static_cast<float>(pos - static_cast<double>(i));
    return data[static_cast<std::size_t>(i)]
         + frac * (data[static_cast<std::size_t>(i + 1)] - data[static_cast<std::size_t>(i)]);
}

// Balance pan so a centred hit is unity on both channels (0 dBFS for a
// full-scale sample). Equal-power would sit at -3 dB and stack on the old
// mix headroom to make the kit sound quiet.
void balanceGains(float pan, float volume, float& gainL, float& gainR) noexcept
{
    const float p = std::clamp(pan, -1.0f, 1.0f);
    const float v = std::max(0.0f, volume);
    gainL = v * std::min(1.0f, 1.0f - p);
    gainR = v * std::min(1.0f, 1.0f + p);
}
} // namespace

void Voice::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
    kill();
}

void Voice::start(int note, int velocity, std::uint64_t age, const Sample* sample,
                  const Keygroup* keygroup) noexcept
{
    const auto sr = static_cast<float>(sampleRate_);
    note_ = note;
    age_ = age;
    amp_ = static_cast<float>(std::clamp(velocity, 0, 127)) / 127.0f;
    phase_ = 0.0;
    position_ = 0.0;
    level_ = 0.0f;
    sample_ = (sample != nullptr && !sample->empty()) ? sample : nullptr;
    oneShot_ = keygroup != nullptr && keygroup->oneShot && sample_ != nullptr;
    chokeGroup_ = keygroup != nullptr ? keygroup->chokeGroup : 0;

    attackRate_ = 1.0f / (kAttack * sr);
    decayRate_ = (1.0f - kSustain) / (kDecay * sr);

    const int root = keygroup != nullptr ? keygroup->rootNote
                                         : (sample_ != nullptr ? sample_->rootNote : note);
    const double pitchRatio = std::pow(2.0, (note - root) / 12.0);

    if (sample_ != nullptr) {
        increment_ = pitchRatio * (sample_->sampleRate / sampleRate_);
    } else {
        const double freq = 440.0 * std::pow(2.0, (note - 69) / 12.0);
        phaseInc_ = kTwoPi * freq / sampleRate_;
        increment_ = 0.0;
    }

    const float pan = keygroup != nullptr ? keygroup->pan : 0.0f;
    const float vol = keygroup != nullptr ? keygroup->volume : 1.0f;
    balanceGains(pan, vol, gainL_, gainR_);

    stage_ = Stage::Attack;
}

void Voice::release() noexcept
{
    if (!isHeld() || oneShot_)
        return;
    releaseRate_ = std::max(level_, 1.0e-4f) / (kRelease * static_cast<float>(sampleRate_));
    stage_ = Stage::Release;
}

void Voice::choke() noexcept
{
    if (!isActive())
        return;
    oneShot_ = false;
    releaseRate_ = std::max(level_, 1.0e-4f) / (kChoke * static_cast<float>(sampleRate_));
    stage_ = Stage::Release;
}

void Voice::kill() noexcept
{
    stage_ = Stage::Idle;
    level_ = 0.0f;
    note_ = -1;
    chokeGroup_ = 0;
    oneShot_ = false;
    sample_ = nullptr;
    position_ = 0.0;
}

bool Voice::advanceEnvelope() noexcept
{
    switch (stage_) {
    case Stage::Idle:
        return false;
    case Stage::Attack:
        level_ += attackRate_;
        if (level_ >= 1.0f) {
            level_ = 1.0f;
            stage_ = Stage::Decay;
        }
        return true;
    case Stage::Decay:
        level_ -= decayRate_;
        if (level_ <= kSustain) {
            level_ = kSustain;
            stage_ = Stage::Sustain;
        }
        return true;
    case Stage::Sustain:
        return true;
    case Stage::Release:
        level_ -= releaseRate_;
        if (level_ <= 0.0f) {
            kill();
            return false;
        }
        return true;
    }
    return false;
}

void Voice::render(float& left, float& right) noexcept
{
    if (!advanceEnvelope())
        return;

    float sL = 0.0f;
    float sR = 0.0f;
    if (sample_ != nullptr) {
        const int n = sample_->length();
        if (n <= 0 || position_ >= static_cast<double>(n)) {
            kill();
            return;
        }
        sL = interpolate(sample_->left, position_);
        sR = sample_->isStereo() ? interpolate(sample_->right, position_) : sL;
        position_ += increment_;
    } else {
        sL = static_cast<float>(std::sin(phase_));
        sR = sL;
        phase_ += phaseInc_;
        if (phase_ >= kTwoPi)
            phase_ -= kTwoPi;
    }

    const float g = level_ * amp_;
    left += sL * g * gainL_;
    right += sR * g * gainR_;
}

float Voice::next() noexcept
{
    float l = 0.0f;
    float r = 0.0f;
    render(l, r);
    return 0.5f * (l + r);
}

} // namespace s1100
