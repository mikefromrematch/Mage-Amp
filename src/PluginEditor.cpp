#include "PluginEditor.h"
#include "BinaryData.h"
#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

using namespace mage::ui;
using namespace mage::ui::colours;

namespace layout
{
    using R = juce::Rectangle<float>;
    const R gate   { 20.0f, 148.0f, 170.0f, 158.0f };
    const R comp   { 20.0f, 318.0f, 170.0f, 152.0f };
    const R amp    { 210.0f, 148.0f, 1180.0f, 322.0f };
    const R cab    { 1410.0f, 148.0f, 170.0f, 322.0f };
    const R pedals[] = { { 20.0f, 492.0f, 300.0f, 388.0f }, { 340.0f, 492.0f, 300.0f, 388.0f },
                         { 660.0f, 492.0f, 300.0f, 388.0f }, { 980.0f, 492.0f, 300.0f, 388.0f } };
    const R output { 1300.0f, 492.0f, 280.0f, 388.0f };
    // Indexed by mage::Pedal.
    const char* pedalTitles[mage::numPedals] = { "OCTAVE", "DRIVE", "SHIMMER", "PHASER", "CHORUS", "DELAY", "REVERB" };
    const char* pedalTaglines[2][mage::numPedals] = {
        { "UPPER OCTAVE FUZZ", "MID-FOCUSED OVERDRIVE", "OCTAVE-UP HALO",
          "ARCANE VORTEX", "SPECTRAL DOUBLING", "WARM ANALOG ECHO", "ETHEREAL SPACE" },
        { "UPPER OCTAVE FUZZ", "MID-FOCUSED OVERDRIVE", "OCTAVE-UP HALO",
          "MOONTIDE SWEEP", "FAERIE CHORUS", "MOONLIT ECHO", "FAERIE GLADE" } };
    const Rune pedalRunes[mage::numPedals] = { Rune::octave, Rune::drive, Rune::shimmer,
                                               Rune::phaser, Rune::chorus, Rune::delay, Rune::reverb };
    // x centre of each gap an amp marker can sit in (before slot 0 ... after slot 3).
    const float ampGapX[] = { 20.0f, 330.0f, 650.0f, 970.0f, 1290.0f };
    constexpr float cableY = 492.0f + 187.5f;
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
      gateOn(p.state, "gateEnabled", "ON", cyan), gatePost(p.state, "gatePost", "POST", cyan),
      lowCut(p.state, "lowCut", "LOW CUT", pink), highCut(p.state, "highCut", "HIGH CUT", pink),
      octaveMix(p.state, "octaveMix", "MIX", violet), octaveTone(p.state, "octaveTone", "TONE", violet),
      octaveLevel(p.state, "octaveLevel", "LEVEL", violet),
      drive(p.state, "drive", "DRIVE", pink), driveTone(p.state, "driveTone", "TONE", pink),
      driveLevel(p.state, "driveLevel", "LEVEL", pink), driveBoost(p.state, "driveBoost", "BOOST", pink),
      delayTime(p.state, "delayTime", "TIME", cyan), delayDivision(p.state, "delayDivision", "NOTE", cyan),
      delayFeedback(p.state, "delayFeedback", "FEEDBACK", cyan), delayMix(p.state, "delayMix", "MIX", cyan),
      delaySync(p.state, "delaySync", "TEMPO SYNC", cyan),
      reverbDecay(p.state, "reverbDecay", "DECAY", blue), reverbSize(p.state, "reverbSize", "SIZE", blue),
      reverbTone(p.state, "reverbTone", "TONE", blue), reverbMix(p.state, "reverbMix", "MIX", blue),
      octaveSwitch(p.state, "octaveEnabled", violet), driveSwitch(p.state, "driveEnabled", pink),
      delaySwitch(p.state, "delayEnabled", cyan), reverbSwitch(p.state, "reverbEnabled", blue),
      output(p.state, "output", "VOLUME", pink),
      channelSwitch(p.state, "ampChannel"),
      shimmerMix(p.state, "shimmerMix", "MIX", violet), shimmerDecay(p.state, "shimmerDecay", "DECAY", violet),
      shimmerAmount(p.state, "shimmerAmount", "SHIMMER", violet), shimmerTone(p.state, "shimmerTone", "TONE", violet),
      shimmerFifth(p.state, "shimmerFifth", "FIFTH", violet),
      shimmerSwitch(p.state, "shimmerEnabled", violet),
      phaserRate(p.state, "phaserRate", "RATE"), phaserDepth(p.state, "phaserDepth", "DEPTH"),
      phaserFeedback(p.state, "phaserFeedback", "RESO"), phaserMix(p.state, "phaserMix", "MIX"),
      phaserStages(p.state, "phaserStages", "8-STAGE", violet),
      chorusRate(p.state, "chorusRate", "RATE"), chorusDepth(p.state, "chorusDepth", "DEPTH"),
      chorusTone(p.state, "chorusTone", "TONE"), chorusMix(p.state, "chorusMix", "MIX"),
      chorusVibrato(p.state, "chorusVibrato", "VIBRATO", violet),
      phaserSwitch(p.state, "phaserEnabled", violet), chorusSwitch(p.state, "chorusEnabled", violet),
      compSustain(p.state, "compSustain", "SUSTAIN", violet), compLevel(p.state, "compLevel", "LEVEL", violet),
      compOn(p.state, "compEnabled", "ON", violet), compParallel(p.state, "compParallel", "BLEND", violet)
{
    setLookAndFeel(&lnf);
    setSize(designWidth, designHeight);

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &prevPreset, &presetBox, &nextPreset, &savePreset, &deletePreset, &undoButton, &redoButton,
             &inMeter, &outMeter, &outMeterTall,
             &frieze, &gain, &bass, &mid, &treble, &presence, &depth, &master,
             &gateThreshold, &gateRelease, &gateOn, &gatePost,
             &lowCut, &highCut, &loadIR, &bypassIR,
             &octaveMix, &octaveTone, &octaveLevel,
             &drive, &driveTone, &driveLevel, &driveBoost,
             &delayTime, &delayDivision, &delayFeedback, &delayMix, &delaySync,
             &reverbDecay, &reverbSize, &reverbTone, &reverbMix,
             &octaveSwitch, &driveSwitch, &delaySwitch, &reverbSwitch, &output,
             &channelSwitch, &shimmerMix, &shimmerDecay, &shimmerAmount, &shimmerTone, &shimmerFifth,
             &shimmerSwitch,
             &phaserRate, &phaserDepth, &phaserFeedback, &phaserMix, &phaserStages, &phaserSwitch,
             &chorusRate, &chorusDepth, &chorusTone, &chorusMix, &chorusVibrato, &chorusSwitch,
             &compSustain, &compLevel, &compOn, &compParallel })
        addAndMakeVisible(c);

    // --- Pedalboard ---
    pedalViews = { {
        { { &octaveMix, &octaveTone, &octaveLevel }, nullptr, &octaveSwitch },
        { { &drive, &driveTone, &driveLevel }, &driveBoost, &driveSwitch },
        { { &shimmerMix, &shimmerDecay, &shimmerAmount, &shimmerTone }, &shimmerFifth, &shimmerSwitch },
        { { &phaserRate, &phaserDepth, &phaserFeedback, &phaserMix }, &phaserStages, &phaserSwitch },
        { { &chorusRate, &chorusDepth, &chorusTone, &chorusMix }, &chorusVibrato, &chorusSwitch },
        { { &delayTime, &delayFeedback, &delayMix }, &delaySync, &delaySwitch },
        { { &reverbDecay, &reverbSize, &reverbTone, &reverbMix }, nullptr, &reverbSwitch } } };
    for (int s = 0; s < mage::numSlots; ++s)
    {
        addAndMakeVisible(slotTitles[(size_t)s]);
        slotTitles[(size_t)s].onClick = [this, s] { showSlotMenu(s); };
    }
    for (int g = 0; g <= mage::numSlots; ++g)
    {
        addAndMakeVisible(ampMarkers[(size_t)g]);
        ampMarkers[(size_t)g].onClick = [this, g] { setBoardParam(mage::ampPosParamID(faeChannel ? 1 : 0), g); };
    }
    for (int ch = 0; ch < 2; ++ch)
    {
        juce::StringArray ids { mage::ampPosParamID(ch) };
        for (int s = 0; s < mage::numSlots; ++s)
            ids.add(mage::slotParamID(ch, s));
        for (const auto& id : ids)
            if (auto* param = p.state.getParameter(id))
                boardWatchers.push_back(std::make_unique<juce::ParameterAttachment>(*param,
                    [this](float) { layoutPedals(); applyTheme(); }, nullptr));
    }
    phaserStages.setTooltip("Eight all-pass stages for a deeper, more dramatic sweep");
    chorusVibrato.setTooltip("Pure pitch vibrato instead of chorus (turn MIX up for the full effect)");
    compParallel.setTooltip("Blend the compressed signal with the dry one to keep your pick attack");
    gateOn.setTooltip("Noise gate on/off");
    gatePost.setTooltip("Gate after the amp instead of before it");
    compOn.setTooltip("Compressor on/off");

    // --- MIDI Learn: right-click any of these to map it to a MIDI CC ---
    learnables = {
        { &gain, "gain" }, { &bass, "bass" }, { &mid, "mid" }, { &treble, "treble" }, { &presence, "presence" },
        { &depth, "depth" }, { &master, "master" }, { &gateThreshold, "gate" }, { &gateRelease, "gateDecay" },
        { &gateOn, "gateEnabled" }, { &gatePost, "gatePost" }, { &lowCut, "lowCut" }, { &highCut, "highCut" },
        { &octaveMix, "octaveMix" }, { &octaveTone, "octaveTone" }, { &octaveLevel, "octaveLevel" },
        { &drive, "drive" }, { &driveTone, "driveTone" }, { &driveLevel, "driveLevel" },
        { &driveBoost, "driveBoost" }, { &delayTime, "delayTime" }, { &delayDivision, "delayDivision" },
        { &delayFeedback, "delayFeedback" }, { &delayMix, "delayMix" }, { &delaySync, "delaySync" },
        { &reverbDecay, "reverbDecay" }, { &reverbSize, "reverbSize" }, { &reverbTone, "reverbTone" },
        { &reverbMix, "reverbMix" }, { &octaveSwitch, "octaveEnabled" }, { &driveSwitch, "driveEnabled" },
        { &delaySwitch, "delayEnabled" }, { &reverbSwitch, "reverbEnabled" }, { &output, "output" },
        { &channelSwitch, "ampChannel" }, { &shimmerMix, "shimmerMix" }, { &shimmerDecay, "shimmerDecay" },
        { &shimmerAmount, "shimmerAmount" }, { &shimmerTone, "shimmerTone" }, { &shimmerFifth, "shimmerFifth" },
        { &shimmerSwitch, "shimmerEnabled" },
        { &phaserRate, "phaserRate" }, { &phaserDepth, "phaserDepth" }, { &phaserFeedback, "phaserFeedback" },
        { &phaserMix, "phaserMix" }, { &phaserStages, "phaserStages" }, { &phaserSwitch, "phaserEnabled" },
        { &chorusRate, "chorusRate" }, { &chorusDepth, "chorusDepth" }, { &chorusTone, "chorusTone" },
        { &chorusMix, "chorusMix" }, { &chorusVibrato, "chorusVibrato" }, { &chorusSwitch, "chorusEnabled" },
        { &compSustain, "compSustain" }, { &compLevel, "compLevel" }, { &compOn, "compEnabled" },
        { &compParallel, "compParallel" } };
    for (auto& [control, id] : learnables)
        control->addMouseListener(this, true);

    shimmerFifth.setTooltip("Shift the shimmer up a fifth instead of an octave");

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

    // --- Channel: swaps the first two pedals and the whole theme ---
    if (auto* ch = p.state.getParameter("ampChannel"))
    {
        channelWatcher = std::make_unique<juce::ParameterAttachment>(*ch,
            [this](float v) { setChannel(v > 0.5f); }, nullptr);
        channelWatcher->sendInitialUpdate();
    }

    refresh();
}

MainPanel::~MainPanel()
{
    processor.midiLearn.cancelLearning();
    if (saveDialog != nullptr)
        saveDialog->setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void MainPanel::updateDelayTimeVisibility(bool)
{
    layoutPedals();
}

// Read board settings from the parameter objects, not the raw values: the raw values
// update after parameter listeners run, so layout code called from a listener would
// otherwise see the previous board.
static int currentIndex(juce::AudioProcessorValueTreeState& state, const juce::String& id)
{
    auto* param = state.getParameter(id);
    return param != nullptr ? juce::roundToInt(param->convertFrom0to1(param->getValue())) : 0;
}

int MainPanel::boardSlot(int slot) const
{
    return juce::jlimit(0, mage::numPedals - 1, currentIndex(processor.state, mage::slotParamID(faeChannel ? 1 : 0, slot)));
}

int MainPanel::ampPosition() const
{
    return juce::jlimit(0, mage::numSlots, currentIndex(processor.state, mage::ampPosParamID(faeChannel ? 1 : 0)));
}

void MainPanel::setBoardParam(const juce::String& paramID, int value)
{
    if (auto* param = processor.state.getParameter(paramID))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1((float)value));
        param->endChangeGesture();
    }
}

void MainPanel::layoutPedals()
{
    if (pedalViews[0].footswitch == nullptr)
        return;   // resized() runs from setSize() before the board is set up
    for (auto& view : pedalViews)
    {
        for (auto* k : view.knobs) k->setVisible(false);
        if (view.toggle != nullptr) view.toggle->setVisible(false);
        view.footswitch->setVisible(false);
    }
    delayDivision.setVisible(false);

    const bool sync = currentIndex(processor.state, "delaySync") > 0;
    for (int s = 0; s < mage::numSlots; ++s)
    {
        const auto r = layout::pedals[s];
        const int type = boardSlot(s);
        auto& view = pedalViews[(size_t)type];

        const float pad = view.knobs.size() > 3 ? 8.0f : 16.0f;
        const float w = (r.getWidth() - pad * 2.0f) / (float)view.knobs.size();
        float x = r.getX() + pad;
        for (auto* k : view.knobs)
        {
            k->setBounds(layout::i(x, r.getY() + 102.0f, w, 126.0f));
            k->setVisible(true);
            x += w;
        }
        if (view.toggle != nullptr)
        {
            view.toggle->setBounds(layout::i(r.getCentreX() - 64.0f, r.getY() + 70.0f, 140.0f, 24.0f));
            view.toggle->setVisible(true);
        }
        view.footswitch->setBounds(layout::i(r.getCentreX() - 42.0f, r.getY() + 244.0f, 84.0f, 112.0f));
        view.footswitch->setVisible(true);
        if (type == (int)mage::Pedal::delay)
        {
            delayDivision.setBounds(delayTime.getBounds());
            delayTime.setVisible(!sync);
            delayDivision.setVisible(sync);
        }
        slotTitles[(size_t)s].setBounds(layout::i(r.getX() + 24.0f, r.getY() + 10.0f, r.getWidth() - 48.0f, 56.0f));
    }
    const int amp = ampPosition();
    for (int g = 0; g <= mage::numSlots; ++g)
    {
        ampMarkers[(size_t)g].setBounds(layout::i(layout::ampGapX[g] - 21.0f, layout::cableY - 21.0f, 42.0f, 42.0f));
        ampMarkers[(size_t)g].setActive(g == amp);
    }
    repaint(layout::i(0.0f, 480.0f, 1300.0f, 410.0f));
}

void MainPanel::showSlotMenu(int slot)
{
    const int ch = faeChannel ? 1 : 0;
    const int current = boardSlot(slot);
    auto slotOf = [this](int type) { for (int s = 0; s < mage::numSlots; ++s) if (boardSlot(s) == type) return s; return -1; };

    juce::PopupMenu menu;
    menu.addSectionHeader("SLOT " + juce::String(slot + 1));
    for (int t = 0; t < mage::numPedals; ++t)
    {
        const int other = slotOf(t);
        auto text = mage::pedalNames()[t];
        if (other >= 0 && other != slot)
            text << "   (swap with slot " << (other + 1) << ")";
        menu.addItem(100 + t, text, true, t == current);
    }
    menu.addSeparator();
    const int amp = ampPosition();
    menu.addItem(200, "Amp before this pedal", true, amp == slot);
    menu.addItem(201, "Amp after this pedal", true, amp == slot + 1);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&slotTitles[(size_t)slot]),
        [safe = juce::Component::SafePointer<MainPanel>(this), slot, ch, current, slotOf](int result)
        {
            if (safe == nullptr || result == 0)
                return;
            if (result >= 100 && result < 100 + mage::numPedals)
            {
                const int type = result - 100;
                if (type == current)
                    return;
                const int other = slotOf(type);
                safe->setBoardParam(mage::slotParamID(ch, slot), type);
                if (other >= 0)
                    safe->setBoardParam(mage::slotParamID(ch, other), current);
            }
            else if (result == 200 || result == 201)
            {
                safe->setBoardParam(mage::ampPosParamID(ch), slot + (result - 200));
            }
        });
}

juce::Colour MainPanel::pedalAccent(int slot) const
{
    const auto& t = *theme;
    const juce::Colour arcane[] = { t.violet, t.pink, t.cyan, t.blue };
    const juce::Colour fae[] = { t.violet, t.cyan, t.pink, t.blue };
    return (faeChannel ? fae : arcane)[juce::jlimit(0, 3, slot)];
}

void MainPanel::setChannel(bool fae)
{
    faeChannel = fae;
    theme = fae ? &faeTheme() : &arcaneTheme();
    layoutPedals();
    applyTheme();
}

void MainPanel::applyTheme()
{
    useTheme(*theme);
    lnf.applyTheme(*theme);
    for (auto* k : { &gateThreshold, &gateRelease }) k->setAccent(cyan);
    for (auto* t : { &gateOn, &gatePost }) t->setAccent(cyan);
    for (auto* k : { &lowCut, &highCut, &output }) k->setAccent(pink);
    for (auto* k : { &compSustain, &compLevel }) k->setAccent(violet);
    for (auto* t : { &compOn, &compParallel }) t->setAccent(violet);
    // Pedal colours follow the slot, so the board always reads as the same rainbow.
    for (int s = 0; s < mage::numSlots; ++s)
    {
        const auto a = pedalAccent(s);
        auto& view = pedalViews[(size_t)boardSlot(s)];
        for (auto* k : view.knobs) k->setAccent(a);
        if (view.toggle != nullptr) view.toggle->setAccent(a);
        view.footswitch->setAccent(a);
        if (boardSlot(s) == (int)mage::Pedal::delay) delayDivision.setAccent(a);
        slotTitles[(size_t)s].accent = a;
    }
    inMeter.setColours(blue, cyan);
    outMeter.setColours(violet, pink);
    outMeterTall.setColours(violet, pink);
    loadIR.setColour(juce::TextButton::textColourOffId, pink);
    bypassIR.setColour(juce::TextButton::textColourOffId, muted);
    presetBox.setColour(juce::ComboBox::backgroundColourId, theme->control);
    presetBox.setColour(juce::ComboBox::outlineColourId, blue.withAlpha(0.75f));
    presetBox.setColour(juce::ComboBox::textColourId, ivory);
    presetBox.setColour(juce::ComboBox::arrowColourId, muted);
    repaint();
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
        repaint(layout::i(layout::gate.getRight() - 30.0f, layout::gate.getY() + 12.0f, 24.0f, 24.0f));
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

    updateMidiHighlight();
}

// ---------------------------------------------------------------------------
// MIDI Learn
juce::Component* MainPanel::findLearnable(const juce::String& paramID) const
{
    for (auto& [control, id] : learnables)
        if (id == paramID)
            return control;
    return nullptr;
}

void MainPanel::mouseDown(const juce::MouseEvent& e)
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

void MainPanel::showMidiMenu(juce::Component& control, const juce::String& paramID)
{
    auto& learn = processor.midiLearn;
    const int cc = learn.getCCFor(paramID);
    const bool learningThis = learn.getLearningParam() == paramID;

    juce::PopupMenu menu;
    if (auto* param = processor.state.getParameter(paramID))
        menu.addSectionHeader(param->getName(40) + (cc >= 0 ? "  -  MIDI CC " + juce::String(cc) : juce::String()));
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
        [safe = juce::Component::SafePointer<MainPanel>(this), paramID](int result)
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

juce::Rectangle<int> MainPanel::midiBadgeArea(const juce::Component& c) const
{
    const auto b = c.getBounds();
    return { b.getCentreX() - 92, b.getY() - 26, 184, 22 };
}

void MainPanel::updateMidiHighlight()
{
    auto& learn = processor.midiLearn;
    if (const int learned = learn.lastLearnedParam.exchange(-1); learned >= 0)
    {
        flashParam = learn.getParamID(learned);
        flashFrames = 50;              // ~1.7 s at 30 Hz
        learn.saveAsDefault();         // new instances start with the same map
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

void MainPanel::paintOverChildren(juce::Graphics& g)
{
    auto* c = findLearnable(highlightParam);
    if (c == nullptr || !c->isVisible())
        return;
    useTheme(*theme);

    const bool learning = processor.midiLearn.getLearningParam() == highlightParam;
    const float t = (float)(juce::Time::getMillisecondCounter() % 1200) / 1200.0f;
    const float pulse = learning ? 0.55f + 0.45f * std::sin(t * juce::MathConstants<float>::twoPi)
                                 : (float)flashFrames / 50.0f;
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
    g.drawText(learning ? juce::String("MOVE A MIDI CONTROL") : "MIDI CC " + juce::String(cc), badge,
               juce::Justification::centred, false);
}

// ---------------------------------------------------------------------------
void MainPanel::paint(juce::Graphics& g)
{
    useTheme(*theme);
    const auto& T = *theme;
    const bool fae = T.fae;
    const float W = (float)designWidth, H = (float)designHeight;
    g.fillAll(bg);
    juce::ColourGradient aura(cyan.withAlpha(0.10f), 0.0f, 0.0f, pink.withAlpha(0.09f), W, H, false);
    g.setGradientFill(aura);
    g.fillRect(0.0f, 0.0f, W, H);

    // ---- Header ----
    g.setColour(T.header);
    g.fillRect(0.0f, 0.0f, W, 78.0f);
    g.setColour(T.headerLine);
    g.drawLine(0.0f, 78.0f, W, 78.0f, 1.0f);
    drawLogo(g, Logo::mark, { 20.0f, 4.0f, 70.0f, 70.0f });
    g.setColour(cyan.withAlpha(0.85f));
    g.setFont(font(11.0f, true, 0.25f));
    g.drawText(fae ? "FAE TONE SYSTEM" : "ARCANE TONE SYSTEM", 100.0f, 24.0f, 180.0f, 16.0f, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(font(10.0f, true, 0.15f));
    g.drawText(juce::String(fae ? "CLEAN AMPLIFIER  v" : "HIGH-GAIN AMPLIFIER  v") + JucePlugin_VersionString,
               100.0f, 42.0f, 190.0f, 14.0f, juce::Justification::centredLeft);

    g.setFont(font(15.0f, true, 0.1f));
    g.setColour(cyan);
    g.drawText("IN", 1000.0f, 26.0f, 36.0f, 28.0f, juce::Justification::centredRight);
    g.setColour(pink);
    g.drawText("OUT", 1262.0f, 26.0f, 50.0f, 28.0f, juce::Justification::centredRight);

    // ---- Horizon wordmark ----
    // The brush "Mage" wordmark sits on the horizon line; the lines and etchings stop short of it.
    const float markH = 90.0f;
    const float markW = markH * logoAspect(Logo::wordmark);
    const float gapL = W * 0.5f - markW * 0.5f - 6.0f, gapR = W * 0.5f + markW * 0.5f + 6.0f;

    for (int n = 0; n < 17; ++n)
    {
        const float x = 18.0f + (float)n * 96.0f;
        if (x + 56.0f > gapL - 10.0f && x < gapR + 10.0f)
            continue;
        juce::Path etching;
        if (fae)
        {
            // Little four-point stars, alternating large and small.
            const float cx = x + 28.0f, cy = 112.0f, rr = (n % 2 == 0) ? 15.0f : 8.0f;
            etching.startNewSubPath(cx, cy - rr);
            etching.quadraticTo(cx, cy, cx + rr, cy);
            etching.quadraticTo(cx, cy, cx, cy + rr);
            etching.quadraticTo(cx, cy, cx - rr, cy);
            etching.quadraticTo(cx, cy, cx, cy - rr);
            etching.closeSubPath();
            g.setColour(T.etching.withAlpha(0.8f));
        }
        else
        {
            etching.startNewSubPath(x, 112.0f);
            etching.lineTo(x + 28.0f, 88.0f);
            etching.lineTo(x + 56.0f, 112.0f);
            etching.lineTo(x + 28.0f, 136.0f);
            etching.closeSubPath();
            g.setColour(T.etching.withAlpha(0.55f));
        }
        g.strokePath(etching, juce::PathStrokeType(0.8f));
    }
    g.setColour(cyan.withAlpha(0.8f));
    g.drawLine(0.0f, 112.0f, gapL, 112.0f, 1.0f);
    g.setColour(pink.withAlpha(0.8f));
    g.drawLine(gapR, 112.0f, W, 112.0f, 1.0f);
    g.setColour(cyan);
    g.fillEllipse(gapL - 4.0f, 108.5f, 7.0f, 7.0f);
    g.setColour(pink);
    g.fillEllipse(gapR - 3.0f, 108.5f, 7.0f, 7.0f);
    drawLogo(g, Logo::wordmark, { W * 0.5f - markW * 0.5f, 112.0f - markH * 0.5f, markW, markH });

    // ---- Noise gate ----
    {
        const auto r = layout::gate;
        drawPanel(g, r, cyan, 13.0f);
        drawRuneTitle(g, Rune::gate, "NOISE GATE", { r.getX(), r.getY() + 12.0f, r.getWidth() - 22.0f, 24.0f },
                      font(13.0f, true, 0.1f), ivory, 18.0f);
        // Gate-open lamp in the top-right corner.
        const juce::Point<float> led(r.getRight() - 18.0f, r.getY() + 24.0f);
        if (gateLed)
        {
            g.setColour(cyan.withAlpha(0.3f));
            g.fillEllipse(led.x - 10.0f, led.y - 10.0f, 20.0f, 20.0f);
        }
        g.setColour(gateLed ? cyan : T.ledOff);
        g.fillEllipse(led.x - 5.0f, led.y - 5.0f, 10.0f, 10.0f);
        g.setColour(gateLed ? cyan.brighter(0.5f) : muted.withAlpha(0.5f));
        g.drawEllipse(led.x - 7.0f, led.y - 7.0f, 14.0f, 14.0f, 1.2f);
    }

    // ---- Compressor ----
    {
        const auto r = layout::comp;
        drawPanel(g, r, violet, 13.0f);
        drawRuneTitle(g, Rune::compressor, "COMPRESSOR", { r.getX(), r.getY() + 12.0f, r.getWidth(), 24.0f },
                      font(13.0f, true, 0.1f), ivory, 18.0f);
    }

    // ---- Amp head ----
    {
        const auto r = layout::amp;
        drawPanel(g, r, cyan, 22.0f);
        // The animated magic frieze (a child component) fills the band above this line.
        g.setColour(cyan.withAlpha(0.25f));
        g.drawLine(r.getX() + 24.0f, r.getY() + 54.0f, r.getRight() - 24.0f, r.getY() + 54.0f, 1.0f);
        g.drawLine(r.getX() + 24.0f, r.getY() + 266.0f, r.getRight() - 24.0f, r.getY() + 266.0f, 1.0f);

        // Section captions and dividers
        auto caption = [&](float x0, float x1, Rune rune, const char* text, juce::Colour c)
        {
            drawRuneTitle(g, rune, text, { r.getX() + x0, r.getY() + 58.0f, x1 - x0, 20.0f },
                          font(11.0f, true, 0.3f), c.withAlpha(0.85f), 18.0f);
        };
        caption(layout::ampKnobX[0], layout::ampKnobX[0] + 130.0f, Rune::gain, "PREAMP", cyan);
        caption(layout::ampKnobX[1], layout::ampKnobX[3] + 130.0f, Rune::equalizer, "EQUALIZER", blue);
        caption(layout::ampKnobX[4], layout::ampKnobX[6] + 130.0f, Rune::power, "POWER", pink);
        g.setColour(T.divider);
        for (float x : { 215.0f, 675.0f })
            g.drawLine(r.getX() + x, r.getY() + 66.0f, r.getX() + x, r.getY() + 250.0f, 1.0f);

        // The channel switch sits in the middle of this strip.
        g.setColour(muted.withAlpha(0.8f));
        g.setFont(font(11.0f, true, 0.4f));
        g.drawText(juce::String::fromUTF8("MAGE   \xc2\xb7"), r.getCentreX() - 340.0f, r.getY() + 276.0f, 110.0f, 20.0f,
                   juce::Justification::centredRight);
        g.drawText(juce::String::fromUTF8("\xc2\xb7   MK I"), r.getCentreX() + 230.0f, r.getY() + 276.0f, 110.0f, 20.0f,
                   juce::Justification::centredLeft);
        for (auto pt : { r.getTopLeft().translated(26.0f, 28.0f), r.getTopRight().translated(-26.0f, 28.0f),
                         r.getBottomLeft().translated(26.0f, -28.0f), r.getBottomRight().translated(-26.0f, -28.0f) })
            drawScrew(g, pt, 9.0f, cyan);
    }

    // ---- Cabinet / IR ----
    {
        const auto r = layout::cab;
        drawPanel(g, r, pink, 13.0f);
        drawRuneTitle(g, Rune::cabinet, "CABINET / IR", { r.getX(), r.getY() + 14.0f, r.getWidth(), 24.0f },
                      font(14.0f, true, 0.1f), ivory, 20.0f);
        const juce::Rectangle<float> screen(r.getX() + 16.0f, r.getY() + 46.0f, r.getWidth() - 32.0f, 58.0f);
        g.setColour(T.screen);
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
        const auto a = pedalAccent(n);
        const int ch = fae ? 1 : 0;
        const int type = boardSlot(n);
        const Rune rune = layout::pedalRunes[type];
        drawPanel(g, r, a, 17.0f);
        const auto titleFont = font(24.0f, true, 0.18f);
        drawRuneTitle(g, rune, layout::pedalTitles[type], { r.getX(), r.getY() + 16.0f, r.getWidth(), 32.0f },
                      titleFont, a, 30.0f);
        // Chevron after the title: click the title to swap the pedal.
        {
            const float tw = 30.0f * 1.4f + juce::GlyphArrangement::getStringWidth(titleFont, layout::pedalTitles[type]);
            const float cx = r.getCentreX() + tw * 0.5f + 12.0f, cy = r.getY() + 33.0f;
            juce::Path chevron;
            chevron.startNewSubPath(cx - 5.0f, cy - 2.5f);
            chevron.lineTo(cx, cy + 2.5f);
            chevron.lineTo(cx + 5.0f, cy - 2.5f);
            g.setColour(a.withAlpha(0.7f));
            g.strokePath(chevron, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour(muted);
        g.setFont(font(9.5f, true, 0.25f));
        g.drawText(layout::pedalTaglines[ch][type], r.getX(), r.getY() + 48.0f, r.getWidth(), 14.0f, juce::Justification::centred);
        drawSectionRune(g, rune, { r.getX() + 24.0f, r.getBottom() - 72.0f, 38.0f, 38.0f }, 0.35f, a);
        drawSectionRune(g, rune, { r.getRight() - 62.0f, r.getBottom() - 72.0f, 38.0f, 38.0f }, 0.35f, a);
        for (auto pt : { r.getTopLeft().translated(16.0f, 16.0f), r.getTopRight().translated(-16.0f, 16.0f),
                         r.getBottomLeft().translated(16.0f, -16.0f), r.getBottomRight().translated(-16.0f, -16.0f) })
            drawScrew(g, pt, 5.0f, a);
    }
    // Patch cables between the pedals and into the output module
    for (float x : { 320.0f, 640.0f, 960.0f, 1280.0f })
    {
        const juce::Rectangle<float> cable(x - 8.0f, layout::cableY - 5.5f, 36.0f, 11.0f);
        g.setColour(T.header);
        g.fillRoundedRectangle(cable.expanded(1.5f), 5.0f);
        juce::ColourGradient glow(cyan.withAlpha(0.9f), cable.getX(), 0.0f, pink.withAlpha(0.9f), cable.getRight(), 0.0f, false);
        g.setGradientFill(glow);
        g.fillRoundedRectangle(cable.reduced(0.0f, 2.5f), 3.0f);
    }

    // ---- Output ----
    {
        const auto r = layout::output;
        drawPanel(g, r, pink, 15.0f);
        drawRuneTitle(g, Rune::output, "OUTPUT", { r.getX(), r.getY() + 16.0f, r.getWidth() - 40.0f, 30.0f },
                      font(20.0f, true, 0.2f), ivory, 24.0f);
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
        frieze.setBounds(i(r.getX() + 44.0f, r.getY() + 9.0f, r.getWidth() - 88.0f, 42.0f));
        Knob* knobs[] = { &gain, &bass, &mid, &treble, &presence, &depth, &master };
        for (int n = 0; n < 7; ++n)
            knobs[n]->setBounds(i(r.getX() + layout::ampKnobX[n], r.getY() + 80.0f, 130.0f, 172.0f));
        channelSwitch.setBounds(i(r.getCentreX() - 215.0f, r.getY() + 272.0f, 430.0f, 28.0f));
    }

    // Gate
    {
        const auto r = layout::gate;
        gateThreshold.setBounds(i(r.getX() + 10.0f, r.getY() + 36.0f, 75.0f, 84.0f));
        gateRelease.setBounds(i(r.getX() + 85.0f, r.getY() + 36.0f, 75.0f, 84.0f));
        gateOn.setBounds(i(r.getX() + 16.0f, r.getY() + 122.0f, 60.0f, 24.0f));
        gatePost.setBounds(i(r.getX() + 86.0f, r.getY() + 122.0f, 80.0f, 24.0f));
    }

    // Compressor
    {
        const auto r = layout::comp;
        compSustain.setBounds(i(r.getX() + 10.0f, r.getY() + 36.0f, 75.0f, 84.0f));
        compLevel.setBounds(i(r.getX() + 85.0f, r.getY() + 36.0f, 75.0f, 84.0f));
        compOn.setBounds(i(r.getX() + 16.0f, r.getY() + 120.0f, 60.0f, 24.0f));
        compParallel.setBounds(i(r.getX() + 86.0f, r.getY() + 120.0f, 80.0f, 24.0f));
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
    layoutPedals();

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

   #if JucePlugin_Build_Standalone
    // JUCE's standalone app mutes the input by default to avoid speaker feedback, which
    // just looks like a dead input on a guitar amp. Unmute it once; after that the
    // "Mute audio input" choice in Settings is remembered as usual.
    if (p.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            if (holder->settings != nullptr && !holder->settings->getBoolValue("mageInputUnmuted", false))
            {
                holder->getMuteInputValue().setValue(false);
                holder->settings->setValue("mageInputUnmuted", true);
            }
   #endif
}

void MageAudioProcessorEditor::paint(juce::Graphics& g)
{
    useTheme(panel.getTheme());
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
