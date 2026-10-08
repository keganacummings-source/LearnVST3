# BBGVST — Level One Study Edition

This is the beginner-readable version of the BBGVST FX + Instrument workstation.

The project keeps the original working architecture: JUCE + CMake, 16 FX slots, a large FX catalog, an instrument catalog, MIDI input, saved parameters, and a custom JUCE interface. The difference is that the code is deliberately explained and formatted for learning.

## Build

### GitHub Actions
Push the repository to GitHub. `.github/workflows/build.yml` configures and builds the plugin.

### Windows
Run `build-windows.bat` on a machine with CMake and the required C++ build tools.

## Start learning

Open `00_READ_THIS_FIRST.md`, then follow `01_STUDY_ORDER.md`.

## Source map

- `PluginProcessor.*` — audio engine, MIDI, parameters, state.
- `FxRackDsp.h` — effects DSP.
- `FxCatalog.h` — named FX data.
- `InstrumentCatalog.h` — named instrument data.
- `PluginEditor.*` — user interface.
- `CustomLookAndFeel.h` — knob/control drawing.
- `Identity.h` — deterministic machine personality.
- `Themes.h` — color palettes.

## Design/legal note

The catalog uses manufacturer/product names as reference labels. The project should continue using original code, original artwork, original UI assets, and original audio processing rather than copying proprietary assets or factory samples.


## Build fixes in this revision

The first GitHub build exposed three beginner-level C++ mistakes in `PluginEditor.cpp`: JUCE `MouseEvent` cannot be default-constructed, the newer JUCE API does not provide `Random::nextIntInRange()`, and the macOS job was using a deployment target that caused JUCE 7.0.12 to hit an unavailable macOS 15 CoreGraphics API. These are fixed in the source and CMake file.
