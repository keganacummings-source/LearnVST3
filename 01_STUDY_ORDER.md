# Suggested Study Order

### Lesson 1 — Build system
Read `CMakeLists.txt`. Learn what `project`, `FetchContent`, `juce_add_plugin`, `target_sources`, and `target_link_libraries` mean.

### Lesson 2 — C++ class
Read `Source/PluginProcessor.h`. Identify public functions, private variables, and the processor object.

### Lesson 3 — Parameters
In `PluginProcessor.cpp`, study `createParameterLayout()`. Every parameter is a bridge between the UI/DAW and DSP.

### Lesson 4 — MIDI
Study the `midiMessages` loop in `processBlock()`. Find note-on, note-off, voice selection, oscillator phase, and envelope.

### Lesson 5 — One effect
Open `FxRackDsp.h` and learn only `case 0` (Drive/Fuzz). Then try `case 3` (Delay/Echo).

### Lesson 6 — Catalogs
Open `FxCatalog.h` and `InstrumentCatalog.h`. These are data tables, not giant effect implementations.

### Lesson 7 — UI
Read `PluginEditor.h`, then the constructor and `resized()` in `PluginEditor.cpp`.

### Lesson 8 — Visuals
Read `CustomLookAndFeel.h`, then `Identity.h` and `Themes.h`.

### Lesson 9 — State
Study `getStateInformation()` and `setStateInformation()`. This is how a DAW saves/restores the plugin.

### Lesson 10 — Change something
Add one catalog entry. Build. Then add one theme. Build again. Only after that start changing DSP.
