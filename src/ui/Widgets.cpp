#include "Widgets.h"

namespace mage::ui
{
using namespace colours;

juce::Font font(float size, bool bold, float kerning)
{
    juce::Font f(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
    if (kerning > 0.0f)
        f.setExtraKerningFactor(kerning);
    return f;
}

static juce::Colour accentOf(const juce::Component& c, juce::Colour fallback)
{
    const auto v = c.getProperties()["accent"];
    return v.isVoid() ? fallback : juce::Colour(static_cast<juce::uint32>(static_cast<juce::int64>(v)));
}

// ---------------------------------------------------------------------------
void drawPanel(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent, float radius)
{
    g.setColour(juce::Colour(0xdd00030a));
    g.fillRoundedRectangle(r.translated(0.0f, 7.0f), radius);
    juce::ColourGradient edge(accent.withAlpha(0.9f), r.getX(), r.getY(),
                              juce::Colour(0xff15152a), r.getRight(), r.getBottom(), false);
    g.setGradientFill(edge);
    g.fillRoundedRectangle(r, radius);
    g.setColour(accent.withAlpha(0.95f));
    g.drawRoundedRectangle(r, radius, 2.0f);
    auto inner = r.reduced(5.0f);
    juce::ColourGradient faceGrad(juce::Colour(0xff0e1224), inner.getX(), inner.getY(),
                                  juce::Colour(0xff080a16), inner.getX(), inner.getBottom(), false);
    g.setGradientFill(faceGrad);
    g.fillRoundedRectangle(inner, radius - 4.0f);
    g.setColour(accent.withAlpha(0.20f));
    g.drawRoundedRectangle(r.reduced(8.0f), radius - 6.0f, 1.0f);
}

void drawRune(juce::Graphics& g, float x, float y, float size, juce::Colour colour)
{
    juce::Path rune;
    rune.startNewSubPath(x + size * 0.18f, y + size * 0.88f);
    rune.lineTo(x + size * 0.18f, y + size * 0.12f);
    rune.lineTo(x + size * 0.82f, y + size * 0.88f);
    rune.lineTo(x + size * 0.82f, y + size * 0.12f);
    rune.startNewSubPath(x + size * 0.18f, y + size * 0.50f);
    rune.lineTo(x + size * 0.82f, y + size * 0.50f);
    g.setColour(colour);
    g.strokePath(rune, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void drawScrew(juce::Graphics& g, juce::Point<float> p, float r, juce::Colour accent)
{
    g.setColour(juce::Colour(0xff02050b));
    g.fillEllipse(p.x - r, p.y - r, r * 2.0f, r * 2.0f);
    g.setColour(accent.withAlpha(0.6f));
    g.drawEllipse(p.x - r * 0.7f, p.y - r * 0.7f, r * 1.4f, r * 1.4f, 1.2f);
    g.setColour(juce::Colour(0xff73839a).withAlpha(0.7f));
    g.drawLine(p.x - r * 0.45f, p.y, p.x + r * 0.45f, p.y, 1.0f);
}

void drawCrest(juce::Graphics& g, float x, float y, float size)
{
    juce::Path frame;
    frame.startNewSubPath(x + size * 0.5f, y);
    frame.lineTo(x + size * 0.91f, y + size * 0.20f);
    frame.lineTo(x + size * 0.91f, y + size * 0.72f);
    frame.lineTo(x + size * 0.5f, y + size);
    frame.lineTo(x + size * 0.09f, y + size * 0.72f);
    frame.lineTo(x + size * 0.09f, y + size * 0.20f);
    frame.closeSubPath();

    juce::ColourGradient aura(cyan.withAlpha(0.28f), x, y, pink.withAlpha(0.10f), x + size, y + size, true);
    g.setGradientFill(aura);
    g.fillEllipse(x - size * 0.10f, y - size * 0.10f, size * 1.20f, size * 1.20f);
    g.setColour(cyan.withAlpha(0.9f));
    g.strokePath(frame, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path monogram;
    monogram.startNewSubPath(x + size * 0.24f, y + size * 0.73f);
    monogram.lineTo(x + size * 0.24f, y + size * 0.29f);
    monogram.lineTo(x + size * 0.50f, y + size * 0.57f);
    monogram.lineTo(x + size * 0.76f, y + size * 0.29f);
    monogram.lineTo(x + size * 0.76f, y + size * 0.73f);
    monogram.startNewSubPath(x + size * 0.35f, y + size * 0.82f);
    monogram.lineTo(x + size * 0.65f, y + size * 0.82f);
    juce::ColourGradient ink(cyan, x, y, pink, x + size, y + size, false);
    g.setGradientFill(ink);
    g.strokePath(monogram, juce::PathStrokeType(size * 0.055f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ---------------------------------------------------------------------------
namespace
{
    struct RuneElement
    {
        juce::Path path;
        juce::uint32 colour;
        float alpha = 1.0f;
        enum Kind { stroke, fill, strokeAndTint, strokeOverBackground } kind = stroke;
    };

    juce::String circle(float cx, float cy, float r)
    {
        return "M" + juce::String(cx + r) + " " + juce::String(cy)
             + " A" + juce::String(r) + " " + juce::String(r) + " 0 1 1 " + juce::String(cx - r) + " " + juce::String(cy)
             + " A" + juce::String(r) + " " + juce::String(r) + " 0 1 1 " + juce::String(cx + r) + " " + juce::String(cy) + " Z";
    }

    const std::vector<RuneElement>& runeElements(Rune rune)
    {
        constexpr juce::uint32 C = 0xff59ddff, P = 0xffff70d5, B = 0xff458bff, V = 0xffb06bff,
                               I = 0xffedf4ff, L = 0xff9a8bff, M = 0xff91a3ba;
        using K = RuneElement;
        auto e = [](const juce::String& d, juce::uint32 colour, K::Kind kind = K::stroke, float alpha = 1.0f)
        {
            return RuneElement { juce::Drawable::parseSVGPath(d), colour, alpha, kind };
        };

        static const std::array<std::vector<RuneElement>, 12> table {{
            // gain: flame
            { e("M24 5 L33 22 L28 24 L34 34 L24 43 L14 34 L20 24 L15 22 Z", C),
              e("M24 26 L28 33 L24 37 L20 33 Z", P, K::fill) },
            // equalizer: three staves
            { e("M13 8 L13 40 M24 8 L24 40 M35 8 L35 40", B),
              e(circle(13, 30, 4) + circle(24, 16, 4) + circle(35, 25, 4), C, K::strokeOverBackground) },
            // presence: seer's eye
            { e("M5 24 Q24 7 43 24 Q24 41 5 24 Z", C),
              e(circle(24, 24, 6), P),
              e(circle(24, 24, 2), I, K::fill) },
            // depth: sinking waves
            { e("M6 14 Q15 8 24 14 T42 14", C),
              e("M10 25 Q17 20 24 25 T38 25", L),
              e("M15 36 Q19.5 32 24 36 T33 36", P) },
            // power: sun
            { e(circle(24, 24, 8), P),
              e("M24 4 L24 10 M24 38 L24 44 M4 24 L10 24 M38 24 L44 24 M10 10 L14 14 M34 34 L38 38 M38 10 L34 14 M10 38 L14 34", C) },
            // gate: portcullis
            { e("M11 42 L11 20 A13 13 0 0 1 37 20 L37 42", C),
              e("M20 10 L20 42 M28 10 L28 42 M11 29 L37 29", C),
              e("M7 42 L41 42", P) },
            // octave: descent
            { e("M10 12 L24 22 L38 12", V),
              e("M10 25 L24 35 L38 25", V, K::stroke, 0.6f),
              e(circle(24, 42, 2.4f), P, K::fill) },
            // drive: bolt
            { e("M28 4 L13 27 L23 27 L19 44 L35 19 L25 19 Z", P, K::strokeAndTint) },
            // delay: echoes
            { e(circle(10, 24, 3.4f), C, K::fill),
              e("M17.07 16.93 A10 10 0 0 1 17.07 31.07", C),
              e("M22.73 11.27 A18 18 0 0 1 22.73 36.73", C, K::stroke, 0.7f),
              e("M28.38 5.62 A26 26 0 0 1 28.38 42.38", C, K::stroke, 0.4f) },
            // reverb: spiral
            { e("M24 24 A3 3 0 0 1 30 24 A6 6 0 0 1 18 24 A9 9 0 0 1 36 24 A12 12 0 0 1 12 24 A15 15 0 0 1 42 24", B),
              e(circle(24, 24, 1.8f), I, K::fill) },
            // cabinet: speaker
            { e("M11 7 L37 7 Q41 7 41 11 L41 37 Q41 41 37 41 L11 41 Q7 41 7 37 L7 11 Q7 7 11 7 Z", P),
              e(circle(24, 24, 10), P),
              e(circle(24, 24, 3), C, K::fill),
              e(circle(11.5f, 11.5f, 1.4f) + circle(36.5f, 11.5f, 1.4f) + circle(11.5f, 36.5f, 1.4f) + circle(36.5f, 36.5f, 1.4f), M, K::fill) },
            // output: ascent
            { e("M24 42 L24 7 M14 17 L24 7 L34 17", P),
              e("M16 30 L24 24 L32 30", P, K::stroke, 0.55f),
              e("M12 42 L36 42", C) },
        }};
        return table[static_cast<size_t>(rune)];
    }
}

void drawSectionRune(juce::Graphics& g, Rune rune, juce::Rectangle<float> area, float alpha, juce::Colour mono)
{
    const float size = juce::jmin(area.getWidth(), area.getHeight());
    const float scale = size / 48.0f;
    const auto transform = juce::AffineTransform::scale(scale)
        .translated(area.getCentreX() - size * 0.5f, area.getCentreY() - size * 0.5f);
    const bool monochrome = !mono.isTransparent();
    // Small runes get a slightly heavier stroke so they stay legible.
    const float strokeWidth = juce::jmax(1.3f, 2.4f * scale * (size < 32.0f ? 1.25f : 1.0f));
    const juce::PathStrokeType stroke(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    for (const auto& el : runeElements(rune))
    {
        const auto colour = (monochrome ? mono : juce::Colour(el.colour)).withMultipliedAlpha(el.alpha * alpha);
        auto path = el.path;
        path.applyTransform(transform);
        switch (el.kind)
        {
            case RuneElement::fill:
                g.setColour(monochrome ? colour.withMultipliedAlpha(0.8f) : colour);
                g.fillPath(path);
                break;
            case RuneElement::strokeAndTint:
                g.setColour(colour.withMultipliedAlpha(0.18f));
                g.fillPath(path);
                g.setColour(colour);
                g.strokePath(path, stroke);
                break;
            case RuneElement::strokeOverBackground:
                if (!monochrome)
                {
                    g.setColour(bg.withMultipliedAlpha(alpha));
                    g.fillPath(path);
                }
                g.setColour(colour);
                g.strokePath(path, stroke);
                break;
            case RuneElement::stroke:
                g.setColour(colour);
                g.strokePath(path, stroke);
                break;
        }
    }
}

void drawRuneTitle(juce::Graphics& g, Rune rune, const juce::String& text, juce::Rectangle<float> area,
                   const juce::Font& f, juce::Colour textColour, float runeSize, juce::Justification justification)
{
    const float gap = runeSize * 0.4f;
    const float textW = juce::GlyphArrangement::getStringWidth(f, text);
    const float total = runeSize + gap + textW;
    float x = area.getX();
    if (justification.testFlags(juce::Justification::horizontallyCentred))
        x = area.getCentreX() - total * 0.5f;
    else if (justification.testFlags(juce::Justification::right))
        x = area.getRight() - total;
    drawSectionRune(g, rune, { x, area.getCentreY() - runeSize * 0.5f, runeSize, runeSize });
    g.setColour(textColour);
    g.setFont(f);
    g.drawText(text, juce::Rectangle<float>(x + runeSize + gap, area.getY(), textW + 4.0f, area.getHeight()),
               juce::Justification::centredLeft, false);
}

// ---------------------------------------------------------------------------
MageLookAndFeel::MageLookAndFeel()
{
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff0c1222));
    setColour(juce::PopupMenu::textColourId, ivory);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, blue.withAlpha(0.35f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::PopupMenu::headerTextColourId, cyan);
    setColour(juce::ComboBox::textColourId, ivory);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0c1425));
    setColour(juce::ComboBox::outlineColourId, blue.withAlpha(0.75f));
    setColour(juce::ComboBox::arrowColourId, muted);
    setColour(juce::TextButton::textColourOffId, cyan);
    setColour(juce::TextButton::textColourOnId, ivory);
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0f1a2c));
    setColour(juce::AlertWindow::backgroundColourId, juce::Colour(0xff0c1222));
    setColour(juce::AlertWindow::textColourId, ivory);
    setColour(juce::AlertWindow::outlineColourId, cyan.withAlpha(0.6f));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff070b16));
    setColour(juce::TextEditor::textColourId, ivory);
    setColour(juce::TextEditor::outlineColourId, blue.withAlpha(0.6f));
    setColour(juce::TextEditor::focusedOutlineColourId, cyan);
    setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff0c1222));
    setColour(juce::TooltipWindow::textColourId, ivory);
    setColour(juce::TooltipWindow::outlineColourId, blue.withAlpha(0.6f));
}

void MageLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float start, float end, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
    const float R = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const auto accent = accentOf(slider, {});
    const bool hasAccent = !accent.isTransparent();
    const auto arcFrom = hasAccent ? accent.darker(0.15f) : cyan;
    const auto arcTo = hasAccent ? accent.brighter(0.35f) : pink;

    const float arcR = R * 0.90f;
    const float arcW = juce::jmax(2.5f, R * 0.085f);

    // Tick marks
    g.setColour(muted.withAlpha(0.45f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float)i / 10.0f;
        const auto p1 = c.getPointOnCircumference(R * 1.0f, a);
        const auto p2 = c.getPointOnCircumference(R * (i % 5 == 0 ? 0.93f : 0.96f), a);
        g.drawLine({ p1, p2 }, i % 5 == 0 ? 1.4f : 1.0f);
    }

    // Track + value arc (with a soft glow)
    juce::Path track;
    track.addCentredArc(c.x, c.y, arcR * 0.93f, arcR * 0.93f, 0.0f, start, end, true);
    g.setColour(juce::Colour(0xff1d2738));
    g.strokePath(track, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (pos > 0.001f)
    {
        juce::Path arc;
        arc.addCentredArc(c.x, c.y, arcR * 0.93f, arcR * 0.93f, 0.0f, start, angle, true);
        juce::ColourGradient glow(arcFrom, c.x - R, c.y, arcTo, c.x + R, c.y, false);
        juce::ColourGradient halo(arcFrom.withAlpha(0.22f), c.x - R, c.y, arcTo.withAlpha(0.22f), c.x + R, c.y, false);
        g.setGradientFill(halo);
        g.strokePath(arc, juce::PathStrokeType(arcW * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setGradientFill(glow);
        g.strokePath(arc, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Knob body
    const float bodyR = R * 0.70f;
    g.setColour(juce::Colour(0xaa000000));
    g.fillEllipse(c.x - bodyR + 1.5f, c.y - bodyR + bodyR * 0.12f, bodyR * 2.0f, bodyR * 2.0f);
    juce::ColourGradient metalGrad(juce::Colour(0xff5d6f86), c.x - bodyR * 0.6f, c.y - bodyR,
                                   juce::Colour(0xff0f1724), c.x + bodyR * 0.5f, c.y + bodyR, false);
    g.setGradientFill(metalGrad);
    g.fillEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour(juce::Colour(0xff8ea0b6));
    g.drawEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.2f);

    const float capR = bodyR * 0.78f;
    juce::ColourGradient capGrad(juce::Colour(0xff26324a), c.x, c.y - capR,
                                 juce::Colour(0xff0b111d), c.x, c.y + capR, false);
    g.setGradientFill(capGrad);
    g.fillEllipse(c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
    g.setColour(juce::Colour(0xff05080f));
    g.drawEllipse(c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.0f);

    // Pointer
    juce::Path pointer;
    const float pw = juce::jmax(2.0f, bodyR * 0.11f);
    pointer.addRoundedRectangle(-pw * 0.5f, -bodyR * 0.92f, pw, bodyR * 0.5f, pw * 0.5f);
    const auto pointerColour = hasAccent ? accent.brighter(0.4f) : juce::Colour(0xffff8ce0);
    g.setColour(pointerColour.withAlpha(0.25f));
    g.fillPath(pointer, juce::AffineTransform::scale(1.8f, 1.05f).rotated(angle).translated(c.x, c.y));
    g.setColour(pointerColour);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
}

void MageLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto accent = accentOf(b, cyan);
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat();
    const float led = juce::jmin(12.0f, r.getHeight() * 0.5f);
    const juce::Point<float> lc(r.getX() + led, r.getCentreY());

    if (on)
    {
        g.setColour(accent.withAlpha(0.25f));
        g.fillEllipse(lc.x - led, lc.y - led, led * 2.0f, led * 2.0f);
    }
    g.setColour(on ? accent : juce::Colour(0xff283245));
    g.fillEllipse(lc.x - led * 0.45f, lc.y - led * 0.45f, led * 0.9f, led * 0.9f);
    g.setColour(on ? accent.brighter(0.5f) : muted.withAlpha(highlighted ? 0.8f : 0.45f));
    g.drawEllipse(lc.x - led * 0.62f, lc.y - led * 0.62f, led * 1.24f, led * 1.24f, 1.2f);

    g.setColour(on ? ivory : (highlighted ? ivory.withAlpha(0.85f) : muted));
    g.setFont(font(juce::jmin(14.0f, r.getHeight() * 0.62f), true, 0.06f));
    g.drawText(b.getButtonText(), r.withTrimmedLeft(led * 2.2f), juce::Justification::centredLeft, false);
}

void MageLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                           bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    const auto accent = b.findColour(juce::TextButton::textColourOffId);
    g.setColour(juce::Colour(0xff0c1425).brighter(down ? 0.15f : highlighted ? 0.08f : 0.0f));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(accent.withAlpha(b.isEnabled() ? (highlighted ? 0.95f : 0.6f) : 0.2f));
    g.drawRoundedRectangle(r, 6.0f, 1.2f);
}

juce::Font MageLookAndFeel::getTextButtonFont(juce::TextButton&, int h)
{
    return font(juce::jmin(15.0f, (float)h * 0.48f), true, 0.08f);
}

void MageLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0, 0, (float)w, (float)h).reduced(0.5f);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(r, 8.0f);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId).withAlpha(box.isMouseOver(true) ? 1.0f : 0.7f));
    g.drawRoundedRectangle(r, 8.0f, 1.2f);
    juce::Path arrow;
    const float ax = (float)w - 26.0f, ay = (float)h * 0.5f;
    arrow.addTriangle(ax, ay - 4.0f, ax + 12.0f, ay - 4.0f, ax + 6.0f, ay + 4.0f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(arrow);
}

juce::Font MageLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return font(juce::jmin(19.0f, (float)box.getHeight() * 0.48f), false);
}

void MageLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(14, 1, box.getWidth() - 46, box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
}

juce::Font MageLookAndFeel::getPopupMenuFont() { return font(16.0f, false); }

// ---------------------------------------------------------------------------
Knob::Knob(juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& text,
           juce::Colour accent)
    : caption(text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
    slider.setMouseDragSensitivity(220);
    slider.setScrollWheelEnabled(true);
    if (!accent.isTransparent())
        slider.getProperties().set("accent", static_cast<juce::int64>(accent.getARGB()));
    slider.addMouseListener(this, false);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible(slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, paramID, slider);
    if (auto* param = state.getParameter(paramID))
    {
        slider.setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
        slider.setTooltip(param->getName(64) + "  (double-click to reset)");
    }
}

void Knob::resized()
{
    auto r = getLocalBounds();
    const int captionH = juce::jlimit(16, 24, (int)(getHeight() * 0.17f));
    slider.setBounds(r.removeFromTop(r.getHeight() - captionH));
}

void Knob::paint(juce::Graphics& g)
{
    const int captionH = juce::jlimit(16, 24, (int)(getHeight() * 0.17f));
    auto area = getLocalBounds().removeFromBottom(captionH).toFloat();
    const bool live = slider.isMouseOverOrDragging() || isMouseOver(true);
    const auto accent = accentOf(slider, cyan);
    g.setFont(font((float)captionH * 0.66f, true, 0.08f));
    g.setColour(live ? accent.brighter(0.3f) : ivory.withAlpha(0.9f));
    g.drawText(live ? slider.getTextFromValue(slider.getValue()) : caption, area, juce::Justification::centred, false);
}

// ---------------------------------------------------------------------------
Footswitch::Footswitch(juce::AudioProcessorValueTreeState& state, const juce::String& paramID, juce::Colour a)
    : accent(a)
{
    if (auto* param = state.getParameter(paramID))
    {
        attachment = std::make_unique<juce::ParameterAttachment>(*param,
            [this](float v) { on = v > 0.5f; repaint(); }, state.undoManager);
        attachment->sendInitialUpdate();
        setTooltip(param->getName(64));
    }
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void Footswitch::mouseDown(const juce::MouseEvent&)
{
    pressed = true;
    if (attachment != nullptr)
        attachment->setValueAsCompleteGesture(on ? 0.0f : 1.0f);
    repaint();
}

void Footswitch::mouseUp(const juce::MouseEvent&) { pressed = false; repaint(); }

void Footswitch::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    // LED (top)
    const float ledR = juce::jmin(8.0f, r.getWidth() * 0.1f);
    const juce::Point<float> lc(r.getCentreX(), r.getY() + ledR * 2.0f);
    if (on)
    {
        juce::ColourGradient halo(accent.withAlpha(0.55f), lc.x, lc.y, accent.withAlpha(0.0f), lc.x + ledR * 3.0f, lc.y, true);
        g.setGradientFill(halo);
        g.fillEllipse(lc.x - ledR * 3.0f, lc.y - ledR * 3.0f, ledR * 6.0f, ledR * 6.0f);
    }
    g.setColour(on ? accent : juce::Colour(0xff2b3344));
    g.fillEllipse(lc.x - ledR * 0.75f, lc.y - ledR * 0.75f, ledR * 1.5f, ledR * 1.5f);
    g.setColour(on ? accent.brighter(0.6f) : juce::Colour(0xff69758a));
    g.drawEllipse(lc.x - ledR, lc.y - ledR, ledR * 2.0f, ledR * 2.0f, 1.4f);
    if (on)
    {
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.fillEllipse(lc.x - ledR * 0.35f, lc.y - ledR * 0.45f, ledR * 0.4f, ledR * 0.3f);
    }

    // Chrome footswitch (bottom)
    auto sw = r.withTrimmedTop(ledR * 4.5f);
    const float d = juce::jmin(sw.getWidth(), sw.getHeight()) * 0.92f;
    const auto c = sw.getCentre();
    const float press = pressed ? 1.5f : 0.0f;
    g.setColour(juce::Colour(0xff02050b));
    g.fillEllipse(c.x - d * 0.5f, c.y - d * 0.5f, d, d);
    const float nutD = d * 0.86f;
    juce::ColourGradient nut(juce::Colour(0xffb8c6d8), c.x - nutD * 0.4f, c.y - nutD * 0.5f,
                             juce::Colour(0xff3b4658), c.x + nutD * 0.4f, c.y + nutD * 0.5f, false);
    g.setGradientFill(nut);
    g.fillEllipse(c.x - nutD * 0.5f, c.y - nutD * 0.5f, nutD, nutD);
    const float capD = d * 0.56f;
    juce::ColourGradient cap(juce::Colour(0xffe4ecf6), c.x - capD * 0.3f, c.y - capD * 0.5f + press,
                             juce::Colour(0xff5b687c), c.x + capD * 0.3f, c.y + capD * 0.5f + press, false);
    g.setGradientFill(cap);
    g.fillEllipse(c.x - capD * 0.5f, c.y - capD * 0.5f + press, capD, capD);
    g.setColour(accent.withAlpha(on ? 0.9f : 0.4f));
    g.drawEllipse(c.x - nutD * 0.5f, c.y - nutD * 0.5f, nutD, nutD, 1.3f);
}

// ---------------------------------------------------------------------------
LedToggle::LedToggle(juce::AudioProcessorValueTreeState& state, const juce::String& paramID,
                     const juce::String& label, juce::Colour accent)
{
    setButtonText(label);
    getProperties().set("accent", static_cast<juce::int64>(accent.getARGB()));
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state, paramID, *this);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

// ---------------------------------------------------------------------------
void LevelMeter::setLevel(float linearPeak)
{
    const float db = juce::Decibels::gainToDecibels(linearPeak, -60.0f);
    const float target = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 63.0f);
    level = target > level ? target : level * 0.86f + target * 0.14f;
    if (target >= held) { held = target; holdFrames = 45; }
    else if (--holdFrames < 0) held = juce::jmax(level, held - 0.015f);
    repaint();
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    constexpr int segments = 24;
    const float gap = 2.0f;
    const float length = vertical ? r.getHeight() : r.getWidth();
    const float seg = (length - gap * (segments - 1)) / segments;
    const int lit = juce::roundToInt(level * segments);
    const int holdSeg = juce::jlimit(0, segments - 1, juce::roundToInt(held * segments) - 1);

    for (int i = 0; i < segments; ++i)
    {
        const float t = (float)i / (segments - 1);
        auto colour = from.interpolatedWith(to, t);
        if (i >= segments - 2)
            colour = juce::Colour(0xffff4d6d); // clip zone (> ~0 dBFS)
        const bool on = i < lit || (i == holdSeg && held > 0.02f);
        g.setColour(on ? colour : colour.withAlpha(0.12f));
        const float o = i * (seg + gap);
        if (vertical)
            g.fillRoundedRectangle(r.getX(), r.getBottom() - o - seg, r.getWidth(), seg, 1.5f);
        else
            g.fillRoundedRectangle(r.getX() + o, r.getY(), seg, r.getHeight(), 1.5f);
    }
}

// ---------------------------------------------------------------------------
namespace
{
    enum class Glyph { bat, cloud, wand, sword, wisp };

    struct GlyphShape { juce::Path fill, stroke, accent; };

    const GlyphShape& glyphShape(Glyph glyph)
    {
        auto P = [](const char* d) { return juce::Drawable::parseSVGPath(d); };
        static const std::array<GlyphShape, 5> shapes {{
            // bat: spread wings, little ears
            { P("M24 21 C22 17 20 16 18 17 C16 13 10 11 3 15 C7 16 8 19 7 23 C10 21 13 22 15 25 C17 23 20 24 24 29 "
                "C28 24 31 23 33 25 C35 22 38 21 41 23 C40 19 41 16 45 15 C38 11 32 13 30 17 C28 16 26 17 24 21 Z "
                "M21.5 19.5 L20.5 15 L23.5 18.5 Z M26.5 19.5 L27.5 15 L24.5 18.5 Z"), {},
              P("M21.4 20.6 A0.9 0.9 0 1 1 21.41 20.6 Z M26.6 20.6 A0.9 0.9 0 1 1 26.61 20.6 Z") },
            // cloud: puffy outline (fill), curled tail (stroke), inner swirls (accent)
            { P("M9 35 A6 6 0 0 1 9 23 A8 8 0 0 1 22 15 A9 9 0 0 1 37 19 A7 7 0 0 1 40 33 Q39 35 36 35 Z"),
              P("M9 35 C5 35 3 39 6 41 C8 42.5 10 40 8.5 39"),
              P("M13.5 30 C11.5 23.5 20.5 20.5 23.5 25 C25.5 28.5 21 31.5 18.8 29.2 C17.2 27.5 19.5 25.2 21.2 27 "
                "M24.5 31 C28.5 31 30.5 28.5 33.5 28.5 C37 28.5 37.5 24.5 35 23.5") },
            // wand: slim staff tipped with a star
            { P("M35 6 L36.6 11.4 L42 13 L36.6 14.6 L35 20 L33.4 14.6 L28 13 L33.4 11.4 Z"),
              P("M10 41 L30 18"),
              P("M41 22 A1 1 0 1 1 41.01 22 Z M26 8 A0.9 0.9 0 1 1 26.01 8 Z M44 7 A0.8 0.8 0 1 1 44.01 7 Z") },
            // sword: blade, cross-guard, grip and pommel (drawn upright, tilted when placed)
            { P("M24 3 L27 9 L27 31 L21 31 L21 9 Z"),
              P("M14 31.5 L34 31.5 M24 32 L24 41"),
              P("M26 43.5 A2 2 0 1 1 26.01 43.5 Z") },
            // wisp: glowing teardrop flame with a curling tail
            { P("M24 9 C30 17 32 23 32 27 A8 8 0 0 1 16 27 C16 23 18 17 24 9 Z"),
              P("M24 35 C24 41 18 44 14 40"),
              P("M24 24 A3 3 0 1 1 24.01 24 Z") },
        }};
        return shapes[static_cast<size_t>(glyph)];
    }

    void drawSparkle(juce::Graphics& g, juce::Point<float> c, float r, juce::Colour colour)
    {
        juce::Path p;
        p.startNewSubPath(c.x, c.y - r);
        p.quadraticTo(c.x, c.y, c.x + r, c.y);
        p.quadraticTo(c.x, c.y, c.x, c.y + r);
        p.quadraticTo(c.x, c.y, c.x - r, c.y);
        p.quadraticTo(c.x, c.y, c.x, c.y - r);
        p.closeSubPath();
        g.setColour(colour);
        g.fillPath(p);
    }
}

MagicFrieze::MagicFrieze()
{
    setInterceptsMouseClicks(false, false);
    startTimerHz(24);
}

void MagicFrieze::timerCallback()
{
    phase += 0.035f;
    if (phase > juce::MathConstants<float>::twoPi * 100.0f)
        phase = 0.0f;
    repaint();
}

void MagicFrieze::paint(juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const float cy = r.getCentreY();
    const float amp = r.getHeight() * 0.2f;
    const float wavelength = 210.0f;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    auto waveY = [&](float x, float offset, float a)
    {
        return cy + a * std::sin(twoPi * x / wavelength - phase + offset);
    };

    // Soft fade at both ends so the frieze melts into the panel.
    auto edgeFade = [&](float x)
    {
        const float d = juce::jmin(x - r.getX(), r.getRight() - x);
        return juce::jlimit(0.0f, 1.0f, d / 70.0f);
    };

    // ---- Flowing waves ----
    struct Strand { float offset, ampScale, width, alpha; };
    const Strand strands[] = { { 0.0f, 1.0f, 2.2f, 0.9f }, { juce::MathConstants<float>::pi, 1.0f, 2.2f, 0.9f },
                               { 1.6f, 0.55f, 1.0f, 0.45f } };
    for (const auto& s : strands)
    {
        juce::Path wave;
        wave.startNewSubPath(r.getX(), waveY(r.getX(), s.offset, amp * s.ampScale));
        for (float x = r.getX() + 4.0f; x <= r.getRight(); x += 4.0f)
            wave.lineTo(x, waveY(x, s.offset, amp * s.ampScale));
        juce::ColourGradient grad(colours::cyan.withAlpha(0.0f), r.getX(), cy, colours::pink.withAlpha(0.0f), r.getRight(), cy, false);
        grad.addColour(0.08, colours::cyan.withAlpha(s.alpha));
        grad.addColour(0.5, colours::violet.withAlpha(s.alpha));
        grad.addColour(0.92, colours::pink.withAlpha(s.alpha));
        juce::ColourGradient glow = grad;
        for (int i = 0; i < glow.getNumColours(); ++i)
            glow.setColour(i, glow.getColour(i).withMultipliedAlpha(0.18f));
        g.setGradientFill(glow);
        g.strokePath(wave, juce::PathStrokeType(s.width * 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setGradientFill(grad);
        g.strokePath(wave, juce::PathStrokeType(s.width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- Sparkles that twinkle along the waves ----
    for (int i = 0; i < 34; ++i)
    {
        const float x = r.getX() + 20.0f + std::fmod(i * 97.3f, r.getWidth() - 40.0f);
        const float yOff = std::sin(i * 2.17f) * r.getHeight() * 0.42f;
        const float tw = 0.5f + 0.5f * std::sin(phase * 2.3f + i * 1.7f);
        const auto colour = colours::cyan.interpolatedWith(colours::pink, (x - r.getX()) / r.getWidth());
        drawSparkle(g, { x, cy + yOff }, 1.5f + 2.2f * tw, (i % 3 == 0 ? colours::ivory : colour)
                        .withAlpha((0.15f + 0.6f * tw) * edgeFade(x)));
    }

    // ---- Magical silhouettes riding the waves ----
    struct Placed { Glyph glyph; float t; float rotation; };
    const Placed placed[] = {
        { Glyph::wisp, 0.05f, 0.0f },  { Glyph::bat, 0.15f, 0.0f },    { Glyph::wand, 0.25f, 0.0f },
        { Glyph::cloud, 0.35f, 0.0f },   { Glyph::wisp, 0.44f, 0.0f },   { Glyph::sword, 0.53f, -0.75f },
        { Glyph::bat, 0.62f, 0.15f },  { Glyph::cloud, 0.71f, 0.0f },    { Glyph::wand, 0.80f, 0.0f },
        { Glyph::sword, 0.88f, 0.75f },{ Glyph::wisp, 0.96f, 0.0f },
    };
    const float size = r.getHeight() * 0.95f;
    for (size_t i = 0; i < std::size(placed); ++i)
    {
        const auto& p = placed[i];
        const float x = r.getX() + p.t * r.getWidth();
        const float t = (x - r.getX()) / r.getWidth();
        const float fade = edgeFade(x);
        const auto tint = colours::cyan.interpolatedWith(colours::violet, juce::jmin(1.0f, t * 2.0f))
                              .interpolatedWith(colours::pink, juce::jmax(0.0f, t * 2.0f - 1.0f));
        // Ride the wave, with a little independent float (bats flap higher, wisps bob).
        float bob = std::sin(phase * 1.6f + (float)i * 1.3f);
        float y = waveY(x, i % 2 == 0 ? 0.0f : juce::MathConstants<float>::pi, amp) * 0.35f + cy * 0.65f;
        if (p.glyph == Glyph::bat)  y += bob * 3.0f - 2.0f;
        if (p.glyph == Glyph::wisp) y += bob * 2.5f;
        if (p.glyph == Glyph::cloud) y += bob * 1.5f;
        const float rot = p.rotation + (p.glyph == Glyph::bat ? 0.08f * bob : 0.0f);

        const auto& shape = glyphShape(p.glyph);
        const auto tf = juce::AffineTransform::translation(-24.0f, -26.0f)
                            .scaled(size / 48.0f).rotated(rot).translated(x, y);

        if (p.glyph == Glyph::wisp || p.glyph == Glyph::wand)
        {
            const auto glowCentre = p.glyph == Glyph::wisp ? juce::Point<float>(24.0f, 26.0f).transformedBy(tf)
                                                           : juce::Point<float>(35.0f, 13.0f).transformedBy(tf);
            const float gr = size * 0.55f;
            juce::ColourGradient halo(tint.withAlpha(0.45f * fade), glowCentre.x, glowCentre.y,
                                      tint.withAlpha(0.0f), glowCentre.x + gr, glowCentre.y, true);
            g.setGradientFill(halo);
            g.fillEllipse(glowCentre.x - gr, glowCentre.y - gr, gr * 2.0f, gr * 2.0f);
        }

        auto fillPath = shape.fill;   fillPath.applyTransform(tf);
        auto strokePath = shape.stroke; strokePath.applyTransform(tf);
        auto accentPath = shape.accent; accentPath.applyTransform(tf);
        const float sw = juce::jmax(1.4f, size * 0.055f);

        switch (p.glyph)
        {
            case Glyph::cloud:
            {
                juce::ColourGradient body(tint.withAlpha(0.28f * fade), x, y - size * 0.3f,
                                          juce::Colour(0xff0a0d1b).withAlpha(0.9f * fade), x, y + size * 0.3f, false);
                g.setGradientFill(body);
                g.fillPath(fillPath);
                g.setColour(tint.withAlpha(0.95f * fade));
                g.strokePath(fillPath, juce::PathStrokeType(sw * 0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour(tint.withAlpha(0.7f * fade));
                g.strokePath(strokePath, juce::PathStrokeType(sw * 0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour(colours::ivory.withAlpha(0.85f * fade));
                g.strokePath(accentPath, juce::PathStrokeType(sw * 0.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                break;
            }
            case Glyph::bat:
                g.setColour(juce::Colour(0xff0a0d1b).withAlpha(fade));          // silhouette
                g.fillPath(fillPath);
                g.setColour(tint.withAlpha(0.9f * fade));                      // rim light
                g.strokePath(fillPath, juce::PathStrokeType(1.2f));
                g.strokePath(strokePath, juce::PathStrokeType(sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour(colours::pink.brighter(0.3f).withAlpha(fade));      // glowing eyes
                g.fillPath(accentPath);
                break;
            case Glyph::wand:
                g.setColour(colours::muted.withAlpha(0.9f * fade));
                g.strokePath(strokePath, juce::PathStrokeType(sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour(colours::ivory.withAlpha(fade));
                g.fillPath(fillPath);
                g.setColour(tint.withAlpha(0.8f * fade));
                g.fillPath(accentPath);
                break;
            case Glyph::sword:
                g.setColour(colours::ivory.withAlpha(0.85f * fade));
                g.fillPath(fillPath);
                g.setColour(tint.withAlpha(fade));
                g.strokePath(fillPath, juce::PathStrokeType(1.0f));
                g.strokePath(strokePath, juce::PathStrokeType(sw * 1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.fillPath(accentPath);
                break;
            case Glyph::wisp:
                g.setColour(tint.withAlpha(0.75f * fade));
                g.fillPath(fillPath);
                g.strokePath(strokePath, juce::PathStrokeType(sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour(colours::ivory.withAlpha(0.9f * fade));
                g.fillPath(accentPath);
                break;
        }
    }
}

// ---------------------------------------------------------------------------
IconButton::IconButton(Icon i, const juce::String& tip) : juce::Button(tip), icon(i)
{
    setTooltip(tip);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void IconButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff0c1425).brighter(down ? 0.15f : highlighted ? 0.07f : 0.0f));
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(blue.withAlpha(isEnabled() ? (highlighted ? 0.9f : 0.5f) : 0.15f));
    g.drawRoundedRectangle(r, 7.0f, 1.1f);

    auto ir = r.reduced(r.getHeight() * 0.27f);
    juce::Path p;
    const float s = juce::jmax(1.6f, ir.getHeight() * 0.11f);
    switch (icon)
    {
        case Icon::undo:
        case Icon::redo:
        {
            const bool redo = icon == Icon::redo;
            const auto c = ir.getCentre().translated(0.0f, ir.getHeight() * 0.08f);
            const float rad = ir.getHeight() * 0.38f;
            p.addCentredArc(c.x, c.y, rad, rad, 0.0f,
                            redo ? -juce::MathConstants<float>::halfPi * 1.6f : juce::MathConstants<float>::halfPi * 1.6f,
                            redo ? juce::MathConstants<float>::halfPi * 0.9f : -juce::MathConstants<float>::halfPi * 0.9f, true);
            const auto tip = p.getCurrentPosition();
            juce::Path head;
            const float hs = ir.getHeight() * 0.22f;
            const float dir = redo ? 1.0f : -1.0f;
            head.addTriangle(tip.x - dir * hs * 0.2f, tip.y - hs, tip.x - dir * hs * 0.2f, tip.y + hs, tip.x + dir * hs, tip.y);
            const auto colour = isEnabled() ? ivory.withAlpha(0.85f) : muted.withAlpha(0.3f);
            g.setColour(colour);
            g.strokePath(p, juce::PathStrokeType(s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.fillPath(head);
            return;
        }
        case Icon::previous:
        case Icon::next:
        {
            const bool next = icon == Icon::next;
            const float x0 = next ? ir.getX() + ir.getWidth() * 0.3f : ir.getRight() - ir.getWidth() * 0.3f;
            const float x1 = next ? ir.getRight() - ir.getWidth() * 0.25f : ir.getX() + ir.getWidth() * 0.25f;
            p.startNewSubPath(x0, ir.getY());
            p.lineTo(x1, ir.getCentreY());
            p.lineTo(x0, ir.getBottom());
            break;
        }
        case Icon::save:
        {
            p.addRoundedRectangle(ir, 2.0f);
            p.startNewSubPath(ir.getX() + ir.getWidth() * 0.25f, ir.getY());
            p.lineTo(ir.getX() + ir.getWidth() * 0.25f, ir.getY() + ir.getHeight() * 0.32f);
            p.lineTo(ir.getRight() - ir.getWidth() * 0.25f, ir.getY() + ir.getHeight() * 0.32f);
            p.lineTo(ir.getRight() - ir.getWidth() * 0.25f, ir.getY());
            p.addRectangle(ir.getX() + ir.getWidth() * 0.25f, ir.getBottom() - ir.getHeight() * 0.38f,
                           ir.getWidth() * 0.5f, ir.getHeight() * 0.38f);
            break;
        }
        case Icon::trash:
        {
            const float w = ir.getWidth();
            p.startNewSubPath(ir.getX(), ir.getY() + ir.getHeight() * 0.15f);
            p.lineTo(ir.getRight(), ir.getY() + ir.getHeight() * 0.15f);
            p.startNewSubPath(ir.getX() + w * 0.35f, ir.getY());
            p.lineTo(ir.getRight() - w * 0.35f, ir.getY());
            p.startNewSubPath(ir.getX() + w * 0.12f, ir.getY() + ir.getHeight() * 0.28f);
            p.lineTo(ir.getX() + w * 0.2f, ir.getBottom());
            p.lineTo(ir.getRight() - w * 0.2f, ir.getBottom());
            p.lineTo(ir.getRight() - w * 0.12f, ir.getY() + ir.getHeight() * 0.28f);
            break;
        }
        case Icon::folder:
        {
            p.startNewSubPath(ir.getX(), ir.getBottom());
            p.lineTo(ir.getX(), ir.getY() + ir.getHeight() * 0.1f);
            p.lineTo(ir.getX() + ir.getWidth() * 0.38f, ir.getY() + ir.getHeight() * 0.1f);
            p.lineTo(ir.getX() + ir.getWidth() * 0.5f, ir.getY() + ir.getHeight() * 0.25f);
            p.lineTo(ir.getRight(), ir.getY() + ir.getHeight() * 0.25f);
            p.lineTo(ir.getRight(), ir.getBottom());
            p.closeSubPath();
            break;
        }
    }
    g.setColour(isEnabled() ? ivory.withAlpha(0.85f) : muted.withAlpha(0.3f));
    g.strokePath(p, juce::PathStrokeType(s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
} // namespace mage::ui
