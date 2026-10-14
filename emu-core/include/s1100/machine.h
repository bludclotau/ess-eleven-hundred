#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>

#include "s1100/sample.h"
#include "s1100/voice.h"

namespace s1100 {

inline constexpr int kNumVoices = 16;

// The emulated sampler. MIDI and render run on the audio thread. setProgram
// may be called from another thread: it writes the inactive program slot and
// the audio thread applies the swap (killing voices so sample pointers never
// dangle).
//
// This is the high-level emulation (HLE) machine. A low-level path (68000 core,
// memory map, device models running the real firmware) can later sit behind the
// same interface.
class Machine {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    // Queue a program on the inactive slot. Applied on the next MIDI or render
    // call. Safe to call while audio is running.
    void setProgram(Program program);
    const Program& program() const noexcept;

    // Raw MIDI, as received. Channel is ignored for now (omni).
    void midiIn(std::uint8_t status, std::uint8_t data1, std::uint8_t data2) noexcept;

    void noteOn(int channel, int note, int velocity) noexcept;
    void noteOff(int channel, int note) noexcept;
    void allNotesOff() noexcept;
    void allSoundOff() noexcept;

    // Renders numSamples of audio. `right` may be null for a mono mix.
    void render(float* left, float* right, int numSamples) noexcept;

    int activeVoices() const noexcept;
    int activeVoicesSnapshot() const noexcept { return activeVoices_.load(std::memory_order_relaxed); }

private:
    void applyPendingProgram() noexcept;
    const Program& liveProgram() const noexcept;
    const Sample* sampleFor(const Keygroup* kg) const noexcept;
    Voice* allocateVoice() noexcept;

    std::array<Voice, kNumVoices> voices_;
    std::array<Program, 2> programs_{};
    std::atomic<int> liveIndex_{0};
    int pendingIndex_{1};
    std::atomic<bool> pendingSwap_{false};
    std::mutex swapMutex_;
    std::uint64_t ageCounter_ = 0;
    std::atomic<int> activeVoices_{0};
};

} // namespace s1100
