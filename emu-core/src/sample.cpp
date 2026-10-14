#include "s1100/sample.h"

#include <algorithm>

namespace s1100 {

const Keygroup* Program::keygroupFor(int note, int velocity) const noexcept
{
    const int vel = std::clamp(velocity, 1, 127);
    for (const auto& kg : keygroups) {
        if (note >= kg.lowNote && note <= kg.highNote && vel >= kg.lowVelocity
            && vel <= kg.highVelocity)
            return &kg;
    }
    return nullptr;
}

} // namespace s1100
