# Samples (not in git)

The default drum program expects the **wavbvkery – dreams** kit laid out as:

```
samples/
  808s/
  kicks/
  snares/
  claps/
  percs/
  cymbals/hats closed/
  cymbals/hats open/
  cymbals/crashes/
  cymbals/rides/
```

Those WAVs are third-party and are not part of this repository. The AGPL
licence covers the emulator code only.

On this machine the pack lives on the NAS:

```sh
rsync -a --exclude='.DS_Store' /mnt/lemon/drum_kit/ samples/
```

Ten files per folder become velocity layers on one GM drum key (see the root
README). Hats share a mute group so open and closed cut each other off.
