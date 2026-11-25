#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/Widgets.h"

// The whole UI is laid out on a fixed 1600x900 design canvas (MainPanel) and
// scaled uniformly to the window, so knobs stay round and text scales too.
class MainPanel final : public juce::Component
{
public:
    static constexpr int designWidth = 1600, designHeight = 900;

    explicit MainPanel(MageAudioProcessor&);
    ~MainPanel() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh(); // called by the editor's timer (~30 Hz)

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

    std::unique_ptr<juce::ParameterAttachment> syncWatcher;
    std::unique_ptr<juce::FileChooser> irChooser;
    std::unique_ptr<juce::AlertWindow> saveDialog;

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
