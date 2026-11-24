#include "PluginEditor.h"

namespace
{
    const juce::Colour bg { 0xff070911 };
    const juce::Colour metal { 0xff151d2b };
    const juce::Colour metalLight { 0xff26364a };
    const juce::Colour inset { 0xff090f19 };
    const juce::Colour cyan { 0xff59ddff };
    const juce::Colour blue { 0xff458bff };
    const juce::Colour pink { 0xffff70d5 };
    const juce::Colour ivory { 0xffedf4ff };
    const juce::Colour muted { 0xff91a3ba };

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
        g.strokePath(rune, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    void drawPlate(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent,
                   const juce::String& heading, const juce::String& subheading = {})
    {
        juce::ColourGradient gradient(metalLight, r.getX(), r.getY(),
                                      metal, r.getRight(), r.getBottom(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(r, 13.0f);
        g.setColour(juce::Colour(0xff40536a));
        g.drawRoundedRectangle(r, 13.0f, 1.2f);
        g.setColour(inset.withAlpha(0.92f));
        g.fillRoundedRectangle(r.reduced(5.0f), 10.0f);
        g.setColour(accent.withAlpha(0.85f));
        g.fillRoundedRectangle(r.getX() + 13.0f, r.getY() + 13.0f, 3.0f, 25.0f, 1.5f);
        g.setColour(accent.withAlpha(0.16f));
        g.drawRoundedRectangle(r.reduced(2.0f), 11.0f, 1.0f);
        g.setColour(accent.withAlpha(0.65f));
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(heading, r.getX() + 25.0f, r.getY() + 11.0f, r.getWidth() - 42.0f, 20.0f,
                   juce::Justification::centredLeft);
        if (subheading.isNotEmpty())
        {
            g.setColour(muted);
            g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
            g.drawText(subheading, r.getX() + 25.0f, r.getY() + 31.0f, r.getWidth() - 42.0f, 13.0f,
                       juce::Justification::centredLeft);
        }
        const float d = 5.0f;
        for (auto p : { r.getTopLeft(), r.getTopRight(), r.getBottomLeft(), r.getBottomRight() })
        {
            g.setColour(juce::Colour(0xff71849b));
            g.fillEllipse(p.x - d * 0.5f, p.y - d * 0.5f, d, d);
            g.setColour(juce::Colour(0xff0a101a));
            g.fillEllipse(p.x - 1.0f, p.y - 1.0f, 2.0f, 2.0f);
        }
    }

    void drawChassis(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent,
                     const juce::String& heading, const juce::String& detail)
    {
        juce::Path shell;
        shell.startNewSubPath(r.getX() + 18.0f, r.getY());
        shell.lineTo(r.getRight() - 18.0f, r.getY());
        shell.lineTo(r.getRight(), r.getY() + 18.0f);
        shell.lineTo(r.getRight(), r.getBottom() - 18.0f);
        shell.lineTo(r.getRight() - 18.0f, r.getBottom());
        shell.lineTo(r.getX() + 18.0f, r.getBottom());
        shell.lineTo(r.getX(), r.getBottom() - 18.0f);
        shell.lineTo(r.getX(), r.getY() + 18.0f);
        shell.closeSubPath();
        g.setColour(juce::Colour(0xaa000000));
        g.fillPath(shell, juce::AffineTransform::translation(0.0f, 6.0f));
        juce::ColourGradient finish(juce::Colour(0xff536278), r.getX(), r.getY(),
                                    juce::Colour(0xff101722), r.getRight(), r.getBottom(), false);
        g.setGradientFill(finish);
        g.fillPath(shell);
        g.setColour(juce::Colour(0xff8495a9));
        g.strokePath(shell, juce::PathStrokeType(1.5f));
        auto face = r.reduced(7.0f);
        g.setColour(juce::Colour(0xff080e18));
        g.fillRoundedRectangle(face, 11.0f);
        g.setColour(accent.withAlpha(0.48f));
        g.drawRoundedRectangle(face, 11.0f, 1.2f);
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(heading, face.getX() + 20.0f, face.getY() + 9.0f,
                   face.getWidth() - 40.0f, 18.0f, juce::Justification::centredLeft);
        g.setColour(muted);
        g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
        g.drawText(detail, face.getX() + 20.0f, face.getY() + 28.0f,
                   face.getWidth() - 40.0f, 13.0f, juce::Justification::centredLeft);
        for (int i = 0; i < 4; ++i)
        {
            const float bx = (i % 2 == 0) ? r.getX() + 9.0f : r.getRight() - 13.0f;
            const float by = (i < 2) ? r.getY() + 9.0f : r.getBottom() - 13.0f;
            g.setColour(juce::Colour(0xff9aa9b9));
            g.fillEllipse(bx, by, 4.0f, 4.0f);
        }
    }

    void drawPedal(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent,
                   const juce::String& name, const juce::String& descriptor, bool enabled)
    {
        juce::Path body;
        body.startNewSubPath(r.getX() + 10.0f, r.getY());
        body.lineTo(r.getRight() - 10.0f, r.getY());
        body.lineTo(r.getRight(), r.getY() + 12.0f);
        body.lineTo(r.getRight() - 3.0f, r.getBottom() - 13.0f);
        body.lineTo(r.getRight() - 15.0f, r.getBottom());
        body.lineTo(r.getX() + 15.0f, r.getBottom());
        body.lineTo(r.getX() + 3.0f, r.getBottom() - 13.0f);
        body.lineTo(r.getX(), r.getY() + 12.0f);
        body.closeSubPath();
        g.setColour(juce::Colour(0x99000000));
        g.fillPath(body, juce::AffineTransform::translation(2.0f, 5.0f));
        juce::ColourGradient shell(juce::Colour(0xff46566b), r.getX(), r.getY(),
                                   juce::Colour(0xff101722), r.getRight(), r.getBottom(), false);
        g.setGradientFill(shell);
        g.fillPath(body);
        g.setColour(juce::Colour(0xff71849b));
        g.strokePath(body, juce::PathStrokeType(1.3f));
        auto insetBody = r.reduced(6.0f, 7.0f);
        g.setColour(juce::Colour(0xff0a101a));
        g.fillRoundedRectangle(insetBody, 8.0f);
        g.setColour(accent.withAlpha(0.85f));
        g.drawRoundedRectangle(insetBody, 8.0f, 1.2f);
        const float ledX = insetBody.getRight() - 13.0f;
        const float ledY = insetBody.getY() + 13.0f;
        if (enabled)
        {
            g.setColour(accent.withAlpha(0.22f));
            g.fillEllipse(ledX - 7.0f, ledY - 7.0f, 14.0f, 14.0f);
        }
        g.setColour(enabled ? accent : juce::Colour(0xff354253));
        g.fillEllipse(ledX - 3.0f, ledY - 3.0f, 6.0f, 6.0f);
        g.setColour(enabled ? accent.withAlpha(0.95f) : muted.withAlpha(0.45f));
        g.drawEllipse(ledX - 4.5f, ledY - 4.5f, 9.0f, 9.0f, 1.0f);
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(name, insetBody.getX() + 5.0f, insetBody.getY() + 7.0f,
                   insetBody.getWidth() - 10.0f, 16.0f, juce::Justification::centred);
        g.setColour(muted);
        g.setFont(juce::Font(juce::FontOptions(7.5f, juce::Font::bold)));
        g.drawText(descriptor, insetBody.getX() + 4.0f, insetBody.getY() + 24.0f,
                   insetBody.getWidth() - 8.0f, 12.0f, juce::Justification::centred);
        const float foot = juce::jmin(25.0f, insetBody.getWidth() * 0.22f);
        const float fx = insetBody.getCentreX() - foot * 0.5f;
        const float fy = insetBody.getBottom() - foot - 8.0f;
        g.setColour(juce::Colour(0xff02050a));
        g.fillEllipse(fx - 3.0f, fy - 3.0f, foot + 6.0f, foot + 6.0f);
        g.setColour(juce::Colour(0xff64758a));
        g.fillEllipse(fx, fy, foot, foot);
        g.setColour(accent.withAlpha(0.7f));
        g.drawEllipse(fx, fy, foot, foot, 1.3f);
        g.setColour(juce::Colour(0xff172334));
        g.fillEllipse(fx + foot * 0.28f, fy + foot * 0.28f, foot * 0.44f, foot * 0.44f);
    }
}

void MageKnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float startAngle, float endAngle,
                                            juce::Slider&)
{
    const auto area = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(5.0f);
    const float diameter = juce::jmin(area.getWidth(), area.getHeight() - 18.0f);
    const float radius = diameter * 0.5f;
    const auto c = area.getCentre().withY(area.getY() + radius + 2.0f);
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    g.setColour(juce::Colour(0x99000000));
    g.fillEllipse(c.x - radius + 2.0f, c.y - radius + 4.0f, diameter, diameter);
    juce::ColourGradient face(juce::Colour(0xff53647a), c.x - radius, c.y - radius,
                              juce::Colour(0xff111a28), c.x + radius, c.y + radius, false);
    g.setGradientFill(face);
    g.fillEllipse(c.x - radius, c.y - radius, diameter, diameter);
    g.setColour(juce::Colour(0xff8294aa));
    g.drawEllipse(c.x - radius, c.y - radius, diameter, diameter, 1.5f);
    g.setColour(juce::Colour(0xff080d16));
    g.drawEllipse(c.x - radius + 5.0f, c.y - radius + 5.0f, diameter - 10.0f, diameter - 10.0f, 1.0f);

    juce::Path track;
    track.addCentredArc(c.x, c.y, radius - 3.0f, radius - 3.0f, 0.0f, startAngle, endAngle, true);
    g.setColour(juce::Colour(0xff35475e));
    g.strokePath(track, juce::PathStrokeType(3.4f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    juce::Path arc;
    arc.addCentredArc(c.x, c.y, radius - 3.0f, radius - 3.0f, 0.0f, startAngle, angle, true);
    juce::ColourGradient glow(cyan, c.x - radius, c.y, pink, c.x + radius, c.y, false);
    g.setGradientFill(glow);
    g.strokePath(arc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
    juce::Path pointer;
    pointer.addRoundedRectangle(-1.8f, -radius + 9.0f, 3.6f, radius * 0.42f, 1.7f);
    g.setColour(juce::Colour(0xffff8ce0));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
    g.setColour(juce::Colour(0xff101827));
    g.fillEllipse(c.x - 5.0f, c.y - 5.0f, 10.0f, 10.0f);
    g.setColour(cyan.withAlpha(0.55f));
    g.drawEllipse(c.x - 2.0f, c.y - 2.0f, 4.0f, 4.0f, 1.0f);
}

MageAudioProcessorEditor::MageAudioProcessorEditor(MageAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    setSize(1672, 941);
    setResizable(true, true);
    setResizeLimits(1200, 675, 2400, 1500);

    title.setText("", juce::dontSendNotification);
    title.setVisible(false);
    title.setFont(juce::Font(juce::FontOptions(42.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, pink);
    title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title);

    subtitle.setText("RUNE-FORGED AMPLIFICATION     /     HIGH VOLTAGE TONE SYSTEM", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    subtitle.setColour(juce::Label::textColourId, muted);
    subtitle.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(subtitle);

    const char* ids[] = {
        "gain", "bass", "mid", "treble", "presence", "depth", "master",
        "gate", "gateDecay", "octaveMix", "drive", "driveTone", "driveLevel",
        "delayTime", "delayFeedback", "delayMix", "reverbDecay", "reverbSize",
        "reverbTone", "reverbMix", "output", "lowCut", "highCut", "octaveTone", "octaveLevel"
    };
    const char* names[] = {
        "GAIN", "BASS", "MIDDLE", "TREBLE", "PRESENCE", "DEPTH", "MASTER",
        "THRESHOLD", "RELEASE", "OCTAVE MIX", "DRIVE", "TONE", "LEVEL",
        "TIME", "FEEDBACK", "MIX", "DECAY", "SIZE", "TONE", "MIX", "OUTPUT",
        "LOW CUT", "HIGH CUT", "TONE", "LEVEL"
    };

    for (int i = 0; i < 25; ++i)
    {
        auto& slider = controls[i];
        slider.setLookAndFeel(&knobLookAndFeel);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 66, 18);
        slider.setColour(juce::Slider::textBoxTextColourId, ivory);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff101a29));
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff34465c));
        slider.setColour(juce::Slider::textBoxHighlightColourId, pink);
        addAndMakeVisible(slider);

        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
        labels[i].setColour(juce::Label::textColourId, ivory);
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);

        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state, ids[i], slider);
    }

    loadIRButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff17344a));
    loadIRButton.setColour(juce::TextButton::textColourOffId, cyan);
    loadIRButton.setColour(juce::TextButton::textColourOnId, ivory);
    loadIRButton.onClick = [this] { chooseCabinetIR(); };
    addAndMakeVisible(loadIRButton);
    clearIRButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff302039));
    clearIRButton.setColour(juce::TextButton::textColourOffId, pink);
    clearIRButton.onClick = [this]
    {
        processor.bypassCabinetIR();
        irStatus.setText("CABINET IR BYPASSED", juce::dontSendNotification);
    };
    addAndMakeVisible(clearIRButton);
    irStatus.setText(processor.hasCabinetIR() ? processor.getCabinetIRName() : "NO CUSTOM IR LOADED",
                     juce::dontSendNotification);
    irStatus.setColour(juce::Label::textColourId, muted);
    irStatus.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    irStatus.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(irStatus);

    const char* pedalIDs[] = { "octaveEnabled", "driveEnabled", "delayEnabled", "reverbEnabled" };
    const char* pedalNames[] = { "OCTAVE", "DRIVE", "DELAY", "REVERB" };
    for (int i = 0; i < 4; ++i)
    {
        pedalSwitches[i].setClickingTogglesState(true);
        pedalSwitches[i].setButtonText("");
        pedalSwitches[i].setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        pedalSwitches[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        pedalSwitches[i].setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
        pedalSwitches[i].setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
        pedalSwitches[i].onClick = [this, i, pedalNames]
        {
            repaint();
        };
        addAndMakeVisible(pedalSwitches[i]);
        pedalSwitchAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.state, pedalIDs[i], pedalSwitches[i]);

    }

    const char* toggleIDs[] = { "gateEnabled", "gatePost", "octaveDown", "driveBoost", "delaySync" };
    const char* toggleNames[] = { "GATE ON", "POST AMP", "SUB OCTAVE", "BOOST", "TEMPO SYNC" };
    for (int i = 0; i < 5; ++i)
    {
        toggles[i].setButtonText(toggleNames[i]);
        toggles[i].setColour(juce::ToggleButton::textColourId, ivory);
        toggles[i].setColour(juce::ToggleButton::tickColourId, cyan);
        toggles[i].setColour(juce::ToggleButton::tickDisabledColourId, muted);
        addAndMakeVisible(toggles[i]);
        toggleAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.state, toggleIDs[i], toggles[i]);
    }
}

void MageAudioProcessorEditor::chooseCabinetIR()
{
    irFileChooser = std::make_unique<juce::FileChooser>("Choose a cabinet impulse response",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.wav;*.aif;*.aiff");
    auto safeThis = juce::Component::SafePointer<MageAudioProcessorEditor>(this);
    irFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;
            const auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                const bool loaded = safeThis->processor.loadCabinetIR(file);
                safeThis->irStatus.setText(loaded ? file.getFileName() : "UNSUPPORTED IR FILE",
                                           juce::dontSendNotification);
            }
        });
}

void MageAudioProcessorEditor::paint(juce::Graphics& g)
{
    const float sx = (float)getWidth() / 1672.0f;
    const float sy = (float)getHeight() / 941.0f;
    g.fillAll(bg);
    g.addTransform(juce::AffineTransform::scale(sx, sy));

    juce::ColourGradient aura(cyan.withAlpha(0.12f), 0.0f, 0.0f,
                              pink.withAlpha(0.10f), 1672.0f, 941.0f, false);
    g.setGradientFill(aura);
    g.fillRect(0.0f, 0.0f, 1672.0f, 941.0f);

    // Etched, low-contrast rune field behind the hardware.
    g.setColour(juce::Colour(0xff14213a).withAlpha(0.48f));
    for (int i = 0; i < 18; ++i)
    {
        const float x = 35.0f + i * 94.0f;
        juce::Path etching;
        etching.startNewSubPath(x, 105.0f);
        etching.lineTo(x + 32.0f, 72.0f);
        etching.lineTo(x + 64.0f, 105.0f);
        etching.lineTo(x + 32.0f, 138.0f);
        etching.closeSubPath();
        g.strokePath(etching, juce::PathStrokeType(0.8f));
    }
    g.setColour(juce::Colour(0xff24304a));
    g.drawLine(0.0f, 91.0f, 1672.0f, 91.0f, 1.0f);

    // Header: sigil, preset strip, transport icons, and glowing meters.
    juce::ColourGradient logoGlow(cyan.withAlpha(0.24f), 28.0f, 8.0f,
                                  blue.withAlpha(0.02f), 135.0f, 88.0f, true);
    g.setGradientFill(logoGlow);
    g.fillEllipse(28.0f, 5.0f, 90.0f, 82.0f);
    drawRune(g, 51.0f, 18.0f, 44.0f, cyan);
    g.setColour(blue.withAlpha(0.7f));
    g.drawEllipse(38.0f, 12.0f, 72.0f, 72.0f, 1.0f);
    g.setColour(juce::Colour(0xff263653));
    g.drawLine(171.0f, 20.0f, 171.0f, 75.0f, 1.0f);
    g.setColour(juce::Colour(0xff0c1425));
    g.fillRoundedRectangle(205.0f, 23.0f, 326.0f, 52.0f, 9.0f);
    g.setColour(blue.withAlpha(0.75f));
    g.drawRoundedRectangle(205.0f, 23.0f, 326.0f, 52.0f, 9.0f, 1.0f);
    g.setColour(ivory);
    g.setFont(juce::Font(juce::FontOptions(19.0f)));
    g.drawText("Granophyre Inspired", 225.0f, 34.0f, 260.0f, 30.0f, juce::Justification::centredLeft);
    juce::Path arrow;
    arrow.addTriangle(495.0f, 44.0f, 512.0f, 44.0f, 503.5f, 55.0f);
    g.setColour(muted); g.fillPath(arrow);
    g.setColour(cyan); g.setFont(juce::Font(juce::FontOptions(34.0f)));
    g.drawText("▣", 572.0f, 23.0f, 54.0f, 52.0f, juce::Justification::centred);
    g.setColour(muted); g.setFont(juce::Font(juce::FontOptions(40.0f)));
    g.drawText("↶", 674.0f, 17.0f, 54.0f, 55.0f, juce::Justification::centred);
    g.drawText("↷", 755.0f, 17.0f, 54.0f, 55.0f, juce::Justification::centred);

    // MAGE crest and cyan-to-magenta horizon.
    g.setColour(cyan.withAlpha(0.85f)); g.drawLine(0.0f, 134.0f, 645.0f, 134.0f, 1.0f);
    g.setColour(pink.withAlpha(0.85f)); g.drawLine(1025.0f, 134.0f, 1672.0f, 134.0f, 1.0f);
    g.setColour(cyan); g.fillEllipse(641.0f, 130.0f, 7.0f, 7.0f);
    g.setColour(pink); g.fillEllipse(1021.0f, 130.0f, 7.0f, 7.0f);
    g.setColour(ivory); g.setFont(juce::Font(juce::FontOptions(58.0f, juce::Font::plain)));
    g.drawText("M A G E", 650.0f, 88.0f, 372.0f, 78.0f, juce::Justification::centred);
    g.setColour(cyan.withAlpha(0.18f)); g.drawEllipse(772.0f, 70.0f, 130.0f, 130.0f, 1.0f);

    auto panel = [&g](juce::Rectangle<float> r, juce::Colour accent, float radius)
    {
        g.setColour(juce::Colour(0xdd00030a));
        g.fillRoundedRectangle(r.translated(0.0f, 7.0f), radius);
        juce::ColourGradient edge(accent.withAlpha(0.9f), r.getX(), r.getY(),
                                  juce::Colour(0xff15152a), r.getRight(), r.getBottom(), false);
        g.setGradientFill(edge); g.fillRoundedRectangle(r, radius);
        g.setColour(accent.withAlpha(0.95f)); g.drawRoundedRectangle(r, radius, 2.0f);
        g.setColour(juce::Colour(0xff0a0d1b)); g.fillRoundedRectangle(r.reduced(5.0f), radius - 4.0f);
        g.setColour(accent.withAlpha(0.20f)); g.drawRoundedRectangle(r.reduced(8.0f), radius - 6.0f, 1.0f);
    };

    // Left noise gate.
    panel({ 20.0f, 155.0f, 166.0f, 375.0f }, cyan, 13.0f);
    g.setColour(ivory); g.setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::bold)));
    g.drawText("NOISE", 35.0f, 171.0f, 136.0f, 24.0f, juce::Justification::centred);
    g.drawText("GATE", 35.0f, 193.0f, 136.0f, 24.0f, juce::Justification::centred);
    g.setColour(muted); g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText("THRESHOLD", 35.0f, 310.0f, 136.0f, 20.0f, juce::Justification::centred);
    g.drawText("DECAY", 35.0f, 405.0f, 136.0f, 20.0f, juce::Justification::centred);

    // Main amp chassis: broad, engraved and bevel-edged.
    panel({ 225.0f, 176.0f, 1193.0f, 306.0f }, cyan, 23.0f);
    g.setColour(cyan.withAlpha(0.25f));
    g.drawLine(244.0f, 241.0f, 1400.0f, 241.0f, 1.0f);
    g.drawLine(244.0f, 438.0f, 1400.0f, 438.0f, 1.0f);
    for (int i = 0; i < 18; ++i)
    {
        const float x = 246.0f + i * 64.0f;
        drawRune(g, x, 187.0f, 30.0f, (i < 9 ? cyan : pink).withAlpha(0.22f));
    }
    for (auto p : { juce::Point<float>(258,218), juce::Point<float>(1386,218),
                    juce::Point<float>(258,446), juce::Point<float>(1386,446) })
    {
        g.setColour(juce::Colour(0xff02050b)); g.fillEllipse(p.x - 13, p.y - 13, 26, 26);
        g.setColour(juce::Colour(0xff73839a)); g.drawEllipse(p.x - 9, p.y - 9, 18, 18, 2.0f);
        g.setColour(juce::Colour(0xff0a1220)); g.fillEllipse(p.x - 5, p.y - 5, 10, 10);
    }

    // Right cabinet / IR bay.
    panel({ 1452.0f, 155.0f, 177.0f, 375.0f }, pink, 13.0f);
    g.setColour(ivory); g.setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::bold)));
    g.drawText("CABINET / IR", 1460.0f, 171.0f, 160.0f, 26.0f, juce::Justification::centred);
    g.setColour(pink.withAlpha(0.24f)); g.drawRoundedRectangle(1480.0f, 210.0f, 122.0f, 116.0f, 5.0f, 2.0f);
    drawRune(g, 1521.0f, 235.0f, 40.0f, pink.withAlpha(0.65f));

    // Pedalboard: four distinct metal stompboxes linked by glowing patch cables.
    const juce::Rectangle<float> pedals[] = {
        { 78.0f, 555.0f, 280.0f, 329.0f },
        { 412.0f, 555.0f, 258.0f, 329.0f },
        { 725.0f, 555.0f, 298.0f, 329.0f },
        { 1078.0f, 555.0f, 291.0f, 329.0f }
    };
    const juce::Colour pedalColours[] = { juce::Colour(0xffa45dff), pink, blue, pink };
    const juce::String pedalTitles[] = { "OCTAVE", "DRIVE", "DELAY", "REVERB" };
    for (int i = 0; i < 4; ++i)
    {
        panel(pedals[i], pedalColours[i], 17.0f);
        g.setColour(pedalColours[i]); g.setFont(juce::Font(juce::FontOptions(26.0f, juce::Font::bold)));
        g.drawText(pedalTitles[i], pedals[i].getX() + 8.0f, 570.0f, pedals[i].getWidth() - 16.0f, 36.0f,
                   juce::Justification::centred);
        g.setColour(pedalColours[i].withAlpha(0.2f));
        drawRune(g, pedals[i].getX() + 18.0f, 804.0f, 42.0f, pedalColours[i].withAlpha(0.34f));
        drawRune(g, pedals[i].getRight() - 60.0f, 804.0f, 42.0f, pedalColours[i].withAlpha(0.34f));
        // Hardware corner screws.
        for (auto p : { pedals[i].getTopLeft().translated(15.0f, 15.0f),
                        pedals[i].getTopRight().translated(-15.0f, 15.0f),
                        pedals[i].getBottomLeft().translated(15.0f, -15.0f),
                        pedals[i].getBottomRight().translated(-15.0f, -15.0f) })
        {
            g.setColour(juce::Colour(0xff050711)); g.fillEllipse(p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f);
            g.setColour(pedalColours[i].withAlpha(0.55f)); g.drawEllipse(p.x - 3.0f, p.y - 3.0f, 6.0f, 6.0f, 1.0f);
        }
    }
    // Cable links between the pedals.
    g.setColour(cyan.withAlpha(0.85f));
    for (float x : { 360.0f, 672.0f, 1025.0f })
    {
        g.fillRoundedRectangle(x, 697.0f, 42.0f, 13.0f, 5.0f);
        g.setColour(juce::Colour(0xff9ba9c7)); g.drawRoundedRectangle(x, 697.0f, 42.0f, 13.0f, 5.0f, 1.0f);
        g.setColour(cyan.withAlpha(0.85f));
    }

    // Jewel status lamps and chrome footswitches are painted separately from the hit targets.
    const char* enabledIDs[] = { "octaveEnabled", "driveEnabled", "delayEnabled", "reverbEnabled" };
    const float ledCentres[] = { 219.0f, 541.0f, 874.0f, 1223.0f };
    const juce::Colour ledColours[] = { juce::Colour(0xffbd69ff), pink, cyan, pink };
    for (int i = 0; i < 4; ++i)
    {
        const bool enabled = processor.state.getRawParameterValue(enabledIDs[i])->load() > 0.5f;
        const float lx = ledCentres[i], ly = 781.0f;
        if (enabled)
        {
            g.setColour(ledColours[i].withAlpha(0.28f)); g.fillEllipse(lx - 13.0f, ly - 13.0f, 26.0f, 26.0f);
        }
        g.setColour(enabled ? ledColours[i] : juce::Colour(0xff394052));
        g.fillEllipse(lx - 6.0f, ly - 6.0f, 12.0f, 12.0f);
        g.setColour(enabled ? ledColours[i] : juce::Colour(0xff69758a));
        g.drawEllipse(lx - 8.0f, ly - 8.0f, 16.0f, 16.0f, 1.5f);
        g.setColour(juce::Colour(0xff02050b)); g.fillEllipse(lx - 20.0f, 802.0f, 40.0f, 40.0f);
        g.setColour(juce::Colour(0xff9ba9c4)); g.drawEllipse(lx - 18.0f, 804.0f, 36.0f, 36.0f, 2.0f);
        g.setColour(juce::Colour(0xff202b3e)); g.fillEllipse(lx - 13.0f, 809.0f, 26.0f, 26.0f);
        g.setColour(ledColours[i].withAlpha(0.65f)); g.drawEllipse(lx - 13.0f, 809.0f, 26.0f, 26.0f, 1.0f);
    }

    // Dedicated output module.
    panel({ 1426.0f, 584.0f, 202.0f, 283.0f }, pink, 15.0f);
    g.setColour(pink.withAlpha(0.75f));
    for (int i = 0; i < 11; ++i)
        g.fillRoundedRectangle(1588.0f, 606.0f + i * 21.0f, 17.0f, 14.0f, 2.0f);
    g.setColour(ivory); g.setFont(juce::Font(juce::FontOptions(19.0f, juce::Font::bold)));
    g.drawText("OUTPUT", 1450.0f, 765.0f, 150.0f, 30.0f, juce::Justification::centred);

    // Input/output meter rails and top-right settings glyph.
    g.setColour(cyan); g.setFont(juce::Font(juce::FontOptions(19.0f, juce::Font::bold)));
    g.drawText("IN", 1065.0f, 31.0f, 36.0f, 32.0f, juce::Justification::centred);
    g.setColour(pink); g.drawText("OUT", 1344.0f, 31.0f, 52.0f, 32.0f, juce::Justification::centred);
    for (int row = 0; row < 3; ++row)
    {
        g.setColour(cyan.withAlpha(0.18f)); g.fillRect(1115.0f, 36.0f + row * 9.0f, 167.0f, 5.0f);
        g.setColour(cyan); g.fillRect(1115.0f, 36.0f + row * 9.0f, 72.0f + row * 8.0f, 5.0f);
        g.setColour(pink.withAlpha(0.18f)); g.fillRect(1408.0f, 36.0f + row * 9.0f, 147.0f, 5.0f);
        g.setColour(pink); g.fillRect(1408.0f, 36.0f + row * 9.0f, 80.0f + row * 8.0f, 5.0f);
    }
    g.setColour(pink); g.setFont(juce::Font(juce::FontOptions(34.0f)));
    g.drawText("⚙", 1574.0f, 19.0f, 56.0f, 54.0f, juce::Justification::centred);
}

void MageAudioProcessorEditor::resized()
{
    const float sx = (float)getWidth() / 1672.0f;
    const float sy = (float)getHeight() / 941.0f;
    auto bounds = [sx, sy](float x, float y, float w, float h)
    {
        return juce::Rectangle<int>((int)std::round(x * sx), (int)std::round(y * sy),
                                    (int)std::round(w * sx), (int)std::round(h * sy));
    };

    // Header elements are painted into the engraved top strip.
    title.setBounds(bounds(650, 88, 372, 78));
    subtitle.setBounds(0, 0, 0, 0);

    // Amp face: seven large controls aligned across the central chassis.
    const int ampX[] = { 288, 445, 602, 759, 916, 1073, 1230 };
    for (int i = 0; i < 7; ++i)
    {
        labels[i].setBounds(bounds((float)ampX[i], 254, 130, 25));
        controls[i].setBounds(bounds((float)ampX[i], 280, 130, 132));
        controls[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    }

    // Gate panel on the left.
    labels[7].setBounds(bounds(40, 307, 126, 20));
    controls[7].setBounds(bounds(56, 220, 96, 88));
    labels[8].setBounds(bounds(40, 405, 126, 20));
    controls[8].setBounds(bounds(60, 330, 88, 78));
    toggles[0].setBounds(bounds(39, 439, 132, 34));
    toggles[1].setBounds(bounds(39, 481, 132, 34));

    // Cabinet bay on the right: IR loader and high/low cut.
    loadIRButton.setBounds(bounds(1470, 386, 145, 37));
    clearIRButton.setBounds(bounds(1470, 427, 145, 28));
    irStatus.setBounds(bounds(1464, 457, 158, 22));
    labels[21].setBounds(bounds(1468, 447, 78, 18));
    controls[21].setBounds(bounds(1537, 435, 48, 47));
    labels[22].setBounds(bounds(1467, 493, 78, 18));
    controls[22].setBounds(bounds(1537, 480, 48, 47));

    // Pedal controls: match the reference's individual hardware enclosures.
    const int octaveX[] = { 96, 178, 260 };
    labels[9].setBounds(bounds(96, 735, 82, 20));
    controls[9].setBounds(bounds(96, 647, 82, 86));
    labels[23].setBounds(bounds(181, 735, 82, 20));
    controls[23].setBounds(bounds(181, 647, 82, 86));
    labels[24].setBounds(bounds(266, 735, 82, 20));
    controls[24].setBounds(bounds(266, 647, 82, 86));
    toggles[2].setBounds(bounds(183, 616, 110, 27));

    const int driveX[] = { 426, 502, 578 };
    for (int i = 0; i < 3; ++i)
    {
        labels[10 + i].setBounds(bounds((float)driveX[i], 692, 74, 18));
        controls[10 + i].setBounds(bounds((float)driveX[i], 620, 74, 78));
    }
    toggles[3].setBounds(bounds(468, 727, 145, 27));

    const int delayX[] = { 742, 837, 932 };
    for (int i = 0; i < 3; ++i)
    {
        labels[13 + i].setBounds(bounds((float)delayX[i], 692, 86, 18));
        controls[13 + i].setBounds(bounds((float)delayX[i], 620, 86, 78));
    }
    toggles[4].setBounds(bounds(916, 747, 92, 27));

    const int reverbX[] = { 1092, 1182, 1272,  };
    for (int i = 0; i < 4; ++i)
    {
        const int x = i < 3 ? reverbX[i] : 1182;
        const int y = i < 3 ? 620 : 704;
        labels[16 + i].setBounds(bounds((float)x, i < 3 ? 692.0f : 779.0f, 86, 18));
        controls[16 + i].setBounds(bounds((float)x, (float)y, 86, 78));
    }

    // Four real stomp switches, visually aligned beneath each pedal's controls.
    const int switchX[] = { 190, 512, 845, 1194 };
    for (int i = 0; i < 4; ++i)
    {
        pedalSwitches[i].setBounds(bounds((float)switchX[i], 824, 58, 35));
        pedalSwitches[i].setButtonText(pedalSwitches[i].getToggleState() ? "ON" : "OFF");
    }

    labels[20].setBounds(bounds(1445, 760, 150, 25));
    controls[20].setBounds(bounds(1452, 635, 128, 128));
    controls[20].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
}
