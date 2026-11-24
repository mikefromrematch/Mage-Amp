#include "PluginEditor.h"

namespace
{
    const juce::Colour background { 0xff070a12 };
    const juce::Colour panel { 0xff111927 };
    const juce::Colour panelInner { 0xff0b121e };
    const juce::Colour edge { 0xff304158 };
    const juce::Colour cyan { 0xff58dfff };
    const juce::Colour pink { 0xffff72d6 };
    const juce::Colour text { 0xffeaf2fc };
    const juce::Colour muted { 0xff8294aa };
}

void MageKnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                   float sliderPos, float startAngle, float endAngle,
                                                   juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(9.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight() - 17.0f);
    const float radius = diameter * 0.5f;
    const auto centre = bounds.getCentre().withY(bounds.getY() + radius + 2.0f);
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    g.setColour(juce::Colour(0x44000000));
    g.fillEllipse(centre.x - radius + 1.0f, centre.y - radius + 3.0f, diameter, diameter);
    juce::ColourGradient face(juce::Colour(0xff26384d), centre.x, centre.y - radius,
                              juce::Colour(0xff090f19), centre.x, centre.y + radius, false);
    g.setGradientFill(face);
    g.fillEllipse(centre.x - radius, centre.y - radius, diameter, diameter);

    g.setColour(juce::Colour(0xff425a70));
    g.drawEllipse(centre.x - radius, centre.y - radius, diameter, diameter, 1.4f);
    g.setColour(juce::Colour(0xff080d16));
    g.drawEllipse(centre.x - radius + 5.0f, centre.y - radius + 5.0f,
                  diameter - 10.0f, diameter - 10.0f, 1.0f);

    juce::Path track;
    track.addCentredArc(centre.x, centre.y, radius - 3.0f, radius - 3.0f, 0.0f,
                        startAngle, endAngle, true);
    g.setColour(juce::Colour(0xff33465c));
    g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius - 3.0f, radius - 3.0f, 0.0f,
                           startAngle, angle, true);
    g.setColour(juce::Colour(0xff58dfff));
    g.strokePath(valueArc, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.7f, -radius + 9.0f, 3.4f, radius * 0.43f, 1.6f);
    g.setColour(juce::Colour(0xffff72d6));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    g.setColour(juce::Colour(0xff101b2a));
    g.fillEllipse(centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f);
}

MageAudioProcessorEditor::MageAudioProcessorEditor(MageAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    setSize(1180, 1060);
    setResizable(true, true);
    setResizeLimits(980, 900, 1700, 1400);

    title.setText("M A G E", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(38.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, pink);
    title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title);

    subtitle.setText("HIGH GAIN AMPLIFIER  /  MODULAR TONE SYSTEM", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
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
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 19);
        slider.setColour(juce::Slider::rotarySliderFillColourId, cyan);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff26364b));
        slider.setColour(juce::Slider::thumbColourId, pink);
        slider.setColour(juce::Slider::textBoxTextColourId, text);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, panelInner);
        slider.setColour(juce::Slider::textBoxOutlineColourId, edge);
        slider.setColour(juce::Slider::textBoxHighlightColourId, pink);
        addAndMakeVisible(slider);

        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        labels[i].setColour(juce::Label::textColourId, text);
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);

        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state, ids[i], slider);
    }

    const char* toggleIDs[] = { "gateEnabled", "gatePost", "octaveDown", "driveBoost", "delaySync" };
    const char* toggleNames[] = { "GATE ACTIVE", "POST AMP", "SUB OCTAVE", "BOOST", "TEMPO SYNC" };
    for (int i = 0; i < 5; ++i)
    {
        toggles[i].setButtonText(toggleNames[i]);
        toggles[i].setColour(juce::ToggleButton::textColourId, text);
        toggles[i].setColour(juce::ToggleButton::tickColourId, cyan);
        toggles[i].setColour(juce::ToggleButton::tickDisabledColourId, muted);
        toggles[i].setColour(juce::ToggleButton::textColourId, text);
        addAndMakeVisible(toggles[i]);
        toggleAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.state, toggleIDs[i], toggles[i]);
    }
}

void MageAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    juce::ColourGradient atmosphere(cyan.withAlpha(0.085f), 0.0f, 0.0f,
                                    pink.withAlpha(0.045f), (float)getWidth(), (float)getHeight(), false);
    g.setGradientFill(atmosphere);
    g.fillAll();

    const float w = (float)getWidth();
    auto header = juce::Rectangle<float>(22.0f, 18.0f, w - 44.0f, 104.0f);
    auto amp = juce::Rectangle<float>(22.0f, 140.0f, w - 44.0f, 248.0f);
    auto utility = juce::Rectangle<float>(22.0f, 406.0f, w - 44.0f, 188.0f);
    auto pedals = juce::Rectangle<float>(22.0f, 612.0f, w - 44.0f, (float)getHeight() - 634.0f);

    const auto drawPanel = [&g](juce::Rectangle<float> r, float radius)
    {
        g.setColour(panel.withAlpha(0.98f));
        g.fillRoundedRectangle(r, radius);
        g.setColour(edge);
        g.drawRoundedRectangle(r, radius, 1.2f);
        g.setColour(cyan.withAlpha(0.72f));
        g.fillRoundedRectangle(r.getX() + 15.0f, r.getY() + 17.0f, 3.0f, 23.0f, 1.5f);
    };
    drawPanel(header, 15.0f);
    drawPanel(amp, 17.0f);
    drawPanel(utility, 17.0f);
    drawPanel(pedals, 17.0f);

    g.setColour(juce::Colour(0xff172638));
    g.drawRoundedRectangle(header.reduced(7.0f), 11.0f, 0.7f);
    g.setColour(pink.withAlpha(0.55f));
    g.drawLine(amp.getX() + 34.0f, amp.getBottom() - 1.0f,
               amp.getRight() - 34.0f, amp.getBottom() - 1.0f, 1.0f);

    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(12.5f, juce::Font::bold)));
    g.drawText("AMPLIFIER  /  TONE STACK", 48, 151, 330, 22, juce::Justification::centredLeft);
    g.setColour(pink);
    g.drawText("GATE  &  CABINET SHAPING", 48, 417, 360, 22, juce::Justification::centredLeft);
    g.drawText("EFFECTS  /  PEDALBOARD", 48, 623, 360, 22, juce::Justification::centredLeft);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(9.5f)));
    g.drawText("MAGE AUDIO     •     HIGH VOLTAGE TONE LAB", 30, getHeight() - 20,
               getWidth() - 60, 12, juce::Justification::centred);
}

void MageAudioProcessorEditor::resized()
{
    const int width = getWidth();
    title.setBounds(48, 29, 270, 48);
    subtitle.setBounds(50, 77, 420, 20);

    const int left = 42;
    const int ampCell = (width - 84) / 7;
    for (int i = 0; i < 7; ++i)
    {
        const int x = left + i * ampCell;
        labels[i].setBounds(x, 190, ampCell, 22);
        controls[i].setBounds(x + 7, 212, ampCell - 14, 158);
    }

    // Noise gate and cabinet filters are grouped as the utility rack.
    const int utilityCell = (width - 100) / 4;
    for (int i = 0; i < 4; ++i)
    {
        const int x = 50 + i * utilityCell;
        const int controlIndex[] = { 7, 8, 21, 22 };
        labels[controlIndex[i]].setBounds(x, 450, utilityCell, 18);
        controls[controlIndex[i]].setBounds(x + 12, 468, utilityCell - 24, 104);
    }

    // Five switches form a compact control strip above the pedals.
    const int toggleWidth = (width - 112) / 5;
    for (int i = 0; i < 5; ++i)
        toggles[i].setBounds(52 + i * toggleWidth, 574, toggleWidth, 24);

    // Pedalboard: two rows of six controls, in signal-chain order.
    const int pedalCell = (width - 104) / 6;
    for (int i = 0; i < 12; ++i)
    {
        const int row = i / 6;
        const int col = i % 6;
        const int index = i + 9;
        const int x = 52 + col * pedalCell;
        const int y = row == 0 ? 666 : 824;
        labels[index].setBounds(x, y, pedalCell, 19);
        controls[index].setBounds(x + 7, y + 19, pedalCell - 14, 126);
    }
}
