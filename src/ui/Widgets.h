#pragma once
#include <JuceHeader.h>

namespace mage::ui
{
// The active palette. These are reassigned from the current Theme at the start of
// every paint call (see useThemeOf), so one plugin window can be Arcane while
// another is Fae. Always paint on the message thread.
namespace colours
{
    inline juce::Colour bg         { 0xff070911 };
    inline juce::Colour face       { 0xff0a0d1b };
    inline juce::Colour metal      { 0xff151d2b };
    inline juce::Colour cyan       { 0xff59ddff };   // primary   (Fae: moss)
    inline juce::Colour blue       { 0xff458bff };   // tertiary  (Fae: pale gold)
    inline juce::Colour pink       { 0xffff70d5 };   // secondary (Fae: rose)
    inline juce::Colour violet     { 0xffb06bff };   // fourth    (Fae: lilac)
    inline juce::Colour ivory      { 0xffedf4ff };
    inline juce::Colour muted      { 0xff91a3ba };
    inline juce::Colour dim        { 0xff3a465a };
}

struct Theme
{
    bool fae = false;
    juce::Colour bg, face, metal, cyan, blue, pink, violet, ivory, muted, dim;
    juce::Colour header, headerLine, etching, divider, panelShadow, panelEdgeDark, faceTop, faceBottom,
                 ledOff, track, knobHi, knobLo, knobRim, capHi, capLo, screen, control, popup,
                 screwHole, screwSlot, glow;
};
const Theme& arcaneTheme();
const Theme& faeTheme();
const Theme& currentTheme();
void useTheme(const Theme&);

// Implemented by the panel that owns a theme; children find it by walking up.
class ThemeHolder
{
public:
    virtual ~ThemeHolder() = default;
    virtual const Theme& getTheme() const = 0;
};
void useThemeOf(const juce::Component&);

juce::Font font(float size, bool bold = true, float kerning = 0.0f);

// Shared drawing helpers used by the editor's background art.
void drawPanel(juce::Graphics&, juce::Rectangle<float>, juce::Colour accent, float radius);
void drawRune(juce::Graphics&, float x, float y, float size, juce::Colour);
void drawScrew(juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour accent);

// The MAGE section runes (one per part of the amp), drawn on a 48-unit grid.
enum class Rune { gain, equalizer, presence, depth, power, gate, octave, drive, delay, reverb, cabinet, output,
                  shimmer, freeze, phaser, chorus, compressor };
// Full-colour rune in `area` (kept square). With `mono` set, every stroke uses that colour instead.
void drawSectionRune(juce::Graphics&, Rune, juce::Rectangle<float> area, float alpha = 1.0f,
                     juce::Colour mono = {});
// Centred title with its rune in front of it.
void drawRuneTitle(juce::Graphics&, Rune, const juce::String& text, juce::Rectangle<float> area,
                   const juce::Font&, juce::Colour textColour, float runeSize,
                   juce::Justification justification = juce::Justification::centred);

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
    void applyTheme(const Theme&);
};

// Right-clicks are left to the panel (MIDI Learn menu), so they never move a control.
class RotarySlider final : public juce::Slider
{
public:
    void mouseDown(const juce::MouseEvent& e) override        { if (!e.mods.isPopupMenu()) Slider::mouseDown(e); }
    void mouseDrag(const juce::MouseEvent& e) override        { if (!e.mods.isPopupMenu()) Slider::mouseDrag(e); }
    void mouseUp(const juce::MouseEvent& e) override          { if (!e.mods.isPopupMenu()) Slider::mouseUp(e); }
    void mouseDoubleClick(const juce::MouseEvent& e) override { if (!e.mods.isPopupMenu()) Slider::mouseDoubleClick(e); }
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
    void setAccent(juce::Colour);

private:
    RotarySlider slider;
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
    void setAccent(juce::Colour a) { accent = a; repaint(); }
    // When this returns true the switch is only on while held down.
    std::function<bool()> isMomentary;

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
    void setAccent(juce::Colour);
    void mouseDown(const juce::MouseEvent& e) override { if (!e.mods.isPopupMenu()) ToggleButton::mouseDown(e); }
    void mouseUp(const juce::MouseEvent& e) override   { if (!e.mods.isPopupMenu()) ToggleButton::mouseUp(e); }
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

// Segmented peak meter (-60 dBFS .. +3 dBFS).
class LevelMeter final : public juce::Component
{
public:
    LevelMeter(bool isVertical, juce::Colour fromColour, juce::Colour toColour) : vertical(isVertical), from(fromColour), to(toColour) {}
    void setLevel(float linearPeak);
    void setColours(juce::Colour fromColour, juce::Colour toColour) { from = fromColour; to = toColour; repaint(); }
    void paint(juce::Graphics&) override;
private:
    bool vertical;
    juce::Colour from, to;
    float level = 0.0f, held = 0.0f;
    int holdFrames = 0;
};

// Animated frieze for the top of the amp head. Arcane: a glowing ribbon carrying a
// slowly scrolling runic spell inscription, with a pulse of light reading along it.
// Fae: an entwined flowering vine with fireflies drifting around it.
class MagicFrieze final : public juce::Component, private juce::Timer
{
public:
    MagicFrieze();
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    void paintInscription(juce::Graphics&);
    void paintVine(juce::Graphics&);
    float phase = 0.0f;
};

// Two-position amp channel selector (Arcane / Fae), bound to a choice parameter.
class ChannelSwitch final : public juce::Component, public juce::SettableTooltipClient
{
public:
    ChannelSwitch(juce::AudioProcessorValueTreeState&, const juce::String& paramID);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
private:
    int channel = 0;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

// Marker in a gap between two pedal slots showing where the amp sits in the chain.
// The active one draws an AMP medallion; the others show a ghost on hover and move
// the amp there when clicked.
class AmpMarker final : public juce::Component, public juce::SettableTooltipClient
{
public:
    AmpMarker();
    void setActive(bool);
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override  { repaint(); }
    void mouseDown(const juce::MouseEvent&) override;
    std::function<void()> onClick;
private:
    bool active = false;
};

// Invisible click target over a pedal's title that opens the pedal picker.
class SlotTitle final : public juce::Component, public juce::SettableTooltipClient
{
public:
    SlotTitle();
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override  { repaint(); }
    void mouseDown(const juce::MouseEvent&) override  { if (onClick) onClick(); }
    std::function<void()> onClick;
    juce::Colour accent;
};

// The brush mark and wordmark, tinted with the theme's gradient at paint time.
enum class Logo { mark, wordmark };
void drawLogo(juce::Graphics&, Logo, juce::Rectangle<float> area);
float logoAspect(Logo);

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
