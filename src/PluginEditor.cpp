#include "PluginEditor.h"

using namespace mage::ui;
using namespace mage::ui::colours;

namespace layout
{
    using R = juce::Rectangle<float>;
    const R gate   { 20.0f, 148.0f, 170.0f, 322.0f };
    const R amp    { 210.0f, 148.0f, 1180.0f, 322.0f };
    const R cab    { 1410.0f, 148.0f, 170.0f, 322.0f };
    const R pedals[] = { { 20.0f, 492.0f, 300.0f, 388.0f }, { 340.0f, 492.0f, 300.0f, 388.0f },
                         { 660.0f, 492.0f, 300.0f, 388.0f }, { 980.0f, 492.0f, 300.0f, 388.0f } };
    const R output { 1300.0f, 492.0f, 280.0f, 388.0f };
    const juce::Colour pedalAccent[] = { violet, pink, cyan, blue };
    const char* pedalTitles[] = { "OCTAVE", "DRIVE", "DELAY", "REVERB" };
    const char* pedalTaglines[] = { "SUB / UPPER OCTAVE", "MID-FOCUSED OVERDRIVE", "WARM ANALOG ECHO", "ETHEREAL SPACE" };
    const float ampKnobX[] = { 65.0f, 245.0f, 385.0f, 525.0f, 705.0f, 845.0f, 985.0f };

    juce::Rectangle<int> i(float x, float y, float w, float h) { return R(x, y, w, h).toNearestInt(); }
}

// ===========================================================================
MainPanel::MainPanel(MageAudioProcessor& p)
    : processor(p),
      gain(p.state, "gain", "GAIN"), bass(p.state, "bass", "BASS"), mid(p.state, "mid", "MIDDLE"),
      treble(p.state, "treble", "TREBLE"), presence(p.state, "presence", "PRESENCE"),
      depth(p.state, "depth", "DEPTH"), master(p.state, "master", "MASTER"),
      gateThreshold(p.state, "gate", "THRESHOLD", cyan), gateRelease(p.state, "gateDecay", "RELEASE", cyan),
      gateOn(p.state, "gateEnabled", "GATE ON", cyan), gatePost(p.state, "gatePost", "POST AMP", cyan),
      lowCut(p.state, "lowCut", "LOW CUT", pink), highCut(p.state, "highCut", "HIGH CUT", pink),
      octaveMix(p.state, "octaveMix", "MIX", violet), octaveTone(p.state, "octaveTone", "TONE", violet),
      octaveLevel(p.state, "octaveLevel", "LEVEL", violet),
      octaveDown(p.state, "octaveDown", "SUB OCTAVE", violet),
      drive(p.state, "drive", "DRIVE", pink), driveTone(p.state, "driveTone", "TONE", pink),
      driveLevel(p.state, "driveLevel", "LEVEL", pink), driveBoost(p.state, "driveBoost", "BOOST", pink),
      delayTime(p.state, "delayTime", "TIME", cyan), delayDivision(p.state, "delayDivision", "NOTE", cyan),
      delayFeedback(p.state, "delayFeedback", "FEEDBACK", cyan), delayMix(p.state, "delayMix", "MIX", cyan),
      delaySync(p.state, "delaySync", "TEMPO SYNC", cyan),
      reverbDecay(p.state, "reverbDecay", "DECAY", blue), reverbSize(p.state, "reverbSize", "SIZE", blue),
      reverbTone(p.state, "reverbTone", "TONE", blue), reverbMix(p.state, "reverbMix", "MIX", blue),
      octaveSwitch(p.state, "octaveEnabled", violet), driveSwitch(p.state, "driveEnabled", pink),
      delaySwitch(p.state, "delayEnabled", cyan), reverbSwitch(p.state, "reverbEnabled", blue),
      output(p.state, "output", "VOLUME", pink)
{
    setLookAndFeel(&lnf);
    setSize(designWidth, designHeight);

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &prevPreset, &presetBox, &nextPreset, &savePreset, &deletePreset, &undoButton, &redoButton,
             &inMeter, &outMeter, &outMeterTall,
             &gain, &bass, &mid, &treble, &presence, &depth, &master,
             &gateThreshold, &gateRelease, &gateOn, &gatePost,
             &lowCut, &highCut, &loadIR, &bypassIR,
             &octaveMix, &octaveTone, &octaveLevel, &octaveDown,
             &drive, &driveTone, &driveLevel, &driveBoost,
             &delayTime, &delayDivision, &delayFeedback, &delayMix, &delaySync,
             &reverbDecay, &reverbSize, &reverbTone, &reverbMix,
             &octaveSwitch, &driveSwitch, &delaySwitch, &reverbSwitch, &output })
        addAndMakeVisible(c);

    // --- Presets ---
    presetBox.setJustificationType(juce::Justification::centredLeft);
    presetBox.setTooltip("Presets");
    presetBox.onAboutToShow = [this] { rebuildPresetList(); };
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0)
            processor.presets.loadPreset(id - 1);
        presetDisplayName.clear(); // force refresh of the label
    };
    prevPreset.onClick = [this] { processor.presets.loadPrevious(); };
    nextPreset.onClick = [this] { processor.presets.loadNext(); };
    savePreset.onClick = [this] { savePresetDialog(); };
    deletePreset.onClick = [this]
    {
        const auto name = processor.presets.getCurrentName();
        if (processor.presets.isFactory(name))
            return;
        auto options = juce::MessageBoxOptions::makeOptionsOkCancel(juce::MessageBoxIconType::QuestionIcon,
            "Delete preset", "Delete \"" + name + "\"? This cannot be undone.", "Delete", "Cancel", this);
        juce::AlertWindow::showAsync(options, [safe = juce::Component::SafePointer<MainPanel>(this), name](int result)
        {
            if (safe != nullptr && result == 1)
                safe->processor.presets.deleteUserPreset(name);
        });
    };
    undoButton.onClick = [this] { processor.undoManager.undo(); };
    redoButton.onClick = [this] { processor.undoManager.redo(); };

    // --- Cabinet ---
    loadIR.setColour(juce::TextButton::textColourOffId, pink);
    loadIR.setTooltip("Load a cabinet impulse response (.wav / .aiff)");
    loadIR.onClick = [this] { chooseCabinetIR(); };
    bypassIR.setColour(juce::TextButton::textColourOffId, muted);
    bypassIR.setTooltip("Unload the IR and use MAGE's built-in cabinet voicing");
    bypassIR.onClick = [this] { processor.bypassCabinetIR(); };

    // --- Delay: TIME knob swaps for a NOTE knob when tempo sync is on ---
    if (auto* sync = p.state.getParameter("delaySync"))
    {
        syncWatcher = std::make_unique<juce::ParameterAttachment>(*sync,
            [this](float v) { updateDelayTimeVisibility(v > 0.5f); }, nullptr);
        syncWatcher->sendInitialUpdate();
    }

    refresh();
}

MainPanel::~MainPanel()
{
    if (saveDialog != nullptr)
        saveDialog->setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void MainPanel::updateDelayTimeVisibility(bool sync)
{
    delayTime.setVisible(!sync);
    delayDivision.setVisible(sync);
}

void MainPanel::rebuildPresetList()
{
    auto& pm = processor.presets;
    const auto names = pm.getPresetNames();
    presetBox.clear(juce::dontSendNotification);
    presetBox.addSectionHeading("FACTORY");
    for (int n = 0; n < names.size(); ++n)
    {
        if (n == pm.getNumFactoryPresets())
        {
            presetBox.addSeparator();
            presetBox.addSectionHeading("USER");
        }
        presetBox.addItem(names[n], n + 1);
    }
    if (names.size() == pm.getNumFactoryPresets())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading("USER  (none saved yet)");
    }
    presetBox.setSelectedId(pm.getCurrentIndex() + 1, juce::dontSendNotification);
}

void MainPanel::savePresetDialog()
{
    const auto current = processor.presets.getCurrentName();
    saveDialog = std::make_unique<juce::AlertWindow>("Save Preset", "Name your preset:", juce::MessageBoxIconType::NoIcon, this);
    saveDialog->setLookAndFeel(&lnf);
    saveDialog->addTextEditor("name", processor.presets.isFactory(current) ? current + " (edit)" : current);
    saveDialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    saveDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    saveDialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [safe = juce::Component::SafePointer<MainPanel>(this)](int result)
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

void MainPanel::chooseCabinetIR()
{
    irChooser = std::make_unique<juce::FileChooser>("Choose a cabinet impulse response",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.wav;*.aif;*.aiff");
    irChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<MainPanel>(this)](const juce::FileChooser& chooser)
        {
            if (safe == nullptr)
                return;
            const auto file = chooser.getResult();
            if (file.existsAsFile() && !safe->processor.loadCabinetIR(file))
                juce::AlertWindow::showAsync(juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                    "Unsupported impulse response", "Please choose a WAV or AIFF file."), nullptr);
        });
}

void MainPanel::refresh()
{
    inMeter.setLevel(processor.inputPeak.exchange(0.0f));
    const float out = processor.outputPeak.exchange(0.0f);
    outMeter.setLevel(out);
    outMeterTall.setLevel(out);

    if (const bool g = processor.gateIsOpen.load(); g != gateLed)
    {
        gateLed = g;
        repaint(layout::i(layout::gate.getRight() - 40.0f, layout::gate.getY() + 12.0f, 30.0f, 30.0f));
    }

    const auto ir = processor.hasCabinetIR() ? processor.getCabinetIRName() : juce::String();
    if (ir != irDisplayName || irDisplayName.isEmpty())
    {
        irDisplayName = ir.isEmpty() ? juce::String(" ") : ir; // " " marks "built-in" so we don't repaint every tick
        repaint(layout::i(layout::cab.getX(), layout::cab.getY() + 40.0f, layout::cab.getWidth(), 70.0f));
        bypassIR.setEnabled(processor.hasCabinetIR());
    }

    auto& pm = processor.presets;
    const auto name = pm.getCurrentName() + (pm.isModified() ? "  *" : "");
    if (name != presetDisplayName && !presetBox.isPopupActive())
    {
        presetDisplayName = name;
        presetBox.setText(name, juce::dontSendNotification);
        deletePreset.setEnabled(!pm.isFactory(pm.getCurrentName()));
    }

    undoButton.setEnabled(processor.undoManager.canUndo());
    redoButton.setEnabled(processor.undoManager.canRedo());
}

// ---------------------------------------------------------------------------
void MainPanel::paint(juce::Graphics& g)
{
    const float W = (float)designWidth, H = (float)designHeight;
    g.fillAll(bg);
    juce::ColourGradient aura(cyan.withAlpha(0.10f), 0.0f, 0.0f, pink.withAlpha(0.09f), W, H, false);
    g.setGradientFill(aura);
    g.fillRect(0.0f, 0.0f, W, H);

    // ---- Header ----
    g.setColour(juce::Colour(0xff0a0f1c));
    g.fillRect(0.0f, 0.0f, W, 78.0f);
    g.setColour(juce::Colour(0xff24304a));
    g.drawLine(0.0f, 78.0f, W, 78.0f, 1.0f);
    drawCrest(g, 26.0f, 11.0f, 56.0f);
    g.setColour(cyan.withAlpha(0.85f));
    g.setFont(font(11.0f, true, 0.25f));
    g.drawText("ARCANE TONE SYSTEM", 100.0f, 24.0f, 180.0f, 16.0f, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(font(10.0f, true, 0.15f));
    g.drawText("HIGH-GAIN AMPLIFIER  v" + juce::String(JucePlugin_VersionString), 100.0f, 42.0f, 190.0f, 14.0f,
               juce::Justification::centredLeft);

    g.setFont(font(15.0f, true, 0.1f));
    g.setColour(cyan);
    g.drawText("IN", 1000.0f, 26.0f, 36.0f, 28.0f, juce::Justification::centredRight);
    g.setColour(pink);
    g.drawText("OUT", 1262.0f, 26.0f, 50.0f, 28.0f, juce::Justification::centredRight);

    // ---- Horizon wordmark ----
    g.setColour(juce::Colour(0xff14213a).withAlpha(0.55f));
    for (int n = 0; n < 17; ++n)
    {
        const float x = 18.0f + n * 96.0f;
        if (x > 600.0f && x < 960.0f)
            continue;
        juce::Path etching;
        etching.startNewSubPath(x, 112.0f);
        etching.lineTo(x + 28.0f, 88.0f);
        etching.lineTo(x + 56.0f, 112.0f);
        etching.lineTo(x + 28.0f, 136.0f);
        etching.closeSubPath();
        g.strokePath(etching, juce::PathStrokeType(0.8f));
    }
    g.setColour(cyan.withAlpha(0.8f));
    g.drawLine(0.0f, 112.0f, 628.0f, 112.0f, 1.0f);
    g.setColour(pink.withAlpha(0.8f));
    g.drawLine(972.0f, 112.0f, W, 112.0f, 1.0f);
    g.setColour(cyan);
    g.fillEllipse(624.0f, 108.5f, 7.0f, 7.0f);
    g.setColour(pink);
    g.fillEllipse(969.0f, 108.5f, 7.0f, 7.0f);
    g.setColour(ivory);
    g.setFont(font(48.0f, false, 0.55f));
    g.drawText("MAGE", 640.0f, 84.0f, 330.0f, 56.0f, juce::Justification::centred);

    // ---- Noise gate ----
    {
        const auto r = layout::gate;
        drawPanel(g, r, cyan, 13.0f);
        g.setColour(ivory);
        g.setFont(font(15.0f, true, 0.12f));
        g.drawText("NOISE GATE", r.getX() + 14.0f, r.getY() + 14.0f, r.getWidth() - 50.0f, 24.0f, juce::Justification::centredLeft);
        const juce::Point<float> led(r.getRight() - 25.0f, r.getY() + 27.0f);
        if (gateLed)
        {
            g.setColour(cyan.withAlpha(0.3f));
            g.fillEllipse(led.x - 10.0f, led.y - 10.0f, 20.0f, 20.0f);
        }
        g.setColour(gateLed ? cyan : juce::Colour(0xff2b3344));
        g.fillEllipse(led.x - 5.0f, led.y - 5.0f, 10.0f, 10.0f);
        g.setColour(gateLed ? cyan.brighter(0.5f) : muted.withAlpha(0.5f));
        g.drawEllipse(led.x - 7.0f, led.y - 7.0f, 14.0f, 14.0f, 1.2f);
    }

    // ---- Amp head ----
    {
        const auto r = layout::amp;
        drawPanel(g, r, cyan, 22.0f);
        for (int n = 0; n < 18; ++n)
            drawRune(g, r.getX() + 52.0f + n * 60.0f, r.getY() + 16.0f, 24.0f, (n < 9 ? cyan : pink).withAlpha(0.22f));
        g.setColour(cyan.withAlpha(0.25f));
        g.drawLine(r.getX() + 24.0f, r.getY() + 50.0f, r.getRight() - 24.0f, r.getY() + 50.0f, 1.0f);
        g.drawLine(r.getX() + 24.0f, r.getY() + 266.0f, r.getRight() - 24.0f, r.getY() + 266.0f, 1.0f);

        // Section captions and dividers
        g.setFont(font(11.0f, true, 0.3f));
        auto caption = [&](float x0, float x1, const char* text, juce::Colour c)
        {
            g.setColour(c.withAlpha(0.8f));
            g.drawText(text, r.getX() + x0, r.getY() + 58.0f, x1 - x0, 16.0f, juce::Justification::centred);
        };
        caption(layout::ampKnobX[0], layout::ampKnobX[0] + 130.0f, "PREAMP", cyan);
        caption(layout::ampKnobX[1], layout::ampKnobX[3] + 130.0f, "EQUALIZER", blue);
        caption(layout::ampKnobX[4], layout::ampKnobX[6] + 130.0f, "POWER", pink);
        g.setColour(juce::Colour(0xff26324a));
        for (float x : { 215.0f, 675.0f })
            g.drawLine(r.getX() + x, r.getY() + 66.0f, r.getX() + x, r.getY() + 250.0f, 1.0f);

        g.setColour(muted.withAlpha(0.8f));
        g.setFont(font(11.0f, true, 0.4f));
        g.drawText(juce::String::fromUTF8("MAGE   \xc2\xb7   ARCANE HIGH-GAIN HEAD   \xc2\xb7   MK I"), r.getX(), r.getY() + 276.0f,
                   r.getWidth(), 20.0f, juce::Justification::centred);
        for (auto pt : { r.getTopLeft().translated(26.0f, 28.0f), r.getTopRight().translated(-26.0f, 28.0f),
                         r.getBottomLeft().translated(26.0f, -28.0f), r.getBottomRight().translated(-26.0f, -28.0f) })
            drawScrew(g, pt, 9.0f, cyan);
    }

    // ---- Cabinet / IR ----
    {
        const auto r = layout::cab;
        drawPanel(g, r, pink, 13.0f);
        g.setColour(ivory);
        g.setFont(font(15.0f, true, 0.12f));
        g.drawText("CABINET / IR", r.getX() + 10.0f, r.getY() + 14.0f, r.getWidth() - 20.0f, 24.0f, juce::Justification::centred);
        const juce::Rectangle<float> screen(r.getX() + 16.0f, r.getY() + 46.0f, r.getWidth() - 32.0f, 58.0f);
        g.setColour(juce::Colour(0xff05070f));
        g.fillRoundedRectangle(screen, 6.0f);
        g.setColour(pink.withAlpha(0.35f));
        g.drawRoundedRectangle(screen, 6.0f, 1.2f);
        const bool hasIR = processor.hasCabinetIR();
        g.setColour(muted);
        g.setFont(font(9.5f, true, 0.2f));
        g.drawText(hasIR ? "IMPULSE RESPONSE" : "CABINET", screen.withHeight(22.0f), juce::Justification::centred);
        g.setColour(hasIR ? pink.brighter(0.2f) : ivory.withAlpha(0.85f));
        g.setFont(font(13.0f, true));
        g.drawFittedText(hasIR ? processor.getCabinetIRName() : "BUILT-IN 4x12",
                         screen.withTrimmedTop(20.0f).reduced(6.0f, 2.0f).toNearestInt(), juce::Justification::centred, 2);
    }

    // ---- Pedals ----
    for (int n = 0; n < 4; ++n)
    {
        const auto r = layout::pedals[n];
        const auto a = layout::pedalAccent[n];
        drawPanel(g, r, a, 17.0f);
        g.setColour(a);
        g.setFont(font(24.0f, true, 0.18f));
        g.drawText(layout::pedalTitles[n], r.getX(), r.getY() + 16.0f, r.getWidth(), 32.0f, juce::Justification::centred);
        g.setColour(muted);
        g.setFont(font(9.5f, true, 0.25f));
        g.drawText(layout::pedalTaglines[n], r.getX(), r.getY() + 48.0f, r.getWidth(), 14.0f, juce::Justification::centred);
        drawRune(g, r.getX() + 24.0f, r.getBottom() - 70.0f, 36.0f, a.withAlpha(0.3f));
        drawRune(g, r.getRight() - 60.0f, r.getBottom() - 70.0f, 36.0f, a.withAlpha(0.3f));
        for (auto pt : { r.getTopLeft().translated(16.0f, 16.0f), r.getTopRight().translated(-16.0f, 16.0f),
                         r.getBottomLeft().translated(16.0f, -16.0f), r.getBottomRight().translated(-16.0f, -16.0f) })
            drawScrew(g, pt, 5.0f, a);
    }
    // Patch cables between the pedals and into the output module
    for (float x : { 320.0f, 640.0f, 960.0f, 1280.0f })
    {
        const juce::Rectangle<float> cable(x - 8.0f, 492.0f + 182.0f, 36.0f, 11.0f);
        g.setColour(juce::Colour(0xff0b1020));
        g.fillRoundedRectangle(cable.expanded(1.5f), 5.0f);
        juce::ColourGradient glow(cyan.withAlpha(0.9f), cable.getX(), 0.0f, pink.withAlpha(0.9f), cable.getRight(), 0.0f, false);
        g.setGradientFill(glow);
        g.fillRoundedRectangle(cable.reduced(0.0f, 2.5f), 3.0f);
    }

    // ---- Output ----
    {
        const auto r = layout::output;
        drawPanel(g, r, pink, 15.0f);
        g.setColour(ivory);
        g.setFont(font(20.0f, true, 0.2f));
        g.drawText("OUTPUT", r.getX(), r.getY() + 16.0f, r.getWidth() - 40.0f, 30.0f, juce::Justification::centred);
        g.setColour(muted);
        g.setFont(font(9.5f, true, 0.25f));
        g.drawText(juce::String::fromUTF8("STEREO  \xc2\xb7  4x OVERSAMPLED"), r.getX(), r.getBottom() - 52.0f, r.getWidth() - 40.0f, 14.0f,
                   juce::Justification::centred);
    }
}

void MainPanel::resized()
{
    using layout::i;

    // Header
    prevPreset.setBounds(i(300, 19, 40, 40));
    presetBox.setBounds(i(346, 19, 330, 40));
    nextPreset.setBounds(i(682, 19, 40, 40));
    savePreset.setBounds(i(736, 19, 40, 40));
    deletePreset.setBounds(i(782, 19, 40, 40));
    undoButton.setBounds(i(850, 19, 40, 40));
    redoButton.setBounds(i(896, 19, 40, 40));
    inMeter.setBounds(i(1044, 32, 200, 14));
    outMeter.setBounds(i(1318, 32, 200, 14));

    // Amp
    {
        const auto r = layout::amp;
        Knob* knobs[] = { &gain, &bass, &mid, &treble, &presence, &depth, &master };
        for (int n = 0; n < 7; ++n)
            knobs[n]->setBounds(i(r.getX() + layout::ampKnobX[n], r.getY() + 80.0f, 130.0f, 172.0f));
    }

    // Gate
    {
        const auto r = layout::gate;
        gateThreshold.setBounds(i(r.getX() + 25.0f, r.getY() + 42.0f, 120.0f, 104.0f));
        gateRelease.setBounds(i(r.getX() + 25.0f, r.getY() + 146.0f, 120.0f, 104.0f));
        gateOn.setBounds(i(r.getX() + 24.0f, r.getY() + 254.0f, 130.0f, 24.0f));
        gatePost.setBounds(i(r.getX() + 24.0f, r.getY() + 282.0f, 130.0f, 24.0f));
    }

    // Cabinet
    {
        const auto r = layout::cab;
        loadIR.setBounds(i(r.getX() + 16.0f, r.getY() + 112.0f, r.getWidth() - 32.0f, 30.0f));
        bypassIR.setBounds(i(r.getX() + 16.0f, r.getY() + 148.0f, r.getWidth() - 32.0f, 26.0f));
        lowCut.setBounds(i(r.getX() + 10.0f, r.getY() + 186.0f, 75.0f, 112.0f));
        highCut.setBounds(i(r.getX() + 85.0f, r.getY() + 186.0f, 75.0f, 112.0f));
    }

    // Pedals
    auto placeRow = [](juce::Rectangle<float> r, std::initializer_list<juce::Component*> comps)
    {
        const float pad = comps.size() > 3 ? 8.0f : 16.0f;
        const float w = (r.getWidth() - pad * 2.0f) / (float)comps.size();
        float x = r.getX() + pad;
        for (auto* c : comps)
        {
            c->setBounds(layout::i(x, r.getY() + 102.0f, w, 126.0f));
            x += w;
        }
    };
    auto placeToggle = [](juce::Rectangle<float> r, juce::Component& t)
    {
        t.setBounds(layout::i(r.getCentreX() - 64.0f, r.getY() + 70.0f, 140.0f, 24.0f));
    };
    auto placeSwitch = [](juce::Rectangle<float> r, juce::Component& s)
    {
        s.setBounds(layout::i(r.getCentreX() - 42.0f, r.getY() + 244.0f, 84.0f, 112.0f));
    };
    placeRow(layout::pedals[0], { &octaveMix, &octaveTone, &octaveLevel });
    placeToggle(layout::pedals[0], octaveDown);
    placeSwitch(layout::pedals[0], octaveSwitch);

    placeRow(layout::pedals[1], { &drive, &driveTone, &driveLevel });
    placeToggle(layout::pedals[1], driveBoost);
    placeSwitch(layout::pedals[1], driveSwitch);

    placeRow(layout::pedals[2], { &delayTime, &delayFeedback, &delayMix });
    delayDivision.setBounds(delayTime.getBounds());
    placeToggle(layout::pedals[2], delaySync);
    placeSwitch(layout::pedals[2], delaySwitch);

    placeRow(layout::pedals[3], { &reverbDecay, &reverbSize, &reverbTone, &reverbMix });
    placeSwitch(layout::pedals[3], reverbSwitch);

    // Output
    {
        const auto r = layout::output;
        output.setBounds(i(r.getX() + 24.0f, r.getY() + 66.0f, 192.0f, 236.0f));
        outMeterTall.setBounds(i(r.getRight() - 46.0f, r.getY() + 24.0f, 18.0f, r.getHeight() - 56.0f));
    }
}

// ===========================================================================
MageAudioProcessorEditor::MageAudioProcessorEditor(MageAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p), panel(p)
{
    addAndMakeVisible(panel);
    setResizable(true, true);
    setResizeLimits(960, 540, 2560, 1440);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio((double)MainPanel::designWidth / MainPanel::designHeight);

    const int savedWidth = (int)p.state.state.getProperty("uiWidth", 1280);
    const int w = juce::jlimit(960, 2560, savedWidth);
    setSize(w, juce::roundToInt(w * (double)MainPanel::designHeight / MainPanel::designWidth));
    startTimerHz(30);
}

void MageAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
}

void MageAudioProcessorEditor::resized()
{
    const float scale = juce::jmin((float)getWidth() / MainPanel::designWidth,
                                   (float)getHeight() / MainPanel::designHeight);
    const float ox = ((float)getWidth() - MainPanel::designWidth * scale) * 0.5f;
    const float oy = ((float)getHeight() - MainPanel::designHeight * scale) * 0.5f;
    panel.setBounds(0, 0, MainPanel::designWidth, MainPanel::designHeight);
    panel.setTransform(juce::AffineTransform::scale(scale).translated(ox, oy));
    processor.state.state.setProperty("uiWidth", getWidth(), nullptr);
}

void MageAudioProcessorEditor::timerCallback()
{
    panel.refresh();
    // Group parameter edits into undo steps: one step per mouse gesture.
    if (!juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown())
        processor.undoManager.beginNewTransaction();
}
