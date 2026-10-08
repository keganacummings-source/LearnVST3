# BBGVST — Level One Study Edition

This is the same BBGVST project rewritten as a **study project**. The goal is not to make the code clever. The goal is to make every important piece explain itself.

## The five things to learn first

1. **CMakeLists.txt** — how source files become a plugin.
2. **PluginProcessor.h** — what the audio engine owns.
3. **PluginProcessor.cpp** — how MIDI becomes sound and sound goes through FX.
4. **FxRackDsp.h** — how one sample is changed by an effect.
5. **PluginEditor.cpp** — how JUCE puts controls on the screen.

Then study the catalog files, Identity.h, Themes.h, and CustomLookAndFeel.h.

## The entire signal path

```text
DAW
 │
 ├── audio input ───────────────┐
 │                              ▼
 └── MIDI note ──> tiny synth ─> 16-slot FX rack ─> output
                                      │
                                      └── meters -> UI
```

## The beginner rule

When you see something confusing, find the smallest function that contains it. Do not read 500 lines at once.

For example, to understand delay: search for `case 3` in `FxRackDsp.h`.

## How to add one machine

Open `Source/FxCatalog.h` and add one line:

```cpp
{"N++", "Dumpster Laser", 10},
```

That is enough to put a new named machine into the browser. The hash/identity system gives it a repeatable personality.

## How to add one theme

Open `Source/Themes.h` and add one line:

```cpp
BB_THEME("Cyber Mint", 0xff07100d, 0xff102018, 0xff62ffb0, 0xffedfff5)
```

## Important real-time audio rule

`processBlock()` runs while audio is playing. Do not casually put network requests, file loading, sleeps, or expensive allocations there. Prepare memory in `prepareToPlay()` instead.

## What the comments mean

- `WHY:` explains the reason for a design choice.
- `STEP:` explains the order of an operation.
- `BEGINNER:` marks a useful learning shortcut.
- `IMPORTANT:` marks something that can break audio/build behavior.

Read the comments, then read the code directly underneath them.
