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
