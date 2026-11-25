#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/Widgets.h"

// The whole UI is laid out on a fixed 1600x900 design canvas (MainPanel) and
// scaled uniformly to the window, so knobs stay round and text scales too.
class MainPanel final : public juce::Component, public mage::ui::ThemeHolder
{
public:
    static constexpr int designWidth = 1600, designHeight = 900;

    explicit MainPanel(MageAudioProcessor&);
    ~MainPanel() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh(); // called by the editor's timer (~30 Hz)
    void paintOverChildren(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;   // right-click on a control: MIDI Learn menu
    const mage::ui::Theme& getTheme() const override { return *theme; }

private:
    using Knob = mage::ui::Knob;
    using Footswitch = mage::ui::Footswitch;
    using LedToggle = mage::ui::LedToggle;
    using Icon = mage::ui::IconButton::Icon;

    class PresetBox final : public juce::ComboBox
    {
    public:
        std::function<void()> onAboutToShow;
        void showPopup() override { if (onAboutToShow) onAboutToShow(); juce::ComboBox::showPopup(); }
    };

    void rebuildPresetList();
    void savePresetDialog();
    void chooseCabinetIR();
    void updateDelayTimeVisibility(bool sync);
    void setChannel(bool fae);   // swaps the pedalboard and theme
    void layoutPedals();         // places each slot's pedal, hides the rest
    int boardSlot(int slot) const;   // pedal type in a slot of the current channel's board
    int ampPosition() const;
    void setBoardParam(const juce::String& paramID, int value);
    void showSlotMenu(int slot);
    void applyTheme();
    juce::Colour pedalAccent(int slot) const;
    void showMidiMenu(juce::Component& control, const juce::String& paramID);
    juce::Component* findLearnable(const juce::String& paramID) const;
    juce::Rectangle<int> midiBadgeArea(const juce::Component&) const;
    void updateMidiHighlight();

    MageAudioProcessor& processor;
    mage::ui::MageLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    // Header
    mage::ui::IconButton prevPreset { Icon::previous, "Previous preset" },
                         nextPreset { Icon::next, "Next preset" },
                         savePreset { Icon::save, "Save preset" },
                         deletePreset { Icon::trash, "Delete user preset" },
                         undoButton { Icon::undo, "Undo" },
                         redoButton { Icon::redo, "Redo" };
    PresetBox presetBox;
    mage::ui::LevelMeter inMeter { false, mage::ui::colours::blue, mage::ui::colours::cyan },
                         outMeter { false, mage::ui::colours::violet, mage::ui::colours::pink },
                         outMeterTall { true, mage::ui::colours::violet, mage::ui::colours::pink };

    // Amp
    mage::ui::MagicFrieze frieze;
    Knob gain, bass, mid, treble, presence, depth, master;
    // Gate
    Knob gateThreshold, gateRelease;
    LedToggle gateOn, gatePost;
    // Cabinet
    Knob lowCut, highCut;
    juce::TextButton loadIR { "LOAD IR" }, bypassIR { "BUILT-IN" };
    // Pedals
    Knob octaveMix, octaveTone, octaveLevel;
    LedToggle octaveDown;
    Knob drive, driveTone, driveLevel;
    LedToggle driveBoost;
    Knob delayTime, delayDivision, delayFeedback, delayMix;
    LedToggle delaySync;
    Knob reverbDecay, reverbSize, reverbTone, reverbMix;
    Footswitch octaveSwitch, driveSwitch, delaySwitch, reverbSwitch;
    // Output
    Knob output;
    // Fae channel: amp switch and the pedals that replace octave and drive
    mage::ui::ChannelSwitch channelSwitch;
    Knob shimmerMix, shimmerDecay, shimmerAmount, shimmerTone;
    LedToggle shimmerFifth;
    Knob freezeMix, freezeFade, freezeTone;
    LedToggle freezeMomentary;
    Footswitch shimmerSwitch, freezeSwitch;
    // Phaser and chorus / vibrato
    Knob phaserRate, phaserDepth, phaserFeedback, phaserMix;
    LedToggle phaserStages;
    Knob chorusRate, chorusDepth, chorusTone, chorusMix;
    LedToggle chorusVibrato;
    Footswitch phaserSwitch, chorusSwitch;
    // Compressor (input column)
    Knob compSustain, compLevel;
    LedToggle compOn, compParallel;

    // Pedalboard: the controls belonging to each pedal type, the clickable titles and
    // the amp markers in the gaps between slots.
    struct PedalView
    {
        std::vector<Knob*> knobs;
        LedToggle* toggle = nullptr;
        Footswitch* footswitch = nullptr;
    };
    std::array<PedalView, mage::numPedals> pedalViews;
    std::array<mage::ui::SlotTitle, mage::numSlots> slotTitles;
    std::array<mage::ui::AmpMarker, mage::numSlots + 1> ampMarkers;
    std::vector<std::unique_ptr<juce::ParameterAttachment>> boardWatchers;

    const mage::ui::Theme* theme = &mage::ui::arcaneTheme();
    bool faeChannel = false;
    std::unique_ptr<juce::ParameterAttachment> channelWatcher;

    std::unique_ptr<juce::ParameterAttachment> syncWatcher;
    std::unique_ptr<juce::FileChooser> irChooser;
    std::unique_ptr<juce::AlertWindow> saveDialog;

    // MIDI Learn: controls that can be mapped, and the highlight state.
    std::vector<std::pair<juce::Component*, juce::String>> learnables;
    juce::String highlightParam, flashParam;
    juce::Rectangle<int> highlightArea;
    int flashFrames = 0;

    juce::String irDisplayName, presetDisplayName;
    bool gateLed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};

class MageAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MageAudioProcessorEditor(MageAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MageAudioProcessor& processor;
    MainPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MageAudioProcessorEditor)
};
