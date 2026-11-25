# MAGE

A modern high-gain guitar amp simulator inspired by boutique metal amplifiers, with a rune-inspired blue/pink UI.
Built with JUCE 8 as a **VST3 plugin** and a **standalone app** (for playing at home without a DAW).

## Signal chain
```
Input (ch 1) → Noise Gate (pre or post) → Octave → [4x oversampled: Drive → 3-stage Preamp → Bass/Mid/Treble
→ Power amp (Master, Presence, Depth)] → Cabinet (IR or built-in 4x12) → Low/High Cut → Delay → Reverb → Output
```
- Guitar input is mono (input channel 1); the output is stereo (delay and reverb are stereo).
- With **Tempo Sync** on, the delay follows the host tempo and the TIME knob becomes a NOTE knob (1/16 … 1/2).
- **LOAD IR** accepts WAV/AIFF cabinet impulse responses; **BUILT-IN** switches back to the internal cab voicing.

## UI tips
- Hover a knob to see its value; double-click to reset; scroll wheel works too.
- The window can be resized from the corner — everything scales evenly.
- Undo/redo buttons in the header cover every knob and switch change.

## Presets
Six factory presets are built in. User presets are saved as `.magepreset` files in `~/Documents/MAGE/Presets`.

## Build locally
Install CMake and Xcode Command Line Tools, then:
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Automated macOS build
GitHub Actions builds the standalone app on pushes to `main` and on manual workflow runs (Actions → *Build MAGE macOS App* → *Run workflow*, any branch). The zipped app is published as a workflow artifact.

This project is an early prototype. The amp voicing needs listening tests with a real guitar.
