#pragma once
#include <JuceHeader.h>

namespace mage::ui
{
namespace colours
{
    const juce::Colour bg         { 0xff070911 };
    const juce::Colour face       { 0xff0a0d1b };
    const juce::Colour metal      { 0xff151d2b };
    const juce::Colour cyan       { 0xff59ddff };
    const juce::Colour blue       { 0xff458bff };
    const juce::Colour pink       { 0xffff70d5 };
    const juce::Colour violet     { 0xffb06bff };
    const juce::Colour ivory      { 0xffedf4ff };
    const juce::Colour muted      { 0xff91a3ba };
    const juce::Colour dim        { 0xff3a465a };
}

juce::Font font(float size, bool bold = true, float kerning = 0.0f);

// Shared drawing helpers used by the editor's background art.
void drawPanel(juce::Graphics&, juce::Rectangle<float>, juce::Colour accent, float radius);
void drawRune(juce::Graphics&, float x, float y, float size, juce::Colour);
void drawScrew(juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour accent);
void drawCrest(juce::Graphics&, float x, float y, float size);

class MageLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    MageLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos,
                          float start, float end, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    void drawComboBox(juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
};

// Rotary control with a caption underneath. While hovered or dragged the
// caption turns into the live value. Double-click resets to default.
class Knob final : public juce::Component
{
public:
    Knob(juce::AudioProcessorValueTreeState&, const juce::String& paramID, const juce::String& caption,
         juce::Colour accent = {});
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override  { repaint(); }
    juce::Slider& getSlider() { return slider; }

private:
    juce::Slider slider;
    juce::String caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Stomp switch with jewel LED, bound to a bool parameter (follows automation).
class Footswitch final : public juce::Component, public juce::SettableTooltipClient
{
public:
    Footswitch(juce::AudioProcessorValueTreeState&, const juce::String& paramID, juce::Colour accent);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    bool isOn() const { return on; }

private:
    juce::Colour accent;
    bool on = false, pressed = false;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

// LED-style toggle bound to a bool parameter.
class LedToggle final : public juce::ToggleButton
{
public:
    LedToggle(juce::AudioProcessorValueTreeState&, const juce::String& paramID, const juce::String& label,
              juce::Colour accent);
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

// Segmented peak meter (-60 dBFS .. +3 dBFS).
class LevelMeter final : public juce::Component
{
public:
    LevelMeter(bool isVertical, juce::Colour fromColour, juce::Colour toColour) : vertical(isVertical), from(fromColour), to(toColour) {}
    void setLevel(float linearPeak);
    void paint(juce::Graphics&) override;
private:
    bool vertical;
    juce::Colour from, to;
    float level = 0.0f, held = 0.0f;
    int holdFrames = 0;
};

// Small vector-icon button (no font glyph dependency).
class IconButton final : public juce::Button
{
public:
    enum class Icon { undo, redo, previous, next, save, trash, folder };
    IconButton(Icon icon, const juce::String& tooltip);
    void paintButton(juce::Graphics&, bool highlighted, bool down) override;
private:
    Icon icon;
};
} // namespace mage::ui
