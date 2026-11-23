# MAGE

A modern high-gain guitar amp simulator inspired by boutique metal amplifiers, with a rune-inspired blue/pink UI.

## Build targets
- macOS standalone application
- VST3 plugin

## Build locally
Install CMake and Xcode Command Line Tools, then:
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Automated macOS build
GitHub Actions builds the standalone app on pushes to `main` and manual workflow runs. The zipped app is published as a workflow artifact.

This project is an early prototype. The amp DSP and effects require further development and listening tests.
