#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include "s1100/sample.h"

// Loads the Dreams drum kit under `samples/` into an S1100-style program.
// Each bank is one GM drum key with the ten WAVs as velocity layers. Hats
// share mute group 1. All hits are one-shots (note-off does not cut them).
s1100::Program loadDreamsKit(const juce::File& samplesDir);
