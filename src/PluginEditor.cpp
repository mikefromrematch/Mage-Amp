#include "PluginEditor.h"

namespace
{
    const juce::Colour background { 0xff080b13 };
    const juce::Colour panel { 0xff101725 };
    const juce::Colour panelEdge { 0xff26364a };
    const juce::Colour cyan { 0xff55d8ff };
    const juce::Colour pink { 0xffff70d5 };
    const juce::Colour muted { 0xff8293a8 };
}

MageAudioProcessorEditor::MageAudioProcessorEditor(MageAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    setSize(1120, 1020);
    setResizable(true, true);
    setResizeLimits(960, 900, 1600, 1300);

    title.setText("M A G E", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(34.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, pink);
    title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title);

    subtitle.setText("HIGH GAIN AMPLIFIER  /  TONE & PEDAL ENGINE", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    subtitle.setColour(juce::Label::textColourId, muted);
    subtitle.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(subtitle);

    const char* ids[] = {
        "gain", "bass", "mid", "treble", "presence", "depth", "master",
        "gate", "gateDecay", "octaveMix", "drive", "driveTone", "driveLevel",
        "delayTime", "delayFeedback", "delayMix", "reverbDecay", "reverbSize",
        "reverbTone", "reverbMix", "output"
    };
    const char* names[] = {
        "GAIN", "BASS", "MIDDLE", "TREBLE", "PRESENCE", "DEPTH", "MASTER",
        "GATE THRESHOLD", "GATE DECAY", "OCTAVE MIX", "DRIVE GAIN", "DRIVE TONE",
        "DRIVE LEVEL", "DELAY TIME", "FEEDBACK", "DELAY MIX", "REVERB DECAY",
        "REVERB SIZE", "REVERB TONE", "REVERB MIX", "OUTPUT"
    };

    for (int i = 0; i < 21; ++i)
    {
        auto& slider = controls[i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
        slider.setColour(juce::Slider::rotarySliderFillColourId, cyan);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff26384b));
        slider.setColour(juce::Slider::thumbColourId, pink);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe7edf7));
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b111c));
        slider.setColour(juce::Slider::textBoxOutlineColourId, panelEdge);
        addAndMakeVisible(slider);

        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        labels[i].setColour(juce::Label::textColourId, juce::Colour(0xffdce7f4));
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);

        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state, ids[i], slider);
    }

    const char* toggleIDs[] = { "gateEnabled", "gatePost", "octaveDown", "driveBoost" };
    const char* toggleNames[] = { "GATE ON", "POST AMP", "OCTAVE DOWN", "DRIVE BOOST" };
    for (int i = 0; i < 4; ++i)
    {
        toggles[i].setButtonText(toggleNames[i]);
        toggles[i].setColour(juce::ToggleButton::textColourId, juce::Colour(0xffdce7f4));
        toggles[i].setColour(juce::ToggleButton::tickColourId, cyan);
        toggles[i].setColour(juce::ToggleButton::tickDisabledColourId, muted);
        addAndMakeVisible(toggles[i]);
        toggleAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.state, toggleIDs[i], toggles[i]);
    }
}

void MageAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    juce::ColourGradient glow(cyan.withAlpha(0.10f), 0.0f, 0.0f,
                              pink.withAlpha(0.045f), (float) getWidth(), (float) getHeight(), false);
    g.setGradientFill(glow);
    g.fillAll();

    auto header = juce::Rectangle<float>(24.0f, 20.0f, (float) getWidth() - 48.0f, 104.0f);
    auto ampPanel = juce::Rectangle<float>(24.0f, 148.0f, (float) getWidth() - 48.0f, 284.0f);
    auto pedalPanel = juce::Rectangle<float>(24.0f, 456.0f, (float) getWidth() - 48.0f, (float) getHeight() - 480.0f);

    g.setColour(panel.withAlpha(0.97f));
    g.fillRoundedRectangle(header, 14.0f);
    g.fillRoundedRectangle(ampPanel, 16.0f);
    g.fillRoundedRectangle(pedalPanel, 16.0f);

    g.setColour(panelEdge);
    g.drawRoundedRectangle(header, 14.0f, 1.0f);
    g.drawRoundedRectangle(ampPanel, 16.0f, 1.0f);
    g.drawRoundedRectangle(pedalPanel, 16.0f, 1.0f);

    g.setColour(cyan);
    g.fillRoundedRectangle(ampPanel.getX() + 18.0f, ampPanel.getY() + 18.0f, 3.0f, 20.0f, 1.5f);
    g.fillRoundedRectangle(pedalPanel.getX() + 18.0f, pedalPanel.getY() + 18.0f, 3.0f, 20.0f, 1.5f);

    g.setColour(juce::Colour(0xffe8f1fc));
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText("AMPLIFIER  /  ACTIVE TONE STACK", 54, 164, 360, 24, juce::Justification::centredLeft);
    g.setColour(pink);
    g.drawText("PEDALBOARD  /  GATE • OCTAVE • DRIVE • DELAY • REVERB", 54, 472, 600, 24,
               juce::Justification::centredLeft);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("MAGE AUDIO  •  DSP PROTOTYPE 0.2", 30, getHeight() - 22, getWidth() - 60, 14,
               juce::Justification::centred);
}

void MageAudioProcessorEditor::resized()
{
    const int width = getWidth();
    title.setBounds(48, 32, 260, 46);
    subtitle.setBounds(50, 78, 500, 22);

    const int ampLeft = 42;
    const int ampCell = (width - 84) / 7;
    for (int i = 0; i < 7; ++i)
    {
        const int x = ampLeft + i * ampCell;
        labels[i].setBounds(x, 220, ampCell, 24);
        controls[i].setBounds(x + 7, 246, ampCell - 14, 158);
    }

    // Two rows of pedal controls. Group ordering follows the signal chain.
    const int left = 42;
    const int cell = (width - 84) / 7;
    const int rowY[] = { 558, 748 };
    for (int i = 0; i < 14; ++i)
    {
        const int row = i / 7;
        const int col = i % 7;
        const int x = left + col * cell;
        const int index = i + 7;
        labels[index].setBounds(x, rowY[row], cell, 22);
        controls[index].setBounds(x + 8, rowY[row] + 24, cell - 16, 132);
    }

    const int toggleY = getHeight() - 78;
    const int toggleWidth = (width - 100) / 4;
    for (int i = 0; i < 4; ++i)
        toggles[i].setBounds(50 + i * toggleWidth, toggleY, toggleWidth, 28);
}
