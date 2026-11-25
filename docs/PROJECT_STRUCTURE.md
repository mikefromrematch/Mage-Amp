# Project structure

- `src/PluginProcessor.*` — parameters, signal chain, state save/restore, IR loading, meters
- `src/dsp/AmpDSP.h` — real-time-safe DSP blocks: gate, octave, drive, preamp, cab voicing, filters
- `src/PresetManager.*` — factory presets and user preset files
- `src/PluginEditor.*` — main window: fixed 1600×900 design canvas scaled uniformly to the window
- `src/ui/Widgets.*` — look-and-feel, knobs, footswitches, LED toggles, meters, icon buttons
- `.github/workflows/` — CI builds
- `CMakeLists.txt` — JUCE/CMake build
