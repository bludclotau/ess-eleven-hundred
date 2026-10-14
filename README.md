# ESS Eleven Hundred

Akai S1100-style sampler. A standalone JUCE app with the emulator kept in a
separate library (`emu-core`) so it can be embedded in a larger audio suite later.

## Status

High-level emulation (HLE). 16 voices, MIDI, linear-interpolated sample playback,
one-shot keygroups, velocity layers, mute groups, a live program swap, and a
peak limiter. Unmapped notes fall back to a sine so the MIDI path can be heard
without a kit. No firmware / 68000 emulation yet.

## Layout

| Path        | What it is |
|-------------|------------|
| `emu-core/` | Static library, no UI or OS dependencies. `Machine` owns the voices and takes raw MIDI. Real-time safe (no allocation or locks while rendering). |
| `app/`      | JUCE standalone app: audio out, MIDI in (hardware ports plus a virtual port named "S1100 Emulator"), on-screen keyboard, WAV program loader. |
| `samples/`  | Local drum kit (not shipped). See `samples/README.md`. |

## Default kit (GM, 88-key)

Copy the Dreams pack into `samples/` yourself — it is not in this repo.

Ten variations of each drum are **velocity layers on one key**, so the map fits
an 88-key controller (A0=21 .. C8=108). Hits are one-shots: note-off does not
cut a crash or 808 short. Open and closed hats share mute group 1.

| Note | Key | Bank |
|------|-----|------|
| 32 | B0 | 808s |
| 36 | C1 | kicks |
| 37 | C#1 | percs |
| 38 | D1 | snares |
| 39 | D#1 | claps |
| 42 | F#1 | closed hats |
| 46 | A#1 | open hats |
| 49 | C#2 | crashes |
| 51 | D#2 | rides |

Retriggering the same note cuts the previous voice instead of stacking. The
output stage peak-limits at 0 dBFS so a full-scale hit is loud and 16 voices
do not wrap.

## Build (Fedora)

```sh
sudo dnf install cmake gcc-c++ git alsa-lib-devel freetype-devel fontconfig-devel \
    mesa-libGL-devel libX11-devel libXext-devel libXrandr-devel libXinerama-devel \
    libXcursor-devel libXcomposite-devel libXrender-devel

rsync -a --exclude='.DS_Store' /mnt/lemon/drum_kit/ samples/

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build            # core smoke test
./build/app/S1100Emu_artefacts/Release/ESS\ Eleven\ Hundred
```

Core only, without downloading JUCE: `cmake -B build -DS1100_BUILD_APP=OFF`.

## Roadmap

1. Sample playback voices (S1000/S1100 sample format, loops, interpolation)
2. Programs and keygroups, per-keygroup filter, envelopes, LFO, multiple outputs
3. Akai disk image loader
4. Low-level emulation skeleton: 68000 core, memory map, MIDI UART, front-panel LCD
5. Virtual front panel and debugger view

## Firmware ROM

The original ROM is copyrighted. It is never committed to this repo (see `.gitignore`);
the emulator will load a user-supplied dump from a path.

## License

JUCE's open-source option is AGPLv3, so the **code** is AGPLv3. See `LICENSE`.
That does not cover third-party sample packs. Do not commit WAVs you do not own
or whose licence forbids redistribution.
