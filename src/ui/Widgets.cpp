#include "Widgets.h"
#include "BinaryData.h"

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

// ---------------------------------------------------------------------------
const Theme& arcaneTheme()
{
    static const Theme t = []
    {
        Theme a;
        a.fae = false;
        a.bg = juce::Colour(0xff070911); a.face = juce::Colour(0xff0a0d1b); a.metal = juce::Colour(0xff151d2b);
        a.cyan = juce::Colour(0xff59ddff); a.blue = juce::Colour(0xff458bff); a.pink = juce::Colour(0xffff70d5);
        a.violet = juce::Colour(0xffb06bff); a.ivory = juce::Colour(0xffedf4ff); a.muted = juce::Colour(0xff91a3ba);
        a.dim = juce::Colour(0xff3a465a);
        a.header = juce::Colour(0xff0a0f1c); a.headerLine = juce::Colour(0xff24304a); a.etching = juce::Colour(0xff14213a);
        a.divider = juce::Colour(0xff26324a); a.panelShadow = juce::Colour(0xdd00030a); a.panelEdgeDark = juce::Colour(0xff15152a);
        a.faceTop = juce::Colour(0xff0e1224); a.faceBottom = juce::Colour(0xff080a16); a.ledOff = juce::Colour(0xff283245);
        a.track = juce::Colour(0xff1d2738); a.knobHi = juce::Colour(0xff5d6f86); a.knobLo = juce::Colour(0xff0f1724);
        a.knobRim = juce::Colour(0xff8ea0b6); a.capHi = juce::Colour(0xff26324a); a.capLo = juce::Colour(0xff0b111d);
        a.screen = juce::Colour(0xff05070f); a.control = juce::Colour(0xff0c1425); a.popup = juce::Colour(0xff0c1222);
        a.screwHole = juce::Colour(0xff02050b); a.screwSlot = juce::Colour(0xff73839a); a.glow = juce::Colour(0xff9a6bff);
        return a;
    }();
    return t;
}

// Moonlit glow: still a night palette, but lifted toward deep teal, with moss,
// pale gold, rose and lilac light, pearl knobs and warm moonlight text.
const Theme& faeTheme()
{
    static const Theme t = []
    {
        Theme f;
        f.fae = true;
        f.bg = juce::Colour(0xff0c1718); f.face = juce::Colour(0xff10201f); f.metal = juce::Colour(0xff1b2d2b);
        f.cyan = juce::Colour(0xff8fe8b6); f.blue = juce::Colour(0xfff1d38a); f.pink = juce::Colour(0xfff6a3c4);
        f.violet = juce::Colour(0xffc9b5ff); f.ivory = juce::Colour(0xfffbf4e4); f.muted = juce::Colour(0xffa8bdb3);
        f.dim = juce::Colour(0xff41564e);
        f.header = juce::Colour(0xff0f1d1d); f.headerLine = juce::Colour(0xff2c4440); f.etching = juce::Colour(0xff21403a);
        f.divider = juce::Colour(0xff2d4540); f.panelShadow = juce::Colour(0xcc000806); f.panelEdgeDark = juce::Colour(0xff15262a);
        f.faceTop = juce::Colour(0xff142624); f.faceBottom = juce::Colour(0xff0c1817); f.ledOff = juce::Colour(0xff2d403b);
        f.track = juce::Colour(0xff203431); f.knobHi = juce::Colour(0xffc6d5cc); f.knobLo = juce::Colour(0xff2a3a35);
        f.knobRim = juce::Colour(0xffe2ede4); f.capHi = juce::Colour(0xff3a5049); f.capLo = juce::Colour(0xff13201d);
        f.screen = juce::Colour(0xff081210); f.control = juce::Colour(0xff11211f); f.popup = juce::Colour(0xff10201e);
        f.screwHole = juce::Colour(0xff050c0a); f.screwSlot = juce::Colour(0xff9fb4a8); f.glow = juce::Colour(0xffb9a6ff);
        return f;
    }();
    return t;
}

static const Theme* activeTheme = nullptr;

const Theme& currentTheme() { return activeTheme != nullptr ? *activeTheme : arcaneTheme(); }

void useTheme(const Theme& t)
{
    if (activeTheme == &t)
        return;
    activeTheme = &t;
    bg = t.bg; face = t.face; metal = t.metal; cyan = t.cyan; blue = t.blue; pink = t.pink;
    violet = t.violet; ivory = t.ivory; muted = t.muted; dim = t.dim;
}

void useThemeOf(const juce::Component& c)
{
    for (auto* p = &c; p != nullptr; p = p->getParentComponent())
        if (auto* holder = dynamic_cast<const ThemeHolder*>(p))
        {
            useTheme(holder->getTheme());
            return;
        }
}

static juce::Colour accentOf(const juce::Component& c, juce::Colour fallback)
{
    const auto v = c.getProperties()["accent"];
    return v.isVoid() ? fallback : juce::Colour(static_cast<juce::uint32>(static_cast<juce::int64>(v)));
}

// ---------------------------------------------------------------------------
void drawPanel(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent, float radius)
{
    const auto& t = currentTheme();
    g.setColour(t.panelShadow);
    g.fillRoundedRectangle(r.translated(0.0f, 7.0f), radius);
    juce::ColourGradient edge(accent.withAlpha(0.9f), r.getX(), r.getY(),
                              t.panelEdgeDark, r.getRight(), r.getBottom(), false);
    g.setGradientFill(edge);
    g.fillRoundedRectangle(r, radius);
    g.setColour(accent.withAlpha(0.95f));
    g.drawRoundedRectangle(r, radius, 2.0f);
    auto inner = r.reduced(5.0f);
    juce::ColourGradient faceGrad(t.faceTop, inner.getX(), inner.getY(),
                                  t.faceBottom, inner.getX(), inner.getBottom(), false);
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
    g.setColour(currentTheme().screwHole);
    g.fillEllipse(p.x - r, p.y - r, r * 2.0f, r * 2.0f);
    g.setColour(accent.withAlpha(0.6f));
    g.drawEllipse(p.x - r * 0.7f, p.y - r * 0.7f, r * 1.4f, r * 1.4f, 1.2f);
    g.setColour(currentTheme().screwSlot.withAlpha(0.7f));
    g.drawLine(p.x - r * 0.45f, p.y, p.x + r * 0.45f, p.y, 1.0f);
}

namespace
{
    struct LogoImages { juce::Image ink, glow; };
    const LogoImages& logoImages(Logo logo)
    {
        static const LogoImages mark { juce::ImageCache::getFromMemory(BinaryData::mage_mark_ink_png, BinaryData::mage_mark_ink_pngSize),
                                       juce::ImageCache::getFromMemory(BinaryData::mage_mark_glow_png, BinaryData::mage_mark_glow_pngSize) };
        static const LogoImages word { juce::ImageCache::getFromMemory(BinaryData::mage_wordmark_ink_png, BinaryData::mage_wordmark_ink_pngSize),
                                       juce::ImageCache::getFromMemory(BinaryData::mage_wordmark_glow_png, BinaryData::mage_wordmark_glow_pngSize) };
        return logo == Logo::mark ? mark : word;
    }
}

float logoAspect(Logo logo)
{
    const auto& img = logoImages(logo).ink;
    return img.isValid() ? (float)img.getWidth() / (float)img.getHeight() : 1.0f;
}

void drawLogo(juce::Graphics& g, Logo logo, juce::Rectangle<float> area)
{
    const auto& images = logoImages(logo);
    if (!images.ink.isValid())
        return;
    const auto& t = currentTheme();
    // Fit the image inside `area`, keeping its aspect ratio.
    const auto target = juce::RectanglePlacement(juce::RectanglePlacement::centred)
                            .appliedTo(juce::Rectangle<float>(0, 0, (float)images.ink.getWidth(), (float)images.ink.getHeight()), area);
    const auto tf = juce::AffineTransform::scale(target.getWidth() / (float)images.ink.getWidth(),
                                                 target.getHeight() / (float)images.ink.getHeight())
                        .translated(target.getX(), target.getY());
    juce::Graphics::ScopedSaveState save(g);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.setColour(t.glow.withAlpha(0.6f));
    g.drawImageTransformed(images.glow, tf, true);
    const bool diagonal = logo == Logo::mark;
    juce::ColourGradient ink(t.cyan, target.getX(), target.getY(),
                             t.pink, target.getRight(), diagonal ? target.getBottom() : target.getY() + target.getHeight() * 0.18f, false);
    ink.addColour(0.5, t.violet);
    g.setGradientFill(ink);
    g.drawImageTransformed(images.ink, tf, true);
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

        static const std::array<std::vector<RuneElement>, 14> table {{
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
            // shimmer: rising star
            { e("M22 10 Q23.5 24 38 26 Q23.5 28 22 42 Q20.5 28 6 26 Q20.5 24 22 10 Z", V, K::strokeAndTint),
              e("M38 5 Q38.8 10.2 44 11 Q38.8 11.8 38 17 Q37.2 11.8 32 11 Q37.2 10.2 38 5 Z", P, K::fill),
              e(circle(39, 38, 2.0f), I, K::fill) },
            // freeze: ice crystal
            { e("M24 5 L40.5 14.5 L40.5 33.5 L24 43 L7.5 33.5 L7.5 14.5 Z", C),
              e("M24 11 L24 37 M12.7 17.5 L35.3 30.5 M12.7 30.5 L35.3 17.5", B, K::stroke, 0.75f),
              e(circle(24, 24, 2.4f), I, K::fill) },
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
        // Rune colours are authored in the Arcane palette; map them onto the active theme.
        auto themed = [](juce::uint32 c)
        {
            switch (c)
            {
                case 0xff59ddff: return cyan;
                case 0xffff70d5: return pink;
                case 0xff458bff: return blue;
                case 0xffb06bff: return violet;
                case 0xffedf4ff: return ivory;
                case 0xff9a8bff: return violet.interpolatedWith(blue, 0.35f);
                case 0xff91a3ba: return muted;
                default:         return juce::Colour(c);
            }
        };
        const auto colour = (monochrome ? mono : themed(el.colour)).withMultipliedAlpha(el.alpha * alpha);
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
    applyTheme(arcaneTheme());
}

void MageLookAndFeel::applyTheme(const Theme& t)
{
    setColour(juce::PopupMenu::backgroundColourId, t.popup);
    setColour(juce::PopupMenu::textColourId, t.ivory);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, t.blue.withAlpha(0.35f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::PopupMenu::headerTextColourId, t.cyan);
    setColour(juce::ComboBox::textColourId, t.ivory);
    setColour(juce::ComboBox::backgroundColourId, t.control);
    setColour(juce::ComboBox::outlineColourId, t.blue.withAlpha(0.75f));
    setColour(juce::ComboBox::arrowColourId, t.muted);
    setColour(juce::TextButton::textColourOffId, t.cyan);
    setColour(juce::TextButton::textColourOnId, t.ivory);
    setColour(juce::TextButton::buttonColourId, t.control);
    setColour(juce::AlertWindow::backgroundColourId, t.popup);
    setColour(juce::AlertWindow::textColourId, t.ivory);
    setColour(juce::AlertWindow::outlineColourId, t.cyan.withAlpha(0.6f));
    setColour(juce::TextEditor::backgroundColourId, t.screen);
    setColour(juce::TextEditor::textColourId, t.ivory);
    setColour(juce::TextEditor::outlineColourId, t.blue.withAlpha(0.6f));
    setColour(juce::TextEditor::focusedOutlineColourId, t.cyan);
    setColour(juce::TooltipWindow::backgroundColourId, t.popup);
    setColour(juce::TooltipWindow::textColourId, t.ivory);
    setColour(juce::TooltipWindow::outlineColourId, t.blue.withAlpha(0.6f));
}

void MageLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float start, float end, juce::Slider& slider)
{
    useThemeOf(slider);
    const auto& t = currentTheme();
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
    g.setColour(t.track);
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
    juce::ColourGradient metalGrad(t.knobHi, c.x - bodyR * 0.6f, c.y - bodyR,
                                   t.knobLo, c.x + bodyR * 0.5f, c.y + bodyR, false);
    g.setGradientFill(metalGrad);
    g.fillEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour(t.knobRim);
    g.drawEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.2f);

    const float capR = bodyR * 0.78f;
    juce::ColourGradient capGrad(t.capHi, c.x, c.y - capR,
                                 t.capLo, c.x, c.y + capR, false);
    g.setGradientFill(capGrad);
    g.fillEllipse(c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
    g.setColour(juce::Colour(0xff05080f));
    g.drawEllipse(c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.0f);

    // Pointer
    juce::Path pointer;
    const float pw = juce::jmax(2.0f, bodyR * 0.11f);
    pointer.addRoundedRectangle(-pw * 0.5f, -bodyR * 0.92f, pw, bodyR * 0.5f, pw * 0.5f);
    const auto pointerColour = hasAccent ? accent.brighter(0.4f) : pink.brighter(0.15f);
    g.setColour(pointerColour.withAlpha(0.25f));
    g.fillPath(pointer, juce::AffineTransform::scale(1.8f, 1.05f).rotated(angle).translated(c.x, c.y));
    g.setColour(pointerColour);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
}

void MageLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    useThemeOf(b);
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
    g.setColour(on ? accent : currentTheme().ledOff);
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
    useThemeOf(b);
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    const auto accent = b.findColour(juce::TextButton::textColourOffId);
    g.setColour(currentTheme().control.brighter(down ? 0.15f : highlighted ? 0.08f : 0.0f));
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
    useThemeOf(box);
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

void Knob::setAccent(juce::Colour accent)
{
    slider.getProperties().set("accent", static_cast<juce::int64>(accent.getARGB()));
    repaint();
}

void Knob::resized()
{
    auto r = getLocalBounds();
    const int captionH = juce::jlimit(16, 24, (int)(getHeight() * 0.17f));
    slider.setBounds(r.removeFromTop(r.getHeight() - captionH));
}

void Knob::paint(juce::Graphics& g)
{
    useThemeOf(*this);
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
    const bool momentary = isMomentary && isMomentary();
    if (attachment != nullptr)
        attachment->setValueAsCompleteGesture(momentary ? 1.0f : (on ? 0.0f : 1.0f));
    repaint();
}

void Footswitch::mouseUp(const juce::MouseEvent&)
{
    pressed = false;
    if (isMomentary && isMomentary() && attachment != nullptr)
        attachment->setValueAsCompleteGesture(0.0f);
    repaint();
}

void Footswitch::paint(juce::Graphics& g)
{
    useThemeOf(*this);
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
    g.setColour(on ? accent : currentTheme().ledOff);
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
    g.setColour(currentTheme().screwHole);
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

void LedToggle::setAccent(juce::Colour accent)
{
    getProperties().set("accent", static_cast<juce::int64>(accent.getARGB()));
    repaint();
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
    useThemeOf(*this);
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
    // An invented runic alphabet: each glyph is a few strokes on a 3x5 grid
    // (x 0..2, y 0..4). Segments are "x1,y1-x2,y2" separated by ';'.
    const std::vector<std::vector<juce::Line<float>>>& runeAlphabet()
    {
        static const std::vector<std::vector<juce::Line<float>>> alphabet = []
        {
            const char* defs[] = {
                "0,0-0,4;0,1-2,0",           "1,0-1,4;0,1-2,3",           "0,0-0,4;0,0-2,1;2,1-0,2",
                "0,4-1,0;1,0-2,4",           "0,0-2,4;2,0-0,4",           "1,0-1,4;0,0-1,1;2,0-1,1",
                "0,0-0,4;2,0-2,4;0,2-2,1",   "1,0-0,2;0,2-1,4;1,4-2,2;2,2-1,0", "0,0-0,4;0,2-2,0;0,2-2,4",
                "1,0-1,4;0,3-1,2;2,3-1,2",   "0,4-0,0;0,0-2,2;2,2-2,4",   "0,1-2,1;0,3-2,3;1,0-1,4",
                "0,0-0,4;0,0-2,1;0,2-2,1;0,2-2,3;0,4-2,3", "1,0-1,4;1,1-2,0;1,3-0,4", "0,0-1,4;1,4-2,0",
                "0,0-0,3;0,3-2,4;2,0-2,4",   "1,0-1,4;0,2-1,1;1,1-2,2",   "0,0-2,0;1,0-1,4;0,3-1,4",
                "0,4-2,0;0,0-1,2",           "0,1-1,0;1,0-2,1;1,0-1,4;0,4-2,4",
            };
            std::vector<std::vector<juce::Line<float>>> out;
            for (auto* d : defs)
            {
                std::vector<juce::Line<float>> glyph;
                for (const auto& seg : juce::StringArray::fromTokens(d, ";", ""))
                {
                    const auto pts = juce::StringArray::fromTokens(seg, "-,", "");
                    if (pts.size() == 4)
                        glyph.push_back({ pts[0].getFloatValue(), pts[1].getFloatValue(),
                                          pts[2].getFloatValue(), pts[3].getFloatValue() });
                }
                out.push_back(std::move(glyph));
            }
            return out;
        }();
        return alphabet;
    }

    // The "spell": glyph indices, with -1 marking a word break (drawn as a small diamond).
    const std::vector<int>& spellText()
    {
        static const std::vector<int> text = []
        {
            std::vector<int> t;
            juce::Random rnd(0x4d414745); // "MAGE"
            const int numGlyphs = (int)runeAlphabet().size();
            for (int word = 0; word < 24; ++word)
            {
                const int len = 3 + rnd.nextInt(5);
                for (int k = 0; k < len; ++k)
                    t.push_back(rnd.nextInt(numGlyphs));
                t.push_back(-1);
            }
            return t;
        }();
        return text;
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
    phase += 1.0f / 24.0f; // seconds
    if (phase > 3600.0f)
        phase = 0.0f;
    repaint();
}

void MagicFrieze::paint(juce::Graphics& g)
{
    useThemeOf(*this);
    if (currentTheme().fae)
        paintVine(g);
    else
        paintInscription(g);
}

void MagicFrieze::paintInscription(juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const float cy = r.getCentreY();
    const float amp = r.getHeight() * 0.11f;
    const float wavelength = 380.0f;
    const float halfBand = r.getHeight() * 0.27f;      // ribbon half-height
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    const float wavePhase = phase * 0.45f;              // ribbon undulation

    auto centreY = [&](float x) { return cy + amp * std::sin(twoPi * x / wavelength - wavePhase); };
    auto slope   = [&](float x) { return amp * twoPi / wavelength * std::cos(twoPi * x / wavelength - wavePhase); };
    auto edgeFade = [&](float x)
    {
        const float d = juce::jmin(x - r.getX(), r.getRight() - x);
        return juce::jlimit(0.0f, 1.0f, d / 90.0f);
    };
    auto tintAt = [&](float x)
    {
        const float t = juce::jlimit(0.0f, 1.0f, (x - r.getX()) / r.getWidth());
        return colours::cyan.interpolatedWith(colours::violet, juce::jmin(1.0f, t * 2.0f))
                            .interpolatedWith(colours::pink, juce::jmax(0.0f, t * 2.0f - 1.0f));
    };

    // ---- Ribbon body and its two glowing edges ----
    juce::Path top, bottom, body;
    top.startNewSubPath(r.getX(), centreY(r.getX()) - halfBand);
    bottom.startNewSubPath(r.getX(), centreY(r.getX()) + halfBand);
    for (float x = r.getX() + 4.0f; x <= r.getRight() + 0.1f; x += 4.0f)
    {
        top.lineTo(x, centreY(x) - halfBand);
        bottom.lineTo(x, centreY(x) + halfBand);
    }
    body = top;
    for (float x = r.getRight(); x >= r.getX() - 0.1f; x -= 4.0f)
        body.lineTo(x, centreY(x) + halfBand);
    body.closeSubPath();

    auto gradientFor = [&](float alpha)
    {
        juce::ColourGradient grad(colours::cyan.withAlpha(0.0f), r.getX(), cy, colours::pink.withAlpha(0.0f), r.getRight(), cy, false);
        grad.addColour(0.07, colours::cyan.withAlpha(alpha));
        grad.addColour(0.5, colours::violet.withAlpha(alpha));
        grad.addColour(0.93, colours::pink.withAlpha(alpha));
        return grad;
    };
    g.setGradientFill(gradientFor(0.07f));
    g.fillPath(body);
    const juce::PathStrokeType glowStroke(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    const juce::PathStrokeType edgeStroke(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setGradientFill(gradientFor(0.12f));
    g.strokePath(top, glowStroke);
    g.strokePath(bottom, glowStroke);
    g.setGradientFill(gradientFor(0.85f));
    g.strokePath(top, edgeStroke);
    g.strokePath(bottom, edgeStroke);

    // ---- Scrolling inscription ----
    const auto& alphabet = runeAlphabet();
    const auto& text = spellText();
    const float glyphH = halfBand * 1.25f;
    const float unit = glyphH / 4.0f;          // grid unit
    const float advance = unit * 2.0f + unit * 2.2f;
    const float wordGap = unit * 4.0f;

    float textLength = 0.0f;
    for (int idx : text)
        textLength += idx < 0 ? wordGap : advance;

    const float scroll = std::fmod(phase * 18.0f, textLength);                       // px/s leftward
    const float pulseCycle = 7.0f;
    const float pulseX = r.getX() - 150.0f + std::fmod(phase, pulseCycle) / pulseCycle * (r.getWidth() + 300.0f);

    const juce::PathStrokeType runeGlow(unit * 1.6f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded);
    const juce::PathStrokeType runeStroke(juce::jmax(1.1f, unit * 0.55f), juce::PathStrokeType::mitered, juce::PathStrokeType::rounded);

    // Lay the text out repeatedly so the ribbon is always full.
    for (float base = r.getX() - scroll; base < r.getRight(); base += textLength)
    {
        float x = base;
        for (int idx : text)
        {
            const float w = idx < 0 ? wordGap : advance;
            const float cx = x + w * 0.5f;
            x += w;
            if (cx < r.getX() - 10.0f || cx > r.getRight() + 10.0f)
                continue;

            const float fade = edgeFade(cx);
            if (fade <= 0.0f)
                continue;
            const float pulse = std::exp(-std::pow((cx - pulseX) / 80.0f, 2.0f));
            const auto tint = tintAt(cx).interpolatedWith(colours::ivory, pulse * 0.7f);
            const float angle = std::atan(slope(cx));
            const auto tf = juce::AffineTransform::rotation(angle).translated(cx, centreY(cx));

            if (idx < 0)
            {
                juce::Path diamond;
                const float d = unit * 0.7f;
                diamond.addQuadrilateral(0, -d, d, 0, 0, d, -d, 0);
                diamond.applyTransform(tf);
                g.setColour(tint.withAlpha((0.45f + 0.5f * pulse) * fade));
                g.fillPath(diamond);
                continue;
            }

            juce::Path glyph;
            for (const auto& seg : alphabet[(size_t)idx])
            {
                glyph.startNewSubPath((seg.getStartX() - 1.0f) * unit, (seg.getStartY() - 2.0f) * unit);
                glyph.lineTo((seg.getEndX() - 1.0f) * unit, (seg.getEndY() - 2.0f) * unit);
            }
            glyph.applyTransform(tf);
            g.setColour(tint.withAlpha((0.10f + 0.30f * pulse) * fade));
            g.strokePath(glyph, runeGlow);
            g.setColour(tint.withAlpha((0.55f + 0.45f * pulse) * fade));
            g.strokePath(glyph, runeStroke);
        }
    }

    // ---- A few faint sparkles drifting around the ribbon ----
    for (int i = 0; i < 16; ++i)
    {
        const float x = r.getX() + 30.0f + std::fmod(i * 131.7f + phase * 6.0f, r.getWidth() - 60.0f);
        const float side = (i % 2 == 0 ? -1.0f : 1.0f);
        const float y = centreY(x) + side * (halfBand + 4.0f + 3.0f * std::abs(std::sin(i * 1.9f)));
        const float tw = 0.5f + 0.5f * std::sin(phase * 2.2f + i * 1.7f);
        drawSparkle(g, { x, y }, 1.2f + 1.6f * tw,
                    (i % 3 == 0 ? colours::ivory : tintAt(x)).withAlpha((0.1f + 0.5f * tw) * edgeFade(x)));
    }
}

// ---------------------------------------------------------------------------
// Fae frieze: two entwined vines drifting slowly sideways, swaying leaves, a few
// blossoms and curling tendrils, with fireflies blinking around them.
void MagicFrieze::paintVine(juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const float cy = r.getCentreY();
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    const float amp = r.getHeight() * 0.17f;
    const float wavelength = 300.0f;
    const float drift = phase * 9.0f;
    const float spacing = 34.0f;

    auto vineY = [&](float x, int vine)
    {
        return cy + amp * std::sin(twoPi * (x + drift) / wavelength + (vine == 0 ? 0.0f : juce::MathConstants<float>::pi));
    };
    auto vineSlope = [&](float x, int vine)
    {
        return amp * twoPi / wavelength * std::cos(twoPi * (x + drift) / wavelength + (vine == 0 ? 0.0f : juce::MathConstants<float>::pi));
    };
    auto edgeFade = [&](float x)
    {
        const float d = juce::jmin(x - r.getX(), r.getRight() - x);
        return juce::jlimit(0.0f, 1.0f, d / 90.0f);
    };
    auto tintAt = [&](float x)
    {
        const float t = juce::jlimit(0.0f, 1.0f, (x - r.getX()) / r.getWidth());
        return cyan.interpolatedWith(blue, juce::jmin(1.0f, t * 2.0f))
                   .interpolatedWith(pink, juce::jmax(0.0f, t * 2.0f - 1.0f));
    };
    auto hash = [](int n) { return std::fmod(std::abs(std::sin((float)n * 12.9898f) * 43758.5453f), 1.0f); };
    auto gradientFor = [&](float alpha)
    {
        juce::ColourGradient grad(cyan.withAlpha(0.0f), r.getX(), cy, pink.withAlpha(0.0f), r.getRight(), cy, false);
        grad.addColour(0.07, cyan.withAlpha(alpha));
        grad.addColour(0.5, blue.withAlpha(alpha));
        grad.addColour(0.93, pink.withAlpha(alpha));
        return grad;
    };

    // ---- Moonlight haze behind the vines ----
    {
        juce::ColourGradient haze(blue.withAlpha(0.0f), r.getX(), cy, blue.withAlpha(0.0f), r.getRight(), cy, false);
        haze.addColour(0.5, ivory.withAlpha(0.05f));
        g.setGradientFill(haze);
        g.fillRoundedRectangle(r.reduced(0.0f, r.getHeight() * 0.18f), r.getHeight() * 0.3f);
    }

    // ---- The two vines ----
    for (int vine = 0; vine < 2; ++vine)
    {
        juce::Path p;
        p.startNewSubPath(r.getX(), vineY(r.getX(), vine));
        for (float x = r.getX() + 4.0f; x <= r.getRight() + 0.1f; x += 4.0f)
            p.lineTo(x, vineY(x, vine));
        g.setGradientFill(gradientFor(vine == 0 ? 0.14f : 0.09f));
        g.strokePath(p, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setGradientFill(gradientFor(vine == 0 ? 0.85f : 0.55f));
        g.strokePath(p, juce::PathStrokeType(vine == 0 ? 1.7f : 1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- Leaves, blossoms and tendrils, fixed to the vines as they drift ----
    const int first = (int)std::floor((r.getX() + drift) / spacing) - 1;
    const int last = (int)std::ceil((r.getRight() + drift) / spacing) + 1;
    for (int n = first; n <= last; ++n)
    {
        const float x = (float)n * spacing - drift;
        const float fade = edgeFade(x);
        if (fade <= 0.0f)
            continue;
        const int vine = (n & 1);
        const float y = vineY(x, vine);
        const float along = std::atan(vineSlope(x, vine));
        const auto tint = tintAt(x);

        if (((n % 7) + 7) % 7 == 3)
        {
            // Blossom: five petals around a moonlit centre, turning slowly.
            const float spin = phase * 0.3f + (float)n;
            const auto petal = (n % 2 == 0 ? pink : blue);
            for (int k = 0; k < 5; ++k)
            {
                const float a = spin + (float)k * twoPi / 5.0f;
                juce::Path petalPath;
                petalPath.addEllipse(-1.8f, -5.2f, 3.6f, 5.2f);
                petalPath.applyTransform(juce::AffineTransform::rotation(a).translated(x, y));
                g.setColour(petal.withAlpha(0.75f * fade));
                g.fillPath(petalPath);
            }
            g.setColour(ivory.withAlpha(0.9f * fade));
            g.fillEllipse(x - 1.6f, y - 1.6f, 3.2f, 3.2f);
            continue;
        }

        const float side = ((n / 2) % 2 == 0) ? 1.0f : -1.0f;
        if (((n % 11) + 11) % 11 == 6)
        {
            // Tendril: a little spiral curling away from the vine.
            juce::Path curl;
            const float base = along + side * 1.2f;
            curl.startNewSubPath(x, y);
            for (float t = 0.05f; t <= 1.0f; t += 0.05f)
            {
                const float a = base + side * t * 4.2f;
                const float rad = 9.0f * (1.0f - t * 0.75f) * t;
                curl.lineTo(x + std::cos(base) * t * 9.0f + std::cos(a) * rad * 0.6f,
                            y + std::sin(base) * t * 9.0f + std::sin(a) * rad * 0.6f);
            }
            g.setColour(tint.withAlpha(0.7f * fade));
            g.strokePath(curl, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            continue;
        }

        // Leaf: an almond on a short stem, swaying gently.
        const float sway = 0.12f * std::sin(phase * 1.3f + (float)n * 0.9f);
        const float angle = along + side * (0.95f + sway);
        const float len = 8.0f + 4.0f * hash(n);
        juce::Path leaf;
        leaf.startNewSubPath(0.0f, 0.0f);
        leaf.quadraticTo(len * 0.5f, -len * 0.36f, len, 0.0f);
        leaf.quadraticTo(len * 0.5f, len * 0.36f, 0.0f, 0.0f);
        leaf.closeSubPath();
        const auto tf = juce::AffineTransform::rotation(angle).translated(x, y);
        leaf.applyTransform(tf);
        g.setColour(tint.withAlpha(0.6f * fade));
        g.fillPath(leaf);
        g.setColour(tint.withAlpha(0.9f * fade));
        g.strokePath(leaf, juce::PathStrokeType(0.9f));
        juce::Path rib;
        rib.startNewSubPath(0.0f, 0.0f);
        rib.lineTo(len * 0.8f, 0.0f);
        rib.applyTransform(tf);
        g.setColour(ivory.withAlpha(0.35f * fade));
        g.strokePath(rib, juce::PathStrokeType(0.6f));
    }

    // ---- Fireflies ----
    for (int i = 0; i < 18; ++i)
    {
        const float speed = 5.0f + (float)(i % 5) * 2.5f;
        const float x = r.getX() + 20.0f + std::fmod((float)i * 97.3f + phase * speed, r.getWidth() - 40.0f);
        const float y = cy + r.getHeight() * 0.42f * std::sin(phase * (0.6f + 0.07f * (float)i) + (float)i * 2.1f);
        const float blink = std::pow(0.5f + 0.5f * std::sin(phase * (1.1f + 0.17f * (float)i) + (float)i), 3.0f);
        const float fade = edgeFade(x);
        const auto c = (i % 3 == 0) ? cyan : blue;
        const float glowR = 2.5f + 5.0f * blink;
        juce::ColourGradient halo(c.withAlpha(0.55f * blink * fade), x, y, c.withAlpha(0.0f), x + glowR, y, true);
        g.setGradientFill(halo);
        g.fillEllipse(x - glowR, y - glowR, glowR * 2.0f, glowR * 2.0f);
        g.setColour(ivory.withAlpha((0.25f + 0.75f * blink) * fade));
        g.fillEllipse(x - 1.1f, y - 1.1f, 2.2f, 2.2f);
    }
}

// ---------------------------------------------------------------------------
ChannelSwitch::ChannelSwitch(juce::AudioProcessorValueTreeState& state, const juce::String& paramID)
{
    if (auto* param = state.getParameter(paramID))
    {
        attachment = std::make_unique<juce::ParameterAttachment>(*param,
            [this](float v) { channel = juce::roundToInt(v); repaint(); }, state.undoManager);
        attachment->sendInitialUpdate();
    }
    setTooltip("Amp channel: Arcane high-gain or Fae clean");
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ChannelSwitch::mouseDown(const juce::MouseEvent& e)
{
    const int wanted = e.x < getWidth() / 2 ? 0 : 1;
    if (attachment != nullptr && wanted != channel)
        attachment->setValueAsCompleteGesture((float)wanted);
}

void ChannelSwitch::paint(juce::Graphics& g)
{
    useThemeOf(*this);
    const auto r = getLocalBounds().toFloat().reduced(1.0f);
    const float radius = r.getHeight() * 0.5f;
    g.setColour(currentTheme().screen.withAlpha(0.9f));
    g.fillRoundedRectangle(r, radius);

    const auto mouse = getMouseXYRelative();
    const bool hover = isMouseOver(true);
    // Each half previews its own channel's colours.
    const juce::Colour accents[] = { arcaneTheme().cyan.interpolatedWith(arcaneTheme().pink, 0.5f),
                                     faeTheme().cyan.interpolatedWith(faeTheme().blue, 0.35f) };
    const char* labels[] = { "ARCANE  HIGH-GAIN", "FAE  CLEAN" };
    for (int half = 0; half < 2; ++half)
    {
        auto seg = r.withWidth(r.getWidth() * 0.5f).translated(half == 0 ? 0.0f : r.getWidth() * 0.5f, 0.0f).reduced(2.0f);
        const bool active = channel == half;
        const bool hot = hover && !active && seg.contains(mouse.toFloat());
        if (active)
        {
            juce::ColourGradient fill(accents[half].withAlpha(0.32f), seg.getX(), seg.getY(),
                                      accents[half].withAlpha(0.12f), seg.getX(), seg.getBottom(), false);
            g.setGradientFill(fill);
            g.fillRoundedRectangle(seg, seg.getHeight() * 0.5f);
            g.setColour(accents[half].withAlpha(0.9f));
            g.drawRoundedRectangle(seg, seg.getHeight() * 0.5f, 1.2f);
        }
        else if (hot)
        {
            g.setColour(accents[half].withAlpha(0.10f));
            g.fillRoundedRectangle(seg, seg.getHeight() * 0.5f);
        }
        const Rune rune = half == 0 ? Rune::gain : Rune::shimmer;
        const auto font11 = font(11.0f, true, 0.3f);
        drawRuneTitle(g, rune, labels[half], seg, font11,
                      active ? ivory : (hot ? ivory.withAlpha(0.8f) : muted.withAlpha(0.75f)), 14.0f);
    }
    g.setColour(muted.withAlpha(0.35f));
    g.drawRoundedRectangle(r, radius, 1.0f);
}

// ---------------------------------------------------------------------------
IconButton::IconButton(Icon i, const juce::String& tip) : juce::Button(tip), icon(i)
{
    setTooltip(tip);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void IconButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    useThemeOf(*this);
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(currentTheme().control.brighter(down ? 0.15f : highlighted ? 0.07f : 0.0f));
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
