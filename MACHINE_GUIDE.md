# BBGVST Machine Guide — Level 1 N++ Edition

This is the beginner system. **One machine = one line.**

## Add an FX machine

Open `Source/FxCatalog.h` and add one line inside `kFxCatalog`:

```cpp
{"N++", "Dumpster Laser", 10},
```

Format:

`{"BRAND", "MACHINE NAME", FAMILY},`

FX families:

```text
0 = DRIVE / FUZZ
1 = DYNAMICS / COMPRESSOR
2 = EQ / TONE
3 = DELAY / ECHO
4 = REVERB / SPACE
5 = MODULATION
6 = FILTER
7 = GATE / DE-ESS
8 = PITCH / DOUBLER
9 = LOOP / FREEZE
10 = EXPERIMENTAL / CREATIVE
```

## Add an instrument

Open `Source/InstrumentCatalog.h` and add one line:

```cpp
{"N++", "Dumpster Piano", InstrumentFamily::PianoKeys},
```

Families:

`Synth, Drum, Bass, PianoKeys, Organ, Guitar, Strings, Brass, Woodwind, Sampler, Texture`

## Rename

Change only the quoted name:

```cpp
{"N++", "Old Name", 0},
```

to:

```cpp
{"N++", "New Name", 0},
```

The name is part of the machine identity, so renaming intentionally gives it a new personality.

## Remove

Delete the one line. Nothing else.

## Why machines sound different

BBGVST hashes the brand + model into a stable identity. That identity changes transfer curves, modulation, timing, feedback, diffusion, stereo movement and other character values. Two machines can share a family while still behaving differently.

## Why machines look different

The same identity also chooses from multiple physical panel archetypes. The editor uses the identity to vary accent color and hardware markings. The goal is recognizable **fantasy hardware**, not a pixel-for-pixel copy of a commercial product.

## Themes — one line

Open `Source/Themes.h` and add:

```cpp
BB_THEME("Cyber Mint", 0xff07100d, 0xff102018, 0xff62ffb0, 0xffedfff5)
```

Order is:

`NAME, BACKGROUND, PANEL, ACCENT, TEXT`

Colors are `0xFFRRGGBB`.

## Beginner workflow

1. Add one line.
2. Save.
3. Push to GitHub.
4. Run Actions.
5. Download the artifact.
6. Test the VST3.
7. Repeat.

Do not touch the DSP for simple catalog additions.

## Level 2 — custom DSP

Only when you want a genuinely new processing family should you edit `Source/FxRackDsp.h`.

The catalog is intentionally simple; the engine is complicated underneath so a beginner does not have to manage it.

## Legal/design rule

Use manufacturer names only as reference targets. BBGVST should use original artwork, original drawing code, original audio algorithms and original assets. Do not add logos, photographs, factory samples, proprietary impulse responses or copied product artwork.
