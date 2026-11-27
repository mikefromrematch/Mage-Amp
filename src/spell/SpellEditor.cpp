#include "SpellEditor.h"
#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

using namespace mage::ui;
using namespace mage::ui::colours;

namespace spellui
{
namespace
{
    // Spell accents (Glitch, Pitch, Swell) follow the MAGE palette.
    juce::Colour accentOf(int spell) { return spell == 0 ? cyan : spell == 1 ? pink : violet; }

    juce::RangedAudioParameter& param(APVTS& s, const juce::String& id) { return *s.getParameter(id); }

    // The three spell glyphs, drawn on a 30-unit grid inside `r`.
    void drawGlyph(juce::Graphics& g, int spell, juce::Rectangle<float> r, juce::Colour c)
    {
        const float s = r.getWidth() / 30.0f;
        auto P = [&](float x, float y) { return juce::Point<float>(r.getX() + x * s, r.getY() + y * s); };
        juce::Path p;
        if (spell == 0)
        {
            p.startNewSubPath(P(15, 3)); p.lineTo(P(25, 13)); p.lineTo(P(19, 13)); p.lineTo(P(23, 18));
            p.lineTo(P(15, 27)); p.lineTo(P(5, 17)); p.lineTo(P(11, 17)); p.lineTo(P(7, 12)); p.closeSubPath();
        }
        else if (spell == 1)
        {
            p.startNewSubPath(P(15, 26)); p.lineTo(P(15, 5));
            p.startNewSubPath(P(8, 12)); p.lineTo(P(15, 5)); p.lineTo(P(22, 12));
            p.startNewSubPath(P(8, 19)); p.lineTo(P(15, 12)); p.lineTo(P(22, 19));
        }
        else
        {
            p.startNewSubPath(P(4, 22));
            p.cubicTo(P(10, 22), P(10, 8), P(15, 8));
            p.cubicTo(P(20, 8), P(20, 22), P(26, 22));
            p.startNewSubPath(P(9, 26)); p.lineTo(P(21, 26));
            g.setColour(c);
            g.fillEllipse(juce::Rectangle<float>(3.2f * s, 3.2f * s).withCentre(P(15, 4)));
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(1.8f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const char* spellNames[3] = { "GLITCH", "PITCH", "SWELL" };
    const char* spellTaglines[3] = { "STUTTER  -  REVERSE  -  JUMP", "HARMONY  -  WHAMMY  -  DOUBLE", "AUTO SWELL  -  REVERSE BLOOM" };
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
StepGrid::StepGrid(APVTS& s)
{
    for (int i = 0; i < 16; ++i)
    {
        attachments[(size_t)i] = std::make_unique<juce::ParameterAttachment>(param(s, SpellAudioProcessor::stepID(i)),
            [this, i](float v) { steps[(size_t)i] = juce::roundToInt(v); repaint(); }, s.undoManager);
        attachments[(size_t)i]->sendInitialUpdate();
    }
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

int StepGrid::cellAt(juce::Point<int> p) const
{
    return juce::jlimit(0, 15, p.x * 16 / juce::jmax(1, getWidth()));
}

void StepGrid::paint(juce::Graphics& g)
{
    useThemeOf(*this);
    const float gap = 4.0f, w = ((float)getWidth() - gap * 15.0f) / 16.0f, h = (float)getHeight();
    for (int i = 0; i < 16; ++i)
    {
        auto r = juce::Rectangle<float>((float)i * (w + gap), 0.0f, w, h);
        const int t = steps[(size_t)i];
        if (t > 0)
        {
            g.setColour(colours[t].withAlpha(0.35f));
            g.fillRoundedRectangle(r.expanded(1.5f), 5.0f);
            g.setColour(colours[t]);
            g.fillRoundedRectangle(r, 4.0f);
        }
        else
        {
            g.setColour(metal);
            g.fillRoundedRectangle(r, 4.0f);
            g.setColour(i % 4 == 0 ? dim.brighter(0.2f) : dim.withAlpha(0.7f));
            g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        }
        if (i == playStep)
        {
            g.setColour(ivory);
            g.drawRoundedRectangle(r.expanded(2.5f), 6.0f, 1.8f);
        }
    }
}

void StepGrid::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        return;
    const int i = cellAt(e.getPosition());
    const int next = e.mods.isShiftDown() ? (steps[(size_t)i] + 3) % 4 : (steps[(size_t)i] + 1) % 4;
    attachments[(size_t)i]->setValueAsCompleteGesture((float)next);
}

// ---------------------------------------------------------------------------
IntervalRow::IntervalRow(APVTS& s, juce::Colour a) : accent(a)
{
    attachment = std::make_unique<juce::ParameterAttachment>(param(s, "pitchShift"),
        [this](float v) { current = juce::roundToInt(v); repaint(); }, s.undoManager);
    attachment->sendInitialUpdate();
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void IntervalRow::paint(juce::Graphics& g)
{
    useThemeOf(*this);
    const float gap = 5.0f, w = ((float)getWidth() - gap * 6.0f) / 7.0f, h = (float)getHeight();
    for (int i = 0; i < 7; ++i)
    {
        auto r = juce::Rectangle<float>((float)i * (w + gap), 0.0f, w, h);
        const bool sel = intervals[i] == current;
        g.setColour(sel ? accent : metal);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(sel ? accent : dim.brighter(0.2f));
        g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);
        g.setColour(sel ? bg : ivory);
        g.setFont(font(14.0f, true));
        g.drawText((intervals[i] > 0 ? "+" : "") + juce::String(intervals[i]), r, juce::Justification::centred, false);
    }
}

void IntervalRow::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        return;
    const int i = juce::jlimit(0, 6, e.x * 7 / juce::jmax(1, getWidth()));
    attachment->setValueAsCompleteGesture((float)intervals[i]);
}

// ---------------------------------------------------------------------------
namespace
{
    // Corners of the morph triangle in pad units (0..1), matching mage::spell::morphWeights.
    const juce::Point<float> cornerGlitch { 0.5f, 0.09f }, cornerPitch { 0.91f, 0.88f }, cornerSwell { 0.09f, 0.88f };
}

MorphPad::MorphPad(APVTS& s)
{
    ax = std::make_unique<juce::ParameterAttachment>(param(s, "morphX"), [this](float v) { x = v; repaint(); if (onMove) onMove(); }, s.undoManager);
    ay = std::make_unique<juce::ParameterAttachment>(param(s, "morphY"), [this](float v) { y = v; repaint(); if (onMove) onMove(); }, s.undoManager);
    ax->sendInitialUpdate();
    ay->sendInitialUpdate();
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

void MorphPad::paint(juce::Graphics& g)
{
    useThemeOf(*this);
    const auto b = getLocalBounds().toFloat();
    g.setColour(currentTheme().screen);
    g.fillRoundedRectangle(b, 10.0f);
    g.setColour(dim);
    g.drawRoundedRectangle(b.reduced(0.5f), 10.0f, 1.0f);
    auto at = [&](juce::Point<float> p) { return juce::Point<float>(b.getX() + p.x * b.getWidth(), b.getY() + p.y * b.getHeight()); };
    juce::Path tri;
    tri.startNewSubPath(at(cornerGlitch)); tri.lineTo(at(cornerPitch)); tri.lineTo(at(cornerSwell)); tri.closeSubPath();
    g.setColour(dim.brighter(0.3f));
    g.strokePath(tri, juce::PathStrokeType(1.5f));
    const auto centre = at({ 0.5f, 0.62f });
    g.setColour(dim.withAlpha(0.6f));
    for (auto c : { cornerGlitch, cornerPitch, cornerSwell })
        g.drawLine({ centre, at(c) }, 1.0f);
    const juce::Point<float> corners[3] = { cornerGlitch, cornerPitch, cornerSwell };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour(colours[i]);
        g.fillEllipse(juce::Rectangle<float>(9.0f, 9.0f).withCentre(at(corners[i])));
    }
    const auto puck = at({ x, y });
    for (int k = 3; k > 0; --k)
    {
        g.setColour(violet.withAlpha(0.12f * (float)k));
        g.fillEllipse(juce::Rectangle<float>(26.0f + 8.0f * (float)k, 26.0f + 8.0f * (float)k).withCentre(puck));
    }
    g.setColour(ivory);
    g.fillEllipse(juce::Rectangle<float>(24.0f, 24.0f).withCentre(puck));
}

void MorphPad::moveTo(juce::Point<float> p)
{
    const float nx = juce::jlimit(0.03f, 0.97f, p.x / (float)juce::jmax(1, getWidth()));
    const float ny = juce::jlimit(0.03f, 0.97f, p.y / (float)juce::jmax(1, getHeight()));
    ax->setValueAsPartOfGesture(nx);
    ay->setValueAsPartOfGesture(ny);
}

void MorphPad::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        return;
    ax->beginGesture();
    ay->beginGesture();
    moveTo(e.position);
}

void MorphPad::mouseDrag(const juce::MouseEvent& e)
{
    if (!e.mods.isPopupMenu())
        moveTo(e.position);
}

void MorphPad::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        return;
    ax->endGesture();
    ay->endGesture();
}

void MorphPad::mouseDoubleClick(const juce::MouseEvent&)
{
    ax->setValueAsCompleteGesture(0.5f);   // back to the centre: every spell at full strength
    ay->setValueAsCompleteGesture(0.62f);
}

// ---------------------------------------------------------------------------
CastButton::CastButton(APVTS& s) : state(s)
{
    attachment = std::make_unique<juce::ParameterAttachment>(param(s, "cast"), [this](float v) { on = v > 0.5f; repaint(); }, nullptr);
    attachment->sendInitialUpdate();
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void CastButton::paint(juce::Graphics& g)
{
    useThemeOf(*this);
    const auto b = getLocalBounds().toFloat();
    const float d = juce::jmin(b.getWidth(), b.getHeight()) - 24.0f;
    const auto circle = juce::Rectangle<float>(d, d).withCentre(b.getCentre());
    const bool lit = casting || on;
    for (int k = 4; k > 0; --k)
    {
        g.setColour(accent.withAlpha((lit ? 0.10f : 0.035f) * (float)k));
        g.fillEllipse(circle.expanded(3.0f * (float)k));
    }
    g.setColour(lit ? accent : currentTheme().control);
    g.fillEllipse(circle);
    g.setColour(accent);
    g.drawEllipse(circle.reduced(1.0f), 2.0f);

    // Sigil above the word. When lit, the sigil is drawn dark on the glowing face.
    auto sigilArea = juce::Rectangle<float>(d * 0.42f, d * 0.36f).withCentre({ circle.getCentreX(), circle.getCentreY() - d * 0.12f });
    if (lit)
    {
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(circle.toNearestInt());
        juce::Image tmp(juce::Image::ARGB, (int)sigilArea.getWidth() + 2, (int)sigilArea.getHeight() + 2, true);
        {
            juce::Graphics tg(tmp);
            drawLogo(tg, Logo::spellSigil, tmp.getBounds().toFloat());
        }
        g.setColour(bg);
        g.drawImageAt(tmp.convertedToFormat(juce::Image::SingleChannel), (int)sigilArea.getX(), (int)sigilArea.getY(), true);
    }
    else
    {
        drawLogo(g, Logo::spellSigil, sigilArea);
    }
    g.setColour(lit ? bg : ivory);
    g.setFont(font(d * 0.16f, true, 0.24f));
    g.drawText("CAST", juce::Rectangle<float>(circle.getX(), circle.getCentreY() + d * 0.1f, circle.getWidth(), d * 0.2f),
               juce::Justification::centred, false);
}

void CastButton::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        return;
    const int mode = (int)state.getRawParameterValue("castMode")->load();
    if (mode == 1)
        attachment->setValueAsCompleteGesture(on ? 0.0f : 1.0f);   // Latch
    else
        attachment->setValueAsCompleteGesture(1.0f);
}

void CastButton::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        return;
    const int mode = (int)state.getRawParameterValue("castMode")->load();
    if (mode != 1)
        attachment->setValueAsCompleteGesture(0.0f);
}

// ---------------------------------------------------------------------------
Panel::Panel(SpellAudioProcessor& p)
    : processor(p),
      glitchOn(p.state, "glitchOn", "ON", arcaneTheme().cyan),
      pitchOn(p.state, "pitchOn", "ON", arcaneTheme().pink),
      swellOn(p.state, "swellOn", "ON", arcaneTheme().violet),
      glitchMode(p.state, "glitchMode", arcaneTheme().cyan),
      pitchMode(p.state, "pitchMode", arcaneTheme().pink),
      swellMode(p.state, "swellMode", arcaneTheme().violet),
      glitchSlice(p.state, "glitchSlice", "SLICE", arcaneTheme().cyan),
      glitchDensity(p.state, "glitchDensity", "DENSITY", arcaneTheme().cyan),
      glitchJump(p.state, "glitchJump", "JUMP", arcaneTheme().cyan),
      glitchMix(p.state, "glitchMix", "MIX", arcaneTheme().cyan),
      pitchBend(p.state, "pitchWhammy", "BEND", arcaneTheme().pink),
      pitchGlide(p.state, "pitchGlide", "GLIDE", arcaneTheme().pink),
      pitchDetune(p.state, "pitchDetune", "DETUNE", arcaneTheme().pink),
      pitchMix(p.state, "pitchMix", "MIX", arcaneTheme().pink),
      swellAttack(p.state, "swellAttack", "ATTACK", arcaneTheme().violet),
      swellBloom(p.state, "swellBloom", "BLOOM", arcaneTheme().violet),
      swellSize(p.state, "swellSize", "SIZE", arcaneTheme().violet),
      swellMix(p.state, "swellMix", "MIX", arcaneTheme().violet),
      steps(p.state),
      intervals(p.state, arcaneTheme().pink),
      morph(p.state),
      cast(p.state),
      castMode(p.state, "castMode", arcaneTheme().violet),
      mix(p.state, "mix", "MIX", arcaneTheme().cyan),
      output(p.state, "output", "OUTPUT", arcaneTheme().cyan)
{
    useTheme(getTheme());
    for (auto* c : std::initializer_list<juce::Component*> {
             &glitchOn, &pitchOn, &swellOn, &glitchMode, &pitchMode, &swellMode,
             &glitchSlice, &glitchDensity, &glitchJump, &glitchMix, &pitchBend, &pitchGlide, &pitchDetune, &pitchMix,
             &swellAttack, &swellBloom, &swellSize, &swellMix, &steps, &intervals, &morph, &cast, &castMode, &reorder,
             &mix, &output, &presetBox, &prevPreset, &nextPreset, &savePreset })
        addAndMakeVisible(c);

    steps.colours[0] = metal; steps.colours[1] = cyan; steps.colours[2] = pink; steps.colours[3] = violet;
    morph.colours[0] = cyan; morph.colours[1] = pink; morph.colours[2] = violet;
    morph.onMove = [this] { repaint(spellArea(3).toNearestInt()); };
    cast.accent = violet;
    steps.setTooltip("Glitch pattern: click a step to cycle Off, Stutter, Reverse, Jump (shift-click goes back)");
    intervals.setTooltip("Pitch interval in semitones");
    morph.setTooltip("Blend the spells: drag the orb toward a corner (double-click to recentre)");
    cast.setTooltip("CAST: glitch every step. Hold, Latch or Burst (one bar). Right-click for MIDI Learn");

    learnables = {
        { &glitchOn, "glitchOn" }, { &pitchOn, "pitchOn" }, { &swellOn, "swellOn" },
        { &glitchMode, "glitchMode" }, { &pitchMode, "pitchMode" }, { &swellMode, "swellMode" },
        { &glitchSlice, "glitchSlice" }, { &glitchDensity, "glitchDensity" }, { &glitchJump, "glitchJump" }, { &glitchMix, "glitchMix" },
        { &pitchBend, "pitchWhammy" }, { &pitchGlide, "pitchGlide" }, { &pitchDetune, "pitchDetune" }, { &pitchMix, "pitchMix" },
        { &swellAttack, "swellAttack" }, { &swellBloom, "swellBloom" }, { &swellSize, "swellSize" }, { &swellMix, "swellMix" },
        { &intervals, "pitchShift" }, { &morph, "morphX" }, { &cast, "cast" }, { &castMode, "castMode" },
        { &reorder, "order" }, { &mix, "mix" }, { &output, "output" } };
    for (auto& [control, id] : learnables)
        control->addMouseListener(this, true);

    // Order and cast-mode readouts.
    orderAttachment = std::make_unique<juce::ParameterAttachment>(param(p.state, "order"),
        [this](float v) { order = juce::roundToInt(v); repaint(spellArea(5).toNearestInt()); }, p.state.undoManager);
    orderAttachment->sendInitialUpdate();
    castModeAttachment = std::make_unique<juce::ParameterAttachment>(param(p.state, "castMode"),
        [this](float v) { castModeIndex = juce::roundToInt(v); repaint(spellArea(4).toNearestInt()); }, nullptr);
    castModeAttachment->sendInitialUpdate();
    reorder.setTooltip("Rotate the spell order");
    reorder.onClick = [this] { orderAttachment->setValueAsCompleteGesture((float)((order + 1) % 3 + (order >= 3 ? 3 : 0))); };

    // Presets.
    presetBox.setJustificationType(juce::Justification::centred);
    presetBox.setTooltip("Presets");
    presetBox.onAboutToShow = [this] { rebuildPresetList(); };
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0)
            processor.presets.loadPreset(id - 1);
        shownPreset.clear();
    };
    prevPreset.onClick = [this] { processor.presets.loadPrevious(); };
    nextPreset.onClick = [this] { processor.presets.loadNext(); };
    savePreset.onClick = [this] { savePresetDialog(); };
    rebuildPresetList();
    shownBpm = p.hostBpm.load();
}

juce::Rectangle<float> Panel::spellArea(int i) const
{
    // 0..2: spell panels, 3: morph, 4: cast, 5: order/output.
    if (i < 3)
        return { 28.0f + (float)i * 414.0f, 112.0f, 396.0f, 360.0f };
    if (i == 3) return { 28.0f, 492.0f, 380.0f, 240.0f };
    if (i == 4) return { 428.0f, 492.0f, 424.0f, 240.0f };
    return { 872.0f, 492.0f, 380.0f, 240.0f };
}

void Panel::resized()
{
    auto i = [](float x, float y, float w, float h) { return juce::Rectangle<float>(x, y, w, h).toNearestInt(); };

    prevPreset.setBounds(i(460, 40, 40, 40));
    presetBox.setBounds(i(506, 40, 268, 40));
    nextPreset.setBounds(i(780, 40, 40, 40));
    savePreset.setBounds(i(830, 40, 40, 40));

    LedToggle* ons[3] = { &glitchOn, &pitchOn, &swellOn };
    ModePills* modes[3] = { &glitchMode, &pitchMode, &swellMode };
    Knob* knobs[3][4] = { { &glitchSlice, &glitchDensity, &glitchJump, &glitchMix },
                          { &pitchBend, &pitchGlide, &pitchDetune, &pitchMix },
                          { &swellAttack, &swellBloom, &swellSize, &swellMix } };
    for (int s = 0; s < 3; ++s)
    {
        const auto r = spellArea(s);
        ons[s]->setBounds(i(r.getRight() - 96.0f, r.getY() + 26.0f, 72.0f, 36.0f));
        modes[s]->setBounds(i(r.getX() + 24.0f, r.getY() + 92.0f, s == 2 ? 200.0f : 300.0f, 30.0f));
        const float kw = (r.getWidth() - 48.0f) / 4.0f;
        for (int k = 0; k < 4; ++k)
            knobs[s][k]->setBounds(i(r.getX() + 24.0f + kw * (float)k, r.getY() + 136.0f, kw, 112.0f));
    }
    steps.setBounds(i(spellArea(0).getX() + 24.0f, spellArea(0).getY() + 266.0f, spellArea(0).getWidth() - 48.0f, 34.0f));
    intervals.setBounds(i(spellArea(1).getX() + 24.0f, spellArea(1).getY() + 266.0f, spellArea(1).getWidth() - 48.0f, 34.0f));

    const auto m = spellArea(3);
    morph.setBounds(i(m.getX() + 20.0f, m.getY() + 20.0f, 200.0f, 200.0f));
    const auto c = spellArea(4);
    cast.setBounds(i(c.getCentreX() - 90.0f, c.getY() + 6.0f, 180.0f, 180.0f));
    castMode.setBounds(i(c.getCentreX() - 120.0f, c.getY() + 186.0f, 240.0f, 28.0f));
    const auto o = spellArea(5);
    reorder.setBounds(i(o.getX() + 22.0f, o.getY() + 96.0f, 120.0f, 32.0f));
    mix.setBounds(i(o.getX() + 22.0f, o.getY() + 134.0f, 78.0f, 84.0f));
    output.setBounds(i(o.getX() + 108.0f, o.getY() + 134.0f, 78.0f, 84.0f));
}

void Panel::paint(juce::Graphics& g)
{
    useTheme(getTheme());
    const auto& t = getTheme();
    g.fillAll(bg);
    juce::ColourGradient aura(cyan.withAlpha(0.07f), 0.0f, 0.0f, violet.withAlpha(0.08f), (float)designWidth, (float)designHeight, false);
    g.setGradientFill(aura);
    g.fillRect(getLocalBounds());

    // ---- Header ----
    const juce::Rectangle<float> header { 28.0f, 28.0f, 1224.0f, 64.0f };
    g.setColour(t.header);
    g.fillRoundedRectangle(header, 12.0f);
    g.setColour(t.headerLine);
    g.drawRoundedRectangle(header.reduced(0.5f), 12.0f, 1.0f);
    drawLogo(g, Logo::wordmark, { 48.0f, 32.0f, 70.0f, 56.0f });
    g.setColour(t.divider);
    g.fillRect(132.0f, 42.0f, 1.0f, 36.0f);
    drawLogo(g, Logo::spellSigil, { 146.0f, 36.0f, 48.0f, 48.0f });
    g.setColour(violet);
    g.setFont(font(22.0f, true, 0.3f));
    g.drawText("SPELL", juce::Rectangle<float>(204.0f, 40.0f, 140.0f, 40.0f), juce::Justification::centredLeft, false);

    // Tempo readout.
    const juce::Rectangle<float> sync { 1062.0f, 40.0f, 170.0f, 40.0f };
    g.setColour(t.headerLine);
    g.drawRoundedRectangle(sync, 8.0f, 1.0f);
    g.setColour(cyan);
    g.fillEllipse(sync.getX() + 14.0f, sync.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setFont(font(12.0f, true, 0.12f));
    g.setColour(muted);
    g.drawText("SYNC", juce::Rectangle<float>(sync.getX() + 30.0f, sync.getY(), 44.0f, sync.getHeight()), juce::Justification::centredLeft, false);
    g.setColour(ivory);
    g.setFont(font(16.0f, true));
    g.drawText(juce::String(juce::roundToInt(shownBpm)) + " BPM", juce::Rectangle<float>(sync.getX() + 72.0f, sync.getY(), 90.0f, sync.getHeight()),
               juce::Justification::centredLeft, false);

    // ---- Spell panels ----
    const auto& proc = processor;
    for (int s = 0; s < 3; ++s)
    {
        const auto r = spellArea(s);
        const auto a = accentOf(s);
        const char* onIDs[3] = { "glitchOn", "pitchOn", "swellOn" };
        const bool on = proc.state.getRawParameterValue(onIDs[s])->load() > 0.5f;
        drawPanel(g, r, on ? a : a.withMultipliedSaturation(0.35f).withMultipliedBrightness(0.6f), 14.0f);
        const juce::Rectangle<float> runeBox { r.getX() + 24.0f, r.getY() + 22.0f, 48.0f, 48.0f };
        g.setColour(t.screen);
        g.fillRoundedRectangle(runeBox, 10.0f);
        g.setColour(a.withAlpha(0.5f));
        g.drawRoundedRectangle(runeBox, 10.0f, 1.0f);
        drawGlyph(g, s, runeBox.reduced(9.0f), on ? a : muted);
        g.setColour(on ? ivory : muted);
        g.setFont(font(22.0f, true, 0.18f));
        g.drawText(spellNames[s], juce::Rectangle<float>(r.getX() + 86.0f, r.getY() + 22.0f, 200.0f, 28.0f), juce::Justification::centredLeft, false);
        g.setColour(muted);
        g.setFont(font(11.0f, true, 0.12f));
        g.drawText(spellTaglines[s], juce::Rectangle<float>(r.getX() + 86.0f, r.getY() + 50.0f, 220.0f, 18.0f), juce::Justification::centredLeft, false);
    }

    // Strip captions.
    g.setFont(font(11.0f, true, 0.1f));
    {
        const auto r = spellArea(0);
        const juce::Colour cols[3] = { cyan, pink, violet };
        const char* names[3] = { "STUTTER", "REVERSE", "JUMP" };
        float x = r.getX() + 24.0f;
        for (int k = 0; k < 3; ++k)
        {
            g.setColour(cols[k]);
            g.fillRoundedRectangle(x, r.getY() + 314.0f, 8.0f, 8.0f, 2.0f);
            g.setColour(muted);
            g.drawText(names[k], juce::Rectangle<float>(x + 13.0f, r.getY() + 308.0f, 80.0f, 20.0f), juce::Justification::centredLeft, false);
            x += k == 0 ? 82.0f : 86.0f;
        }
        g.drawText("CLICK A STEP", juce::Rectangle<float>(r.getRight() - 124.0f, r.getY() + 308.0f, 100.0f, 20.0f), juce::Justification::centredRight, false);
    }
    {
        const auto r = spellArea(1);
        g.setColour(muted);
        g.drawText("INTERVAL (SEMITONES)", juce::Rectangle<float>(r.getX() + 24.0f, r.getY() + 308.0f, 180.0f, 20.0f), juce::Justification::centredLeft, false);
        g.drawText("BEND = EXPRESSION", juce::Rectangle<float>(r.getRight() - 184.0f, r.getY() + 308.0f, 160.0f, 20.0f), juce::Justification::centredRight, false);
    }
    {
        const auto r = spellArea(2);
        const juce::Rectangle<float> env { r.getX() + 24.0f, r.getY() + 266.0f, r.getWidth() - 48.0f, 34.0f };
        juce::Path p;
        const float w = env.getWidth(), b = env.getBottom(), top = env.getY() + 3.0f;
        p.startNewSubPath(env.getX(), b);
        p.cubicTo(env.getX() + w * 0.12f, b, env.getX() + w * 0.18f, top, env.getX() + w * 0.32f, top);
        p.lineTo(env.getX() + w * 0.44f, top);
        p.cubicTo(env.getX() + w * 0.5f, top, env.getX() + w * 0.51f, b, env.getX() + w * 0.56f, b);
        p.cubicTo(env.getX() + w * 0.64f, b, env.getX() + w * 0.66f, top + 2.0f, env.getX() + w * 0.77f, top + 2.0f);
        p.lineTo(env.getX() + w * 0.86f, top + 2.0f);
        p.cubicTo(env.getX() + w * 0.92f, top + 2.0f, env.getX() + w * 0.94f, b, env.getRight(), b);
        g.setColour(dim);
        g.drawLine(env.getX(), b, env.getRight(), b, 1.0f);
        g.setColour(violet);
        g.strokePath(p, juce::PathStrokeType(2.0f));
        g.setColour(muted);
        g.drawText("SWELL ENVELOPE", juce::Rectangle<float>(r.getX() + 24.0f, r.getY() + 308.0f, 180.0f, 20.0f), juce::Justification::centredLeft, false);
        g.drawText("RETRIGGERS ON EACH PICK", juce::Rectangle<float>(r.getRight() - 204.0f, r.getY() + 308.0f, 180.0f, 20.0f), juce::Justification::centredRight, false);
    }

    // ---- Morph ----
    {
        const auto r = spellArea(3);
        drawPanel(g, r, violet.withMultipliedBrightness(0.7f), 14.0f);
        g.setColour(muted);
        g.setFont(font(12.0f, true, 0.18f));
        g.drawText("MORPH", juce::Rectangle<float>(r.getX() + 238.0f, r.getY() + 22.0f, 120.0f, 18.0f), juce::Justification::centredLeft, false);
        const auto w = mage::spell::morphWeights(morph.position().x, morph.position().y);
        const juce::Colour cols[3] = { cyan, pink, violet };
        for (int k = 0; k < 3; ++k)
        {
            const float y = r.getY() + 54.0f + (float)k * 44.0f;
            g.setColour(muted);
            g.setFont(font(12.0f, true, 0.08f));
            g.drawText(spellNames[k], juce::Rectangle<float>(r.getX() + 238.0f, y, 80.0f, 16.0f), juce::Justification::centredLeft, false);
            g.setColour(ivory);
            g.drawText(juce::String(juce::roundToInt(w[(size_t)k] * 100.0f)) + "%", juce::Rectangle<float>(r.getX() + 300.0f, y, 58.0f, 16.0f),
                       juce::Justification::centredRight, false);
            g.setColour(t.track);
            g.fillRoundedRectangle(r.getX() + 238.0f, y + 21.0f, 120.0f, 4.0f, 2.0f);
            g.setColour(cols[k]);
            g.fillRoundedRectangle(r.getX() + 238.0f, y + 21.0f, 120.0f * w[(size_t)k], 4.0f, 2.0f);
        }
        g.setColour(muted);
        g.setFont(font(11.0f, false));
        g.drawText("Drag the orb", juce::Rectangle<float>(r.getX() + 238.0f, r.getY() + 196.0f, 120.0f, 16.0f), juce::Justification::centredLeft, false);
    }

    // ---- Cast ----
    drawPanel(g, spellArea(4), violet, 14.0f);

    // ---- Order + output ----
    {
        const auto r = spellArea(5);
        drawPanel(g, r, cyan.withMultipliedBrightness(0.7f), 14.0f);
        g.setColour(muted);
        g.setFont(font(12.0f, true, 0.18f));
        g.drawText("SPELL ORDER", juce::Rectangle<float>(r.getX() + 22.0f, r.getY() + 20.0f, 200.0f, 18.0f), juce::Justification::centredLeft, false);
        float x = r.getX() + 22.0f;
        for (int k = 0; k < 3; ++k)
        {
            const int id = SpellAudioProcessor::orders[order][k];
            const auto chip = juce::Rectangle<float>(x, r.getY() + 48.0f, 90.0f, 34.0f);
            g.setColour(t.screen);
            g.fillRoundedRectangle(chip, 8.0f);
            g.setColour(accentOf(id).withAlpha(0.6f));
            g.drawRoundedRectangle(chip.reduced(0.5f), 8.0f, 1.0f);
            g.setColour(accentOf(id));
            g.setFont(font(13.0f, true, 0.12f));
            g.drawText(spellNames[id], chip, juce::Justification::centred, false);
            if (k < 2)
            {
                juce::Path arrow;
                const float ax = chip.getRight() + 5.0f, ay = chip.getCentreY();
                arrow.startNewSubPath(ax, ay); arrow.lineTo(ax + 12.0f, ay);
                arrow.startNewSubPath(ax + 7.0f, ay - 4.0f); arrow.lineTo(ax + 12.0f, ay); arrow.lineTo(ax + 7.0f, ay + 4.0f);
                g.setColour(muted);
                g.strokePath(arrow, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            x += 116.0f;
        }
        g.setColour(muted);
        g.setFont(font(11.0f, false));
        g.drawText("Right-click any control", juce::Rectangle<float>(r.getRight() - 172.0f, r.getBottom() - 54.0f, 150.0f, 16.0f), juce::Justification::centredRight, false);
        g.drawText("for MIDI Learn", juce::Rectangle<float>(r.getRight() - 172.0f, r.getBottom() - 36.0f, 150.0f, 16.0f), juce::Justification::centredRight, false);
    }
}

void Panel::refresh()
{
    const float bpm = processor.hostBpm.load();
    if (std::abs(bpm - shownBpm) > 0.4f)
    {
        shownBpm = bpm;
        repaint(1062, 40, 170, 40);
    }
    steps.setPlayStep(processor.glitchStep.load());
    cast.setCasting(processor.casting.load());

    const auto name = processor.presets.getCurrentName();
    const bool modified = processor.presets.isModified();
    if (name != shownPreset || modified != shownModified)
    {
        shownPreset = name;
        shownModified = modified;
        presetBox.setText(modified ? name + " *" : name, juce::dontSendNotification);
        repaint();   // spell on/off states may have changed with the preset
    }
    const char* onIDs[3] = { "glitchOn", "pitchOn", "swellOn" };
    for (int s = 0; s < 3; ++s)
    {
        const float v = processor.state.getRawParameterValue(onIDs[s])->load();
        if (std::abs(v - lastOn[s]) > 0.25f) { lastOn[s] = v; repaint(spellArea(s).expanded(10.0f).toNearestInt()); }
    }
    updateMidiHighlight();
}

void Panel::rebuildPresetList()
{
    auto& pm = processor.presets;
    presetBox.clear(juce::dontSendNotification);
    const auto names = pm.getPresetNames();
    for (int n = 0; n < names.size(); ++n)
    {
        if (n == pm.getNumFactoryPresets())
        {
            presetBox.addSeparator();
            presetBox.addSectionHeading("USER");
        }
        presetBox.addItem(names[n], n + 1);
    }
    presetBox.setSelectedId(pm.getCurrentIndex() + 1, juce::dontSendNotification);
}

void Panel::savePresetDialog()
{
    const auto current = processor.presets.getCurrentName();
    saveDialog = std::make_unique<juce::AlertWindow>("Save Preset", "Name your preset:", juce::MessageBoxIconType::NoIcon, this);
    saveDialog->setLookAndFeel(&getLookAndFeel());
    saveDialog->addTextEditor("name", processor.presets.isFactory(current) ? current + " (edit)" : current);
    saveDialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    saveDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    saveDialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [safe = juce::Component::SafePointer<Panel>(this)](int result)
        {
            if (safe == nullptr || safe->saveDialog == nullptr)
                return;
            const auto name = safe->saveDialog->getTextEditorContents("name");
            if (result == 1 && !safe->processor.presets.saveUserPreset(name))
                juce::AlertWindow::showAsync(juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                    "Couldn't save preset", "Choose a name that isn't empty and isn't a factory preset name."), nullptr);
            juce::MessageManager::callAsync([safe] { if (safe != nullptr) safe->saveDialog.reset(); });
        }), false);
}

// ---------------------------------------------------------------------------
// MIDI Learn (same behaviour as the amp: right-click a control).
void Panel::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isPopupMenu())
        return;
    for (auto* c = e.originalComponent; c != nullptr && c != this; c = c->getParentComponent())
        for (auto& [control, id] : learnables)
            if (control == c)
            {
                showMidiMenu(*control, id);
                return;
            }
}

juce::Component* Panel::findLearnable(const juce::String& id) const
{
    if (id.isEmpty())
        return nullptr;
    for (auto& [control, pid] : learnables)
        if (pid == id)
            return control;
    return nullptr;
}

void Panel::showMidiMenu(juce::Component& control, const juce::String& paramID)
{
    auto& learn = processor.midiLearn;
    const int cc = learn.getCCFor(paramID);
    const bool learningThis = learn.getLearningParam() == paramID;
    juce::PopupMenu menu;
    if (auto* p = processor.state.getParameter(paramID))
        menu.addSectionHeader(p->getName(40) + (cc >= 0 ? "  -  MIDI CC " + juce::String(cc) : juce::String()));
    if (learningThis)
        menu.addItem(1, "Cancel MIDI Learn");
    else
        menu.addItem(2, cc >= 0 ? "Re-learn MIDI" : "MIDI Learn");
    menu.addItem(3, "Clear MIDI", cc >= 0);
    if (learn.hasAnyMapping())
    {
        menu.addSeparator();
        menu.addItem(4, "Clear all MIDI mappings");
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&control),
        [safe = juce::Component::SafePointer<Panel>(this), paramID](int result)
        {
            if (safe == nullptr || result == 0)
                return;
            auto& ml = safe->processor.midiLearn;
            switch (result)
            {
                case 1: ml.cancelLearning(); break;
                case 2: ml.startLearning(paramID); break;
                case 3: ml.clear(paramID); ml.saveAsDefault(); break;
                case 4: ml.clearAll(); ml.saveAsDefault(); break;
                default: break;
            }
            safe->updateMidiHighlight();
        });
}

juce::Rectangle<int> Panel::midiBadgeArea(const juce::Component& c) const
{
    const auto b = c.getBounds();
    return { b.getCentreX() - 92, b.getY() - 26, 184, 22 };
}

void Panel::updateMidiHighlight()
{
    auto& learn = processor.midiLearn;
    if (const int learned = learn.lastLearnedParam.exchange(-1); learned >= 0)
    {
        flashParam = learn.getParamID(learned);
        flashFrames = 50;
        learn.saveAsDefault();
    }
    else if (flashFrames > 0)
    {
        --flashFrames;
    }
    const auto learning = learn.getLearningParam();
    const auto shown = learning.isNotEmpty() ? learning : (flashFrames > 0 ? flashParam : juce::String());
    juce::Rectangle<int> area;
    if (auto* c = findLearnable(shown); c != nullptr && c->isVisible())
        area = c->getBounds().expanded(8).getUnion(midiBadgeArea(*c)).expanded(4);
    if (!highlightArea.isEmpty())
        repaint(highlightArea);
    highlightParam = shown;
    highlightArea = area;
    if (!area.isEmpty())
        repaint(area);
}

void Panel::paintOverChildren(juce::Graphics& g)
{
    auto* c = findLearnable(highlightParam);
    if (c == nullptr || !c->isVisible())
        return;
    useTheme(getTheme());
    const bool learning = processor.midiLearn.getLearningParam() == highlightParam;
    const float t = (float)(juce::Time::getMillisecondCounter() % 1200) / 1200.0f;
    const float pulse = learning ? 0.55f + 0.45f * std::sin(t * juce::MathConstants<float>::twoPi) : (float)flashFrames / 50.0f;
    const auto colour = learning ? pink : cyan;
    g.setColour(colour.withAlpha(0.9f * pulse));
    g.drawRoundedRectangle(c->getBounds().toFloat().expanded(6.0f), 10.0f, 2.0f);
    const auto badge = midiBadgeArea(*c).toFloat();
    g.setColour(bg.withAlpha(0.92f));
    g.fillRoundedRectangle(badge, 11.0f);
    g.setColour(colour.withAlpha(0.6f + 0.4f * pulse));
    g.drawRoundedRectangle(badge.reduced(0.5f), 11.0f, 1.2f);
    g.setFont(font(12.0f, true, 0.12f));
    const int cc = processor.midiLearn.getCCFor(highlightParam);
    g.drawText(learning ? juce::String("MOVE A MIDI CONTROL") : "MIDI CC " + juce::String(cc), badge, juce::Justification::centred, false);
}
} // namespace spellui

// ---------------------------------------------------------------------------
SpellEditor::SpellEditor(SpellAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p), panel(p)
{
    lookAndFeel.applyTheme(arcaneTheme());
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(panel);
    setResizable(true, true);
    setResizeLimits(960, 570, 2560, 1520);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio((double)spellui::Panel::designWidth / spellui::Panel::designHeight);
    const int w = juce::jlimit(960, 2560, (int)p.state.state.getProperty("uiWidth", 1280));
    setSize(w, juce::roundToInt(w * (double)spellui::Panel::designHeight / spellui::Panel::designWidth));
    startTimerHz(30);

   #if JucePlugin_Build_Standalone
    // JUCE's standalone app mutes the input by default; unmute it once (see the amp).
    if (p.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            if (holder->settings != nullptr && !holder->settings->getBoolValue("mageInputUnmuted", false))
            {
                holder->getMuteInputValue().setValue(false);
                holder->settings->setValue("mageInputUnmuted", true);
            }
   #endif
}

SpellEditor::~SpellEditor()
{
    setLookAndFeel(nullptr);
}

void SpellEditor::paint(juce::Graphics& g)
{
    useTheme(arcaneTheme());
    g.fillAll(bg);
}

void SpellEditor::resized()
{
    using spellui::Panel;
    const float scale = juce::jmin((float)getWidth() / Panel::designWidth, (float)getHeight() / Panel::designHeight);
    const float ox = ((float)getWidth() - Panel::designWidth * scale) * 0.5f;
    const float oy = ((float)getHeight() - Panel::designHeight * scale) * 0.5f;
    panel.setBounds(0, 0, Panel::designWidth, Panel::designHeight);
    panel.setTransform(juce::AffineTransform::scale(scale).translated(ox, oy));
    processor.state.state.setProperty("uiWidth", getWidth(), nullptr);
}

void SpellEditor::timerCallback()
{
    panel.refresh();
}
