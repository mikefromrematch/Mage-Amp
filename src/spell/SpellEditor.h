#pragma once
#include <JuceHeader.h>
#include "SpellProcessor.h"
#include "ui/Widgets.h"

namespace spellui
{
using APVTS = juce::AudioProcessorValueTreeState;

using ModePills = mage::ui::ChoicePills;

// The glitch pattern: 16 cells, click to cycle Off > Stutter > Reverse > Jump.
class StepGrid final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepGrid(APVTS&);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void setPlayStep(int step) { if (step != playStep) { playStep = step; repaint(); } }
    juce::Colour colours[4];
private:
    int cellAt(juce::Point<int>) const;
    std::array<int, 16> steps {};
    std::array<std::unique_ptr<juce::ParameterAttachment>, 16> attachments;
    int playStep = -1;
};

// Interval buttons that set the pitch shift.
class IntervalRow final : public juce::Component, public juce::SettableTooltipClient
{
public:
    IntervalRow(APVTS&, juce::Colour accent);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    static constexpr int intervals[7] = { -12, -7, -5, 3, 5, 7, 12 };
    juce::Colour accent;
    int current = 12;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

// Triangle morph pad: Glitch at the top, Pitch bottom-right, Swell bottom-left.
class MorphPad final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MorphPad(APVTS&);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    juce::Point<float> position() const { return { x, y }; }
    juce::Colour colours[3];
    std::function<void()> onMove;
private:
    void moveTo(juce::Point<float>);
    float x = 0.5f, y = 0.62f;
    std::unique_ptr<juce::ParameterAttachment> ax, ay;
};

// The big CAST button. Hold and Burst are momentary, Latch toggles.
class CastButton final : public juce::Component, public juce::SettableTooltipClient
{
public:
    CastButton(APVTS&);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void setCasting(bool c) { if (c != casting) { casting = c; repaint(); } }
    juce::Colour accent;
private:
    APVTS& state;
    bool on = false, casting = false;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

class Panel final : public juce::Component, public mage::ui::ThemeHolder
{
public:
    static constexpr int designWidth = 1280, designHeight = 760;
    explicit Panel(SpellAudioProcessor&);
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void refresh();
    void mouseDown(const juce::MouseEvent&) override;
    const mage::ui::Theme& getTheme() const override { return mage::ui::arcaneTheme(); }

private:
    using Knob = mage::ui::Knob;
    using LedToggle = mage::ui::LedToggle;

    class PresetBox final : public juce::ComboBox
    {
    public:
        std::function<void()> onAboutToShow;
        void showPopup() override { if (onAboutToShow) onAboutToShow(); juce::ComboBox::showPopup(); }
    };

    void rebuildPresetList();
    void savePresetDialog();
    void showMidiMenu(juce::Component&, const juce::String& paramID);
    void updateMidiHighlight();
    juce::Component* findLearnable(const juce::String& id) const;
    juce::Rectangle<int> midiBadgeArea(const juce::Component&) const;
    juce::Rectangle<float> spellArea(int index) const;

    SpellAudioProcessor& processor;

    // Glitch / Pitch / Swell
    LedToggle glitchOn, pitchOn, swellOn;
    ModePills glitchMode, pitchMode, swellMode;
    Knob glitchSlice, glitchDensity, glitchJump, glitchMix;
    Knob pitchBend, pitchGlide, pitchDetune, pitchMix;
    Knob swellAttack, swellBloom, swellSize, swellMix;
    StepGrid steps;
    IntervalRow intervals;

    MorphPad morph;
    CastButton cast;
    ModePills castMode;
    juce::TextButton reorder { "REORDER" };
    Knob mix, output;

    PresetBox presetBox;
    mage::ui::IconButton prevPreset { mage::ui::IconButton::Icon::previous, "Previous preset" };
    mage::ui::IconButton nextPreset { mage::ui::IconButton::Icon::next, "Next preset" };
    mage::ui::IconButton savePreset { mage::ui::IconButton::Icon::save, "Save preset" };
    std::unique_ptr<juce::AlertWindow> saveDialog;
    juce::String shownPreset;
    bool shownModified = false;

    std::unique_ptr<juce::ParameterAttachment> orderAttachment, castModeAttachment;
    int order = 0, castModeIndex = 0;
    float shownBpm = 0.0f;
    float lastOn[3] { -1.0f, -1.0f, -1.0f };

    std::vector<std::pair<juce::Component*, juce::String>> learnables;
    juce::String highlightParam, flashParam;
    juce::Rectangle<int> highlightArea;
    int flashFrames = 0;
};
} // namespace spellui

class SpellEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SpellEditor(SpellAudioProcessor&);
    ~SpellEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    SpellAudioProcessor& processor;
    mage::ui::MageLookAndFeel lookAndFeel;
    spellui::Panel panel;
};
