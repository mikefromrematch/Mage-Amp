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
    setSize(1120, 760);
    setResizable(true, true);
    setResizeLimits(900, 650, 1600, 1100);

    title.setText("M A G E", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(34.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, pink);
    title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title);

    subtitle.setText("HIGH GAIN AMPLIFIER  /  DIGITAL TONE ENGINE", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    subtitle.setColour(juce::Label::textColourId, muted);
    subtitle.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(subtitle);

    const char* parameterIDs[] = {
        "gain", "bass", "mid", "treble", "presence", "depth", "master",
        "gate", "drive", "delayMix", "reverbMix"
    };
    const char* parameterNames[] = {
        "GAIN", "BASS", "MIDDLE", "TREBLE", "PRESENCE", "DEPTH", "MASTER",
        "NOISE GATE", "DRIVE", "DELAY", "REVERB"
    };

    for (int i = 0; i < 11; ++i)
    {
        auto& slider = controls[i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 22);
        slider.setColour(juce::Slider::rotarySliderFillColourId, cyan);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff26384b));
        slider.setColour(juce::Slider::thumbColourId, pink);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe7edf7));
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b111c));
        slider.setColour(juce::Slider::textBoxOutlineColourId, panelEdge);
        addAndMakeVisible(slider);

        labels[i].setText(parameterNames[i], juce::dontSendNotification);
        labels[i].setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        labels[i].setColour(juce::Label::textColourId, juce::Colour(0xffdce7f4));
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);

        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state, parameterIDs[i], slider);
    }
}

void MageAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    juce::ColourGradient glow(cyan.withAlpha(0.10f), 0.0f, 0.0f,
                              pink.withAlpha(0.035f), (float) getWidth(), (float) getHeight(), false);
    g.setGradientFill(glow);
    g.fillAll();

    auto header = juce::Rectangle<float>(24.0f, 20.0f, (float) getWidth() - 48.0f, 104.0f);
    auto ampPanel = juce::Rectangle<float>(24.0f, 148.0f, (float) getWidth() - 48.0f, 284.0f);
    auto fxPanel = juce::Rectangle<float>(24.0f, 456.0f, (float) getWidth() - 48.0f, 264.0f);

    g.setColour(panel.withAlpha(0.96f));
    g.fillRoundedRectangle(header, 14.0f);
    g.fillRoundedRectangle(ampPanel, 16.0f);
    g.fillRoundedRectangle(fxPanel, 16.0f);

    g.setColour(panelEdge);
    g.drawRoundedRectangle(header, 14.0f, 1.0f);
    g.drawRoundedRectangle(ampPanel, 16.0f, 1.0f);
    g.drawRoundedRectangle(fxPanel, 16.0f, 1.0f);

    g.setColour(cyan);
    g.fillRoundedRectangle(ampPanel.getX() + 18.0f, ampPanel.getY() + 18.0f, 3.0f, 20.0f, 1.5f);
    g.fillRoundedRectangle(fxPanel.getX() + 18.0f, fxPanel.getY() + 18.0f, 3.0f, 20.0f, 1.5f);

    g.setColour(juce::Colour(0xffe8f1fc));
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText("AMPLIFIER", 54, 164, 180, 24, juce::Justification::centredLeft);
    g.drawText("TONE & OUTPUT STAGE", 54, 190, 220, 18, juce::Justification::centredLeft);

    g.setColour(pink);
    g.drawText("EFFECTS", 54, 472, 180, 24, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("INPUT SHAPING  /  TIME  /  SPACE", 54, 497, 280, 16, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff53677f));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("MAGE AUDIO  •  VERSION 0.1", 30, getHeight() - 24, getWidth() - 60, 14,
               juce::Justification::centred);
}

void MageAudioProcessorEditor::resized()
{
    const int width = getWidth();
    const int ampLeft = 42;
    const int ampWidth = width - 84;
    const int ampCell = ampWidth / 7;

    title.setBounds(48, 32, 260, 46);
    subtitle.setBounds(50, 78, 440, 22);

    for (int i = 0; i < 7; ++i)
    {
        const int x = ampLeft + i * ampCell;
        labels[i].setBounds(x, 220, ampCell, 24);
        controls[i].setBounds(x + 8, 246, ampCell - 16, 158);
    }

    const int fxLeft = 54;
    const int fxWidth = width - 108;
    const int fxCell = fxWidth / 4;
    for (int i = 0; i < 4; ++i)
    {
        const int x = fxLeft + i * fxCell;
        labels[i + 7].setBounds(x, 528, fxCell, 24);
        controls[i + 7].setBounds(x + 18, 552, fxCell - 36, 142);
    }
}
