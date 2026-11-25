#pragma once
#include <JuceHeader.h>
#include <array>

namespace mage
{
// Maps MIDI CC numbers (any channel) to plugin parameters.
//
// The audio thread calls process() with each block's MIDI; everything else runs on
// the message thread. The map is a table of atomics, so both sides can touch it
// without locks.
//
// Switches (bool or two-way choice parameters) treat CC >= 64 as on and < 64 as off,
// so a latching foot button toggles a pedal and a momentary one holds it on while
// pressed. Knobs follow the CC value across their full range.
class MidiLearn
{
public:
    explicit MidiLearn(juce::AudioProcessor&);

    // Audio thread.
    void process(const juce::MidiBuffer&) noexcept;

    // Message thread.
    void startLearning(const juce::String& paramID);
    void cancelLearning() noexcept { learning.store(-1); }
    juce::String getLearningParam() const;
    int getCCFor(const juce::String& paramID) const;  // -1 when not mapped
    void clear(const juce::String& paramID);
    void clearAll();
    bool hasAnyMapping() const;

    // Bumped whenever the map changes (including when a learn completes on the audio thread).
    int getChangeCount() const noexcept { return changes.load(); }
    // Set when a learn completes; the editor reads and clears it to flash the new mapping.
    std::atomic<int> lastLearnedParam { -1 };

    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree&);
    static constexpr const char* treeType = "MidiMap";

    // The user's map is also kept in ~/Documents/MAGE/MidiMap.xml so new instances start with it.
    void saveAsDefault() const;
    void loadDefault();

    juce::String getParamID(int index) const;
    int indexOf(const juce::String& paramID) const;

private:
    void apply(int paramIndex, int value) noexcept;

    juce::Array<juce::RangedAudioParameter*> params;
    juce::Array<bool> isSwitch;
    std::array<std::atomic<int>, 128> ccToParam;
    std::atomic<int> learning { -1 };
    std::atomic<int> changes { 0 };
};
} // namespace mage
