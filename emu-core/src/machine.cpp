#include "s1100/machine.h"

#include <cmath>

namespace s1100 {

namespace {
// Instant-attack peak limiter: unity below 0 dBFS, never exceeds the rail.
float limitPeak(float x) noexcept
{
    const float a = std::fabs(x);
    if (a <= 1.0f)
        return x;
    return std::copysign(1.0f, x);
}
} // namespace

void Machine::prepare(double sampleRate) noexcept
{
    applyPendingProgram();
    for (auto& v : voices_)
        v.prepare(sampleRate);
}

void Machine::reset() noexcept
{
    applyPendingProgram();
    allSoundOff();
    ageCounter_ = 0;
}

void Machine::setProgram(Program program)
{
    std::lock_guard<std::mutex> lock(swapMutex_);
    const int live = liveIndex_.load(std::memory_order_relaxed);
    const int dest = pendingSwap_.load(std::memory_order_relaxed) ? pendingIndex_ : (1 - live);
    programs_[static_cast<std::size_t>(dest)] = std::move(program);
    pendingIndex_ = dest;
    pendingSwap_.store(true, std::memory_order_release);
}

const Program& Machine::program() const noexcept
{
    return programs_[static_cast<std::size_t>(liveIndex_.load(std::memory_order_relaxed))];
}

const Program& Machine::liveProgram() const noexcept
{
    return programs_[static_cast<std::size_t>(liveIndex_.load(std::memory_order_relaxed))];
}

void Machine::applyPendingProgram() noexcept
{
    if (!pendingSwap_.load(std::memory_order_acquire))
        return;
    std::unique_lock<std::mutex> lock(swapMutex_, std::try_to_lock);
    if (!lock.owns_lock())
        return;
    if (!pendingSwap_.load(std::memory_order_relaxed))
        return;
    allSoundOff();
    liveIndex_.store(pendingIndex_, std::memory_order_release);
    pendingSwap_.store(false, std::memory_order_release);
}

const Sample* Machine::sampleFor(const Keygroup* kg) const noexcept
{
    if (kg == nullptr || kg->sampleIndex < 0)
        return nullptr;
    const auto idx = static_cast<std::size_t>(kg->sampleIndex);
    const auto& prog = liveProgram();
    if (idx >= prog.samples.size())
        return nullptr;
    return &prog.samples[idx];
}

Voice* Machine::allocateVoice() noexcept
{
    Voice* target = nullptr;
    for (auto& v : voices_) {
        if (!v.isActive()) {
            target = &v;
            break;
        }
    }
    if (target == nullptr) {
        target = &voices_[0];
        for (auto& v : voices_) {
            if (v.age() < target->age())
                target = &v;
        }
    }
    return target;
}

void Machine::midiIn(std::uint8_t status, std::uint8_t data1, std::uint8_t data2) noexcept
{
    applyPendingProgram();
    const int type = status & 0xF0;
    const int channel = status & 0x0F;

    switch (type) {
    case 0x80:
        noteOff(channel, data1);
        break;
    case 0x90:
        noteOn(channel, data1, data2); // velocity 0 is handled as note-off
        break;
    case 0xB0:
        if (data1 == 120)
            allSoundOff();
        else if (data1 == 123)
            allNotesOff();
        break;
    default:
        break;
    }
}

void Machine::noteOn(int channel, int note, int velocity) noexcept
{
    applyPendingProgram();
    if (velocity == 0) {
        noteOff(channel, note);
        return;
    }

    const Keygroup* kg = liveProgram().keygroupFor(note, velocity);

    // Same-note retrigger: cut the previous hit instead of stacking voices.
    for (auto& v : voices_) {
        if (v.isActive() && v.note() == note)
            v.choke();
    }

    // Mute groups (open/closed hats, etc.).
    if (kg != nullptr && kg->chokeGroup > 0) {
        for (auto& v : voices_) {
            if (v.isActive() && v.chokeGroup() == kg->chokeGroup)
                v.choke();
        }
    }

    allocateVoice()->start(note, velocity, ++ageCounter_, sampleFor(kg), kg);
}

void Machine::noteOff(int /*channel*/, int note) noexcept
{
    applyPendingProgram();
    for (auto& v : voices_) {
        if (v.isHeld() && v.note() == note)
            v.release();
    }
}

void Machine::allNotesOff() noexcept
{
    applyPendingProgram();
    for (auto& v : voices_)
        v.release();
}

void Machine::allSoundOff() noexcept
{
    for (auto& v : voices_)
        v.kill();
    activeVoices_.store(0, std::memory_order_relaxed);
}

void Machine::render(float* left, float* right, int numSamples) noexcept
{
    applyPendingProgram();
    for (int i = 0; i < numSamples; ++i) {
        float mixL = 0.0f;
        float mixR = 0.0f;
        for (auto& v : voices_) {
            if (v.isActive())
                v.render(mixL, mixR);
        }
        mixL = limitPeak(mixL);
        mixR = limitPeak(mixR);
        left[i] = mixL;
        if (right != nullptr)
            right[i] = mixR;
    }
    activeVoices_.store(activeVoices(), std::memory_order_relaxed);
}

int Machine::activeVoices() const noexcept
{
    int count = 0;
    for (const auto& v : voices_) {
        if (v.isActive())
            ++count;
    }
    return count;
}

} // namespace s1100
