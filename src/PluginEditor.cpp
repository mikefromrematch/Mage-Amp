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

    void drawPedal(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent,
                   const juce::String& name, const juce::String& descriptor)
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
    setSize(1440, 1000);
    setResizable(true, true);
    setResizeLimits(980, 850, 1900, 1400);

    title.setText("M A G E", juce::dontSendNotification);
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
        "reverbTone", "reverbMix", "output", "lowCut", "highCut"
    };
    const char* names[] = {
        "GAIN", "BASS", "MIDDLE", "TREBLE", "PRESENCE", "DEPTH", "MASTER",
        "THRESHOLD", "RELEASE", "OCTAVE MIX", "DRIVE", "TONE", "LEVEL",
        "TIME", "FEEDBACK", "MIX", "DECAY", "SIZE", "TONE", "MIX", "OUTPUT",
        "LOW CUT", "HIGH CUT"
    };

    for (int i = 0; i < 23; ++i)
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
    g.fillAll(bg);
    juce::ColourGradient aura(cyan.withAlpha(0.10f), 0.0f, 0.0f,
                              pink.withAlpha(0.07f), (float)getWidth(), (float)getHeight(), false);
    g.setGradientFill(aura);
    g.fillAll();

    const float w = (float)getWidth();
    const float h = (float)getHeight();
    // Fine etched frame and corner rune marks.
    g.setColour(juce::Colour(0xff29384d));
    g.drawRoundedRectangle(10.0f, 10.0f, w - 20.0f, h - 20.0f, 18.0f, 1.0f);
    drawRune(g, 18.0f, 18.0f, 25.0f, cyan.withAlpha(0.75f));
    drawRune(g, w - 43.0f, 18.0f, 25.0f, pink.withAlpha(0.75f));

    auto header = juce::Rectangle<float>(24.0f, 25.0f, w - 48.0f, 132.0f);
    auto amp = juce::Rectangle<float>(24.0f, 178.0f, w - 48.0f, 282.0f);
    auto utility = juce::Rectangle<float>(24.0f, 480.0f, w - 48.0f, 222.0f);
    auto pedals = juce::Rectangle<float>(24.0f, 722.0f, w - 48.0f, h - 746.0f);
    drawPlate(g, header, pink, "MAGE  /  THE RUNE SERIES", "SIGNAL PROCESSING  •  AMPLIFIER  •  EFFECTS");
    drawPlate(g, amp, cyan, "THE AMPLIFIER", "SEVEN-STAGE ANALOG-INSPIRED TONE STACK");
    drawPlate(g, utility, blue, "GATE  /  CABINET SHAPING", "INPUT CONTROL AND FREQUENCY CONTOUR");
    drawPlate(g, pedals, pink, "EFFECTS CHAIN", "FOUR DISTINCT PEDAL MODULES  /  MASTER OUTPUT");

    // The amp's ornamental centre crest and inset faceplate rails.
    const float cx = w * 0.5f;
    juce::Path crest;
    crest.startNewSubPath(cx - 28.0f, 205.0f);
    crest.lineTo(cx, 190.0f);
    crest.lineTo(cx + 28.0f, 205.0f);
    crest.lineTo(cx + 28.0f, 225.0f);
    crest.lineTo(cx, 240.0f);
    crest.lineTo(cx - 28.0f, 225.0f);
    crest.closeSubPath();
    g.setColour(juce::Colour(0xff26384e));
    g.fillPath(crest);
    g.setColour(cyan.withAlpha(0.8f));
    g.strokePath(crest, juce::PathStrokeType(1.2f));
    drawRune(g, cx - 11.0f, 197.0f, 22.0f, pink);
    g.setColour(cyan.withAlpha(0.25f));
    g.drawLine(52.0f, 448.0f, w - 52.0f, 448.0f, 1.0f);

    const float pedalTop = 722.0f;
    const float innerX = 42.0f;
    const float gap = 12.0f;
    const float cell = (w - 84.0f - gap * 4.0f) / 5.0f;
    const juce::Colour accents[] = { cyan, pink, blue, juce::Colour(0xffffb36b), pink };
    const juce::String headings[] = { "OCTAVE", "OVERDRIVE", "DELAY", "REVERB", "OUTPUT" };
    const juce::String captions[] = { "SUB-HARMONIC", "SATURATION", "ECHO MATRIX", "AMBIENCE", "MASTER LEVEL" };
    for (int i = 0; i < 5; ++i)
        drawPedal(g, { innerX + i * (cell + gap), pedalTop + 48.0f, cell, h - pedalTop - 84.0f },
                  accents[i], headings[i], captions[i]);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
    g.drawText("MAGE AUDIO  •  DESIGNED FOR THE HEAVY END OF THE SIGNAL", 30, (int)h - 20,
               (int)w - 60, 12, juce::Justification::centred);
}

void MageAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();
    title.setBounds(66, 42, 300, 55);
    subtitle.setBounds(70, 99, 550, 20);

    // Seven controls sit directly on the engraved amplifier faceplate.
    const int ampX = 48;
    const int ampWidth = w - 96;
    const int ampCell = ampWidth / 7;
    for (int i = 0; i < 7; ++i)
    {
        const int x = ampX + i * ampCell;
        labels[i].setBounds(x, 292, ampCell, 20);
        controls[i].setBounds(x + 8, 314, ampCell - 16, 126);
    }

    // Gate controls on the left; cabinet frequency controls on the right.
    const int utilityX = 52;
    const int utilityWidth = w - 104;
    const int utilityCell = utilityWidth / 4;
    const int utilityIndices[] = { 7, 8, 21, 22 };
    for (int i = 0; i < 4; ++i)
    {
        const int x = utilityX + i * utilityCell;
        labels[utilityIndices[i]].setBounds(x, 540, utilityCell, 18);
        controls[utilityIndices[i]].setBounds(x + 14, 558, utilityCell - 28, 94);
    }
    const int toggleW = (utilityWidth - 24) / 2;
    toggles[0].setBounds(utilityX + 8, 665, (utilityWidth / 4) - 12, 24);
    toggles[1].setBounds(utilityX + 8 + utilityWidth / 4, 665, (utilityWidth / 4) - 12, 24);
    loadIRButton.setBounds(utilityX + utilityWidth / 2 + 10, 663, 125, 26);
    clearIRButton.setBounds(utilityX + utilityWidth / 2 + 143, 663, 105, 26);
    irStatus.setBounds(utilityX + utilityWidth / 2 + 250, 663, utilityWidth / 2 - 258, 26);

    // Each effect gets its own metal pedal enclosure and its own control layout.
    const int gap = 12;
    const int pedalX = 42;
    const int pedalWidth = (w - 84 - gap * 4) / 5;
    const int pedalY = 824;
    const int pedalH = juce::jmax(42, h - pedalY - 58);
    const auto place = [this, pedalX, pedalWidth, gap, pedalY, pedalH](int index, int group,
                                                                      int count, int position)
    {
        const int x = pedalX + group * (pedalWidth + gap);
        const int cell = pedalWidth / count;
        const int left = x + position * cell;
        labels[index].setBounds(left + 2, pedalY, cell - 4, 18);
        controls[index].setBounds(left + 3, pedalY + 18, cell - 6, juce::jmin(104, pedalH - 42));
    };

    place(9, 0, 1, 0);
    place(10, 1, 3, 0); place(11, 1, 3, 1); place(12, 1, 3, 2);
    place(13, 2, 3, 0); place(14, 2, 3, 1); place(15, 2, 3, 2);
    place(16, 3, 4, 0); place(17, 3, 4, 1); place(18, 3, 4, 2); place(19, 3, 4, 3);
    place(20, 4, 1, 0);

    toggles[2].setBounds(pedalX + 6, h - 43, pedalWidth - 12, 22);
    toggles[3].setBounds(pedalX + pedalWidth + gap + 4, h - 43, pedalWidth - 8, 22);
    toggles[4].setBounds(pedalX + 2 * (pedalWidth + gap) + 4, h - 43, pedalWidth - 8, 22);
}
