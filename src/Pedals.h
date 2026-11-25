#pragma once
#include <JuceHeader.h>

// The pedal library and the pedalboard layout shared by the processor and the editor.
//
// Each amp channel has its own board: four slots, each holding one pedal from the
// library (no duplicates), plus the amp's position among them (0..4). Pedals left of
// the amp feed its input; pedals right of it run after the amp and cabinet, like an
// effects loop. Left to right is the signal path.
namespace mage
{
enum class Pedal { octave, drive, shimmer, freeze, phaser, chorus, delay, reverb };
constexpr int numPedals = 8;
constexpr int numSlots = 4;

inline const juce::StringArray& pedalNames()
{
    static const juce::StringArray names { "Octave", "Drive", "Shimmer", "Freeze", "Phaser", "Chorus / Vibrato",
                                           "Delay", "Reverb" };
    return names;
}

// Channel 0 = Arcane, 1 = Fae.
inline juce::String slotParamID(int channel, int slot)
{
    return juce::String(channel == 0 ? "arcane" : "fae") + "Slot" + juce::String(slot + 1);
}
inline juce::String ampPosParamID(int channel) { return channel == 0 ? "arcaneAmpPos" : "faeAmpPos"; }

// Today's boards: Arcane = octave, drive | amp | delay, reverb; Fae = amp | shimmer, freeze, delay, reverb.
constexpr int defaultBoard[2][numSlots] = { { 0, 1, 6, 7 }, { 2, 3, 6, 7 } };
constexpr int defaultAmpPos[2] = { 2, 0 };
} // namespace mage
