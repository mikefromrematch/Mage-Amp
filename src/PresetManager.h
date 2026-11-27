#pragma once
#include <JuceHeader.h>

namespace mage
{
// Factory presets live in code; user presets are XML files in
// ~/Documents/MAGE/Presets (*.magepreset) so they're easy to back up or share.
class PresetManager
{
public:
    struct Factory { juce::String name; std::vector<std::pair<juce::String, float>> values; };

    // The amp's presets (folder ~/Documents/MAGE/Presets).
    PresetManager(juce::AudioProcessorValueTreeState& state, juce::UndoManager& undo);
    // Another plugin in the suite: its own factory list and preset folder under ~/Documents/MAGE.
    PresetManager(juce::AudioProcessorValueTreeState& state, juce::UndoManager& undo,
                  std::vector<Factory> factoryPresets, const juce::String& folderName);

    juce::StringArray getPresetNames() const;      // factory first, then user
    int getNumFactoryPresets() const;
    int getCurrentIndex() const;                   // -1 if the current name is unknown
    juce::String getCurrentName() const;
    bool isModified() const;                       // edited since the preset was loaded

    void loadPreset(int index);
    void loadNext()     { step(1); }
    void loadPrevious() { step(-1); }
    bool saveUserPreset(const juce::String& name);
    bool deleteUserPreset(const juce::String& name);
    bool isFactory(const juce::String& name) const;

    static juce::File getUserPresetFolder();       // the amp's folder; ~/Documents/MAGE is its parent
    juce::File getFolder() const;                  // this instance's preset folder

private:
    void step(int delta);
    void applyValues(const std::function<float(juce::RangedAudioParameter&)>& valueFor);
    void setCurrentName(const juce::String& name);
    juce::Array<juce::File> getUserPresetFiles() const;
    juce::String snapshot() const;

    juce::AudioProcessorValueTreeState& state;
    juce::UndoManager& undoManager;
    std::vector<Factory> factory;
    juce::String folderName { "Presets" };
    juce::String loadedSnapshot;
};
} // namespace mage
