// ============================================================================
// PLUGIN EDITOR IMPLEMENTATION
// ============================================================================
// This is the visual side of BBGVST.
//
// The editor does NOT generate audio. It changes parameters on the processor
// and draws the rack/catalog. Keeping that separation makes the project much
// easier to understand and much safer in a real-time audio plugin.
// ============================================================================

#include "PluginEditor.h"

namespace
{
    const juce::Colour bg0 (0xff07090c);
    const juce::Colour bg1 (0xff0c1015);
    const juce::Colour panel (0xff11171e);
    const juce::Colour panel2 (0xff151c24);
    const juce::Colour border (0xff29333e);
    const juce::Colour text (0xffedf3f8);
    const juce::Colour muted (0xff7f91a4);
    const juce::Colour amber (0xffffb84d);
    const juce::Colour cyan (0xff54d6ff);
    const juce::Colour violet (0xffb889ff);

    void styleButton(juce::TextButton& b, juce::Colour fill, juce::Colour fg)
    {
        b.setColour(juce::TextButton::buttonColourId, fill);
        b.setColour(juce::TextButton::textColourOffId, fg);
        b.setColour(juce::TextButton::textColourOnId, fg);
        b.setConnectedEdges(juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
    }
}

// ---------------------------------------------------------------------------
// ONE FX SLOT
// ---------------------------------------------------------------------------
// Each slot owns six rotary controls plus an ON button. Attachments keep the
// screen controls synchronized with the processor's automatable parameters.
// ---------------------------------------------------------------------------
FxSlotComponent::FxSlotComponent(BabyGirlAudioProcessor& p, int slotIndex)
    : processor(p), index(slotIndex)
{
    title.setFont(juce::Font(12.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(title);

    family.setFont(juce::Font(9.0f, juce::Font::bold));
    family.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(family);

    enabled.setButtonText("ON");
    enabled.setColour(juce::ToggleButton::textColourId, text);
    enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "SLOT" + juce::String(index + 1) + "_EN", enabled);
    addAndMakeVisible(enabled);

    clear.setTooltip("Clear this FX slot");
    clear.setButtonText("×");
    clear.onClick = [this] { if (onCleared) onCleared(index); };
    styleButton(clear, juce::Colour(0xff202731), muted);
    addAndMakeVisible(clear);

    setupKnob(mix, "SLOT" + juce::String(index + 1) + "_MIX");
    setupKnob(drive, "SLOT" + juce::String(index + 1) + "_DRIVE");
    setupKnob(tone, "SLOT" + juce::String(index + 1) + "_TONE");
    setupKnob(time, "SLOT" + juce::String(index + 1) + "_TIME");
    setupKnob(feedback, "SLOT" + juce::String(index + 1) + "_FEEDBACK");
    setupKnob(width, "SLOT" + juce::String(index + 1) + "_WIDTH");

    juce::Slider* knobs[] = { &mix, &drive, &tone, &time, &feedback, &width };
    for (int i = 0; i < 6; ++i)
    {
        knobLabels[(size_t)i].setText(BabyGirl::macroLabel(0, i), juce::dontSendNotification);
        knobLabels[(size_t)i].setFont(juce::Font(7.5f, juce::Font::bold));
        knobLabels[(size_t)i].setColour(juce::Label::textColourId, muted);
        knobLabels[(size_t)i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(knobLabels[(size_t)i]);
        knobs[i]->setTooltip(knobLabels[(size_t)i].getText());
    }
}

void FxSlotComponent::setupKnob(juce::Slider& s, const juce::String& id)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 15);
    s.setRange(0.0, 1.0, 0.001);
    s.setColour(juce::Slider::textBoxTextColourId, muted);
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b0f14));
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
    addAndMakeVisible(s);
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, id, s));
}

void FxSlotComponent::setModelIndex(int catalogIndex)
{
    if (catalogIndex < 0 || catalogIndex >= BabyGirl::kFxCatalogSize) return;
    modelIndex = catalogIndex;
    const auto& item = BabyGirl::kFxCatalog[catalogIndex];
    identity = BabyGirl::makeFxIdentity(item.brand, item.model);
    const int familyId = item.family;
    const auto accent = BabyGirl::identityColour(identity);
    juce::Slider* knobs[] = { &mix, &drive, &tone, &time, &feedback, &width };
    for (int i = 0; i < 6; ++i)
    {
        knobLabels[(size_t)i].setText(BabyGirl::macroLabel(familyId, i), juce::dontSendNotification);
        knobLabels[(size_t)i].setColour(juce::Label::textColourId, accent.withAlpha(0.78f));
        knobs[i]->setColour(juce::Slider::rotarySliderFillColourId, accent);
        knobs[i]->setTooltip(BabyGirl::macroLabel(familyId, i));
    }
    repaint();
}

void FxSlotComponent::setModelName(const juce::String& name)
{
    if (modelName == name) return;
    modelName = name;
    title.setText("SLOT " + juce::String(index + 1) + "  •  " + modelName,
                  juce::dontSendNotification);
    const auto familyId = processor.rack.getSlot(index).family;
    family.setText(BabyGirl::familyName(familyId), juce::dontSendNotification);
    if (modelIndex >= 0 && modelIndex < BabyGirl::kFxCatalogSize)
        setModelIndex(modelIndex);
    repaint();
}

void FxSlotComponent::setSelected(bool s)
{
    if (selected == s) return;
    selected = s;
    repaint();
}

// Draw the physical-looking panel around the controls.
void FxSlotComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(selected ? juce::Colour(0xff182b39) : panel);
    g.fillRoundedRectangle(b, 9.0f);

    if (selected)
    {
        g.setColour(cyan.withAlpha(0.22f));
        g.fillRoundedRectangle(b.reduced(1.5f), 8.0f);
    }

    const auto accent = BabyGirl::identityColour(identity);
    g.setColour(selected ? accent.withAlpha(0.92f) : accent.withAlpha(0.34f));
    g.drawRoundedRectangle(b, 9.0f, selected ? 1.8f : 1.0f);

    // Each model gets a deterministic physical-design archetype.
    g.setColour(accent.withAlpha(0.08f));
    if (identity.archetype == 0)
        g.fillRoundedRectangle(b.reduced(7.0f).withHeight(5.0f), 2.0f);
    else if (identity.archetype == 1)
        g.fillRect(10.0f, getHeight() - 9.0f, getWidth() - 20.0f, 2.0f);
    else if (identity.archetype == 2)
    {
        g.fillEllipse(10.0f, getHeight() - 15.0f, 5.0f, 5.0f);
        g.fillEllipse(getWidth() - 15.0f, getHeight() - 15.0f, 5.0f, 5.0f);
    }
    else if (identity.archetype == 3)
    {
        for (int i = 0; i < 5; ++i)
            g.fillRect(12.0f + i * 10.0f, 39.0f, 5.0f, 1.0f);
    }

    g.setColour(juce::Colour(0xff080b0f));
    g.fillRoundedRectangle(8.0f, 37.0f, getWidth() - 16.0f, 1.0f, 0.5f);

    // Tiny signal-chain indicator: purely visual, cheap to draw.
    const auto dot = selected ? cyan : juce::Colour(0xff394653);
    g.setColour(dot);
    g.fillEllipse(10.0f, 12.0f, 5.0f, 5.0f);
}

// JUCE calls resized() whenever the plugin window changes size.
// Keeping layout here prevents controls from drifting when the user resizes.
void FxSlotComponent::resized()
{
    title.setBounds(20, 6, getWidth() - 82, 21);
    family.setBounds(20, 24, getWidth() - 88, 12);
    enabled.setBounds(getWidth() - 62, 6, 34, 22);
    clear.setBounds(getWidth() - 27, 6, 20, 22);

    const int top = 42;
    const int cellW = juce::jmax(1, (getWidth() - 24) / 3);
    const int cellH = juce::jmax(1, (getHeight() - top - 5) / 2);
    juce::Slider* sliders[] = { &mix, &drive, &tone, &time, &feedback, &width };
    for (int i = 0; i < 6; ++i)
    {
        const int col = i % 3;
        const int row = i / 3;
        const int x = 8 + col * cellW;
        const int y = top + row * cellH;
        sliders[i]->setBounds(x, y + 8, cellW - 2, cellH - 10);
        knobLabels[(size_t)i].setBounds(x + 4, y, cellW - 10, 10);
    }
}

void FxSlotComponent::mouseDown(const juce::MouseEvent&)
{
    if (onSelected) onSelected(index);
}

int BabyGirlAudioProcessorEditor::CatalogModel::getNumRows()
{
    return (int) editor.visibleFxIndices.size();
}

void BabyGirlAudioProcessorEditor::CatalogModel::paintListBoxItem(int row, juce::Graphics& g,
                                                                   int width, int height, bool selected)
{
    if (row < 0 || row >= (int)editor.visibleFxIndices.size()) return;
    const auto& item = BabyGirl::kFxCatalog[editor.visibleFxIndices[(size_t)row]];
    g.setColour(selected ? juce::Colour(0xff19394a) : juce::Colour(0xff0f141a));
    g.fillRoundedRectangle(3.0f, 2.0f, (float)width - 6.0f, (float)height - 4.0f, 6.0f);

    g.setColour(amber);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(item.brand, 12, 5, width - 24, 15, juce::Justification::left);

    g.setColour(text);
    g.setFont(juce::Font(11.0f));
    g.drawText(item.model, 12, 21, width - 24, 16, juce::Justification::left);

    g.setColour(muted);
    g.setFont(juce::Font(8.5f));
    g.drawText(BabyGirl::familyName(item.family), 12, 38, width - 24, 12, juce::Justification::left);
}

void BabyGirlAudioProcessorEditor::CatalogModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= (int)editor.visibleFxIndices.size()) return;
    editor.selectedCatalog = editor.visibleFxIndices[(size_t)row];
    editor.catalogList.repaint();
    const auto& item = BabyGirl::kFxCatalog[editor.selectedCatalog];
    editor.selectedLabel.setText("SELECTED  •  " + juce::String(item.brand) + "  /  " +
        juce::String(item.model), juce::dontSendNotification);
}

void BabyGirlAudioProcessorEditor::CatalogModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent& event)
{
    // JUCE already gives us a valid MouseEvent here.
    // Reuse it instead of trying to construct MouseEvent ourselves.
    listBoxItemClicked(row, event);
    editor.addSelectedToSlot();
}

int BabyGirlAudioProcessorEditor::InstrumentModel::getNumRows()
{
    return (int) editor.visibleInstrumentIndices.size();
}

void BabyGirlAudioProcessorEditor::InstrumentModel::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row < 0 || row >= (int)editor.visibleInstrumentIndices.size()) return;
    const auto& item = BabyGirl::kInstrumentCatalog[editor.visibleInstrumentIndices[(size_t)row]];
    g.setColour(selected ? juce::Colour(0xff30204a) : juce::Colour(0xff0f141a));
    g.fillRoundedRectangle(3.0f, 2.0f, (float)width - 6.0f, (float)height - 4.0f, 6.0f);
    g.setColour(violet);
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText(item.brand, 12, 5, width - 24, 14, juce::Justification::left);
    g.setColour(text);
    g.setFont(juce::Font(10.5f));
    g.drawText(item.model, 12, 20, width - 24, 15, juce::Justification::left);
    g.setColour(muted);
    g.setFont(juce::Font(8.0f));
    g.drawText(BabyGirl::instrumentFamilyName(item.family), 12, 35, width - 24, 11, juce::Justification::left);
}

void BabyGirlAudioProcessorEditor::InstrumentModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= (int)editor.visibleInstrumentIndices.size()) return;
    editor.selectedInstrument = editor.visibleInstrumentIndices[(size_t)row];
    editor.instrumentList.repaint();
    editor.loadSelectedInstrument();
}

void BabyGirlAudioProcessorEditor::InstrumentModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent& event)
{
    // Reuse JUCE's event. MouseEvent has no empty/default constructor.
    listBoxItemClicked(row, event);
}

// ---------------------------------------------------------------------------
// WHOLE PLUGIN WINDOW CONSTRUCTOR
// ---------------------------------------------------------------------------
// This is where we create labels, buttons, lists, knobs, and the 16 FX slots.
// If you want to learn JUCE UI programming, study this constructor slowly.
// ---------------------------------------------------------------------------
BabyGirlAudioProcessorEditor::BabyGirlAudioProcessorEditor(BabyGirlAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), catalogModel(*this), instrumentModel(*this)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(1180, 700, 2400, 1500);
    setSize(1600, 920);

    logo.setText("BABYGIRL", juce::dontSendNotification);
    logo.setFont(juce::Font(23.0f, juce::Font::bold));
    logo.setColour(juce::Label::textColourId, amber);
    addAndMakeVisible(logo);

    rackTitle.setText("FX LAB  /  INFINITE RACK", juce::dontSendNotification);
    rackTitle.setFont(juce::Font(13.0f, juce::Font::bold));
    rackTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(rackTitle);

    rackHint.setText("16 slots  •  drag-free workflow  •  every control is MIDI/automation ready", juce::dontSendNotification);
    rackHint.setFont(juce::Font(9.0f));
    rackHint.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(rackHint);

    catalogTitle.setText("HARDWARE / PEDAL VAULT", juce::dontSendNotification);
    catalogTitle.setFont(juce::Font(12.0f, juce::Font::bold));
    catalogTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(catalogTitle);

    instrumentTitle.setText("INSTRUMENT VAULT", juce::dontSendNotification);
    instrumentTitle.setFont(juce::Font(12.0f, juce::Font::bold));
    instrumentTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(instrumentTitle);

    instrumentLabel.setText("MIDI ENGINE: " + audioProcessor.getInstrumentName(), juce::dontSendNotification);
    instrumentLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    instrumentLabel.setColour(juce::Label::textColourId, violet);
    addAndMakeVisible(instrumentLabel);

    fxCountLabel.setFont(juce::Font(8.5f, juce::Font::bold));
    fxCountLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(fxCountLabel);
    instrumentCountLabel.setFont(juce::Font(8.5f, juce::Font::bold));
    instrumentCountLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(instrumentCountLabel);

    searchBox.setTextToShowWhenEmpty("Search the vault: brand, model, family...", juce::Colour(0xff5e7184));
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff090d12));
    searchBox.setColour(juce::TextEditor::outlineColourId, border);
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, cyan);
    searchBox.onTextChange = [this] { refreshCatalog(); };
    addAndMakeVisible(searchBox);

    familyBox.addItem("ALL FX", 1);
    for (int i = 0; i <= 10; ++i) familyBox.addItem(BabyGirl::familyName(i), i + 2);
    familyBox.setSelectedId(1);
    familyBox.onChange = [this] { refreshCatalog(); };
    addAndMakeVisible(familyBox);

    styleButton(addButton, amber, juce::Colour(0xff101317));
    addButton.onClick = [this] { addSelectedToSlot(); };
    addAndMakeVisible(addButton);

    styleButton(clearButton, juce::Colour(0xff202832), text);
    clearButton.onClick = [this] { clearSelectedSlot(); };
    addAndMakeVisible(clearButton);

    styleButton(randomButton, juce::Colour(0xff17303d), cyan);
    randomButton.onClick = [this] { randomizeRack(); };
    addAndMakeVisible(randomButton);

    styleButton(clearAllButton, juce::Colour(0xff202832), muted);
    clearAllButton.onClick = [this] { clearAllSlots(); };
    addAndMakeVisible(clearAllButton);

    selectedLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    selectedLabel.setColour(juce::Label::textColourId, cyan);
    selectedLabel.setText("SELECTED  •  choose something from the vault", juce::dontSendNotification);
    addAndMakeVisible(selectedLabel);

    peakLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    peakLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(peakLabel);

    instrumentSearch.setTextToShowWhenEmpty("Search synths, drums, keys, bass...", juce::Colour(0xff5e7184));
    instrumentSearch.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff090d12));
    instrumentSearch.setColour(juce::TextEditor::outlineColourId, border);
    instrumentSearch.setColour(juce::TextEditor::focusedOutlineColourId, violet);
    instrumentSearch.onTextChange = [this] { refreshInstrumentCatalog(); };
    addAndMakeVisible(instrumentSearch);

    instrumentEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "INST_EN", instrumentOn);
    instrumentOn.setButtonText("INSTRUMENT");
    addAndMakeVisible(instrumentOn);

    auto setupInst = [this](juce::Slider& s, const juce::String& id) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 15);
        s.setColour(juce::Slider::textBoxTextColourId, muted);
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff090d12));
        addAndMakeVisible(s);
        instrumentAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, id, s));
    };
    setupInst(instrumentCutoff, "INST_CUTOFF");
    setupInst(instrumentResonance, "INST_RESONANCE");
    setupInst(instrumentAttack, "INST_ATTACK");
    setupInst(instrumentRelease, "INST_RELEASE");

    instrumentList.setModel(&instrumentModel);
    instrumentList.setRowHeight(46);
    instrumentList.setColour(juce::ListBox::backgroundColourId, bg0);
    instrumentList.setColour(juce::ListBox::outlineColourId, border);
    addAndMakeVisible(instrumentList);

    catalogList.setModel(&catalogModel);
    catalogList.setRowHeight(56);
    catalogList.setColour(juce::ListBox::backgroundColourId, bg0);
    catalogList.setColour(juce::ListBox::outlineColourId, border);
    addAndMakeVisible(catalogList);

    for (int i = 0; i < BabyGirl::FxRackDsp::kMaxSlots; ++i)
    {
        slotComponents[(size_t)i] = std::make_unique<FxSlotComponent>(audioProcessor, i);
        slotComponents[(size_t)i]->onSelected = [this](int slot) { selectSlot(slot); };
        slotComponents[(size_t)i]->onCleared = [this](int slot)
        {
            audioProcessor.clearSlot(slot);
            slotComponents[(size_t)slot]->setModelName(audioProcessor.getSlotModel(slot));
            selectSlot(slot);
        };
        slotComponents[(size_t)i]->setModelName(audioProcessor.getSlotModel(i));
        if (auto* model = audioProcessor.apvts.getRawParameterValue("SLOT" + juce::String(i + 1) + "_MODEL"))
            slotComponents[(size_t)i]->setModelIndex((int) model->load());
        addAndMakeVisible(*slotComponents[(size_t)i]);
    }

    refreshCatalog();
    refreshInstrumentCatalog();
    selectSlot(0);
    startTimerHz(20);
}

// Clean up the timer and disconnect the list models before the editor dies.
BabyGirlAudioProcessorEditor::~BabyGirlAudioProcessorEditor()
{
    stopTimer();
    catalogList.setModel(nullptr);
    instrumentList.setModel(nullptr);
    setLookAndFeel(nullptr);
}

bool BabyGirlAudioProcessorEditor::instrumentMatches(int index) const
{
    if (index < 0 || index >= BabyGirl::kInstrumentCatalogSize) return false;
    const auto& item = BabyGirl::kInstrumentCatalog[index];
    const auto q = instrumentSearch.getText().trim().toLowerCase();
    if (q.isEmpty()) return true;
    const auto hay = juce::String(item.brand) + " " + juce::String(item.model) + " " + BabyGirl::instrumentFamilyName(item.family);
    return hay.toLowerCase().contains(q);
}

// Same idea as refreshCatalog(), but for the instrument browser.
void BabyGirlAudioProcessorEditor::refreshInstrumentCatalog()
{
    visibleInstrumentIndices.clear();
    visibleInstrumentIndices.reserve((size_t)BabyGirl::kInstrumentCatalogSize);
    for (int i = 0; i < BabyGirl::kInstrumentCatalogSize; ++i)
        if (instrumentMatches(i)) visibleInstrumentIndices.push_back(i);
    instrumentCountLabel.setText(juce::String(visibleInstrumentIndices.size()) + " RESULTS / " +
                                  juce::String(BabyGirl::kInstrumentCatalogSize) + " TOTAL", juce::dontSendNotification);
    instrumentList.updateContent();
    instrumentList.repaint();
}

void BabyGirlAudioProcessorEditor::loadSelectedInstrument()
{
    if (selectedInstrument < 0 || selectedInstrument >= BabyGirl::kInstrumentCatalogSize) return;
    audioProcessor.setInstrument(selectedInstrument);
    instrumentLabel.setText("MIDI ENGINE: " + audioProcessor.getInstrumentName(), juce::dontSendNotification);
}

bool BabyGirlAudioProcessorEditor::catalogMatches(int index) const
{
    if (index < 0 || index >= BabyGirl::kFxCatalogSize) return false;
    const auto& item = BabyGirl::kFxCatalog[index];
    const int familyId = familyBox.getSelectedId() - 2;
    if (familyBox.getSelectedId() > 1 && item.family != familyId) return false;
    const auto q = searchBox.getText().trim().toLowerCase();
    if (q.isEmpty()) return true;
    const auto hay = juce::String(item.brand) + " " + juce::String(item.model) + " " + BabyGirl::familyName(item.family);
    return hay.toLowerCase().contains(q);
}

// Rebuild the visible FX list after a search/family filter changes.
void BabyGirlAudioProcessorEditor::refreshCatalog()
{
    visibleFxIndices.clear();
    visibleFxIndices.reserve((size_t)BabyGirl::kFxCatalogSize);
    for (int i = 0; i < BabyGirl::kFxCatalogSize; ++i)
        if (catalogMatches(i)) visibleFxIndices.push_back(i);
    fxCountLabel.setText(juce::String(visibleFxIndices.size()) + " RESULTS / " +
                          juce::String(BabyGirl::kFxCatalogSize) + " TOTAL", juce::dontSendNotification);
    catalogList.updateContent();
    catalogList.repaint();
}

void BabyGirlAudioProcessorEditor::selectSlot(int slot)
{
    selectedSlot = juce::jlimit(0, BabyGirl::FxRackDsp::kMaxSlots - 1, slot);
    for (int i = 0; i < BabyGirl::FxRackDsp::kMaxSlots; ++i)
        slotComponents[(size_t)i]->setSelected(i == selectedSlot);
    selectedLabel.setText("TARGET SLOT " + juce::String(selectedSlot + 1) + "  •  " +
                          audioProcessor.getSlotModel(selectedSlot), juce::dontSendNotification);
}

void BabyGirlAudioProcessorEditor::addSelectedToSlot()
{
    if (selectedCatalog < 0 || selectedCatalog >= BabyGirl::kFxCatalogSize) return;
    audioProcessor.loadCatalogItem(selectedSlot, selectedCatalog);
    slotComponents[(size_t)selectedSlot]->setModelName(audioProcessor.getSlotModel(selectedSlot));
    slotComponents[(size_t)selectedSlot]->setModelIndex(selectedCatalog);
    selectSlot(selectedSlot);
}

void BabyGirlAudioProcessorEditor::clearSelectedSlot()
{
    audioProcessor.clearSlot(selectedSlot);
    slotComponents[(size_t)selectedSlot]->setModelName(audioProcessor.getSlotModel(selectedSlot));
    selectSlot(selectedSlot);
}

// Turn off every FX slot and return its display to the empty state.
void BabyGirlAudioProcessorEditor::clearAllSlots()
{
    for (int i = 0; i < BabyGirl::FxRackDsp::kMaxSlots; ++i)
    {
        audioProcessor.clearSlot(i);
        slotComponents[(size_t)i]->setModelName(audioProcessor.getSlotModel(i));
    }
    selectSlot(selectedSlot);
}

// Fill the rack with random catalog choices. This changes parameters; it does
// not create new DSP code at runtime.
void BabyGirlAudioProcessorEditor::randomizeRack()
{
    if (BabyGirl::kFxCatalogSize <= 0) return;
    juce::Random r;
    const int count = 3 + r.nextInt(juce::jmax(1, BabyGirl::FxRackDsp::kMaxSlots - 3 + 1));
    clearAllSlots();
    for (int i = 0; i < count; ++i)
    {
        const int cat = r.nextInt(BabyGirl::kFxCatalogSize);
        audioProcessor.loadCatalogItem(i, cat);
        slotComponents[(size_t)i]->setModelName(audioProcessor.getSlotModel(i));
        slotComponents[(size_t)i]->setModelIndex(cat);
        for (const auto id : {"MIX", "DRIVE", "TONE", "TIME", "FEEDBACK", "WIDTH"})
        {
            const auto paramId = "SLOT" + juce::String(i + 1) + "_" + id;
            if (auto* param = audioProcessor.apvts.getParameter(paramId))
                param->setValueNotifyingHost(r.nextFloat());
        }
    }
    selectSlot(0);
}

void BabyGirlAudioProcessorEditor::toggleInstrument()
{
    if (auto* p = audioProcessor.apvts.getParameter("INST_EN"))
        p->setValueNotifyingHost(p->getValue() > 0.5f ? 0.0f : 1.0f);
}

// The timer gives the UI a safe, slow update loop for meters and display text.
void BabyGirlAudioProcessorEditor::timerCallback()
{
    const float peakDb = audioProcessor.currentPeak > 1.0e-5f ? juce::Decibels::gainToDecibels(audioProcessor.currentPeak) : -100.0f;
    const float rmsDb = audioProcessor.currentRms > 1.0e-5f ? juce::Decibels::gainToDecibels(audioProcessor.currentRms) : -100.0f;
    peakLabel.setText("PEAK " + juce::String(peakDb, 1) + " dBFS   RMS " + juce::String(rmsDb, 1) + " dBFS", juce::dontSendNotification);
    repaint();
}

void BabyGirlAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg0);

    // Top hardware strip.
    auto top = getLocalBounds().removeFromTop(64);
    juce::ColourGradient topGrad(juce::Colour(0xff111820), 0.0f, 0.0f,
                                 juce::Colour(0xff090d12), (float)getWidth(), 0.0f, false);
    g.setGradientFill(topGrad);
    g.fillRect(top);
    g.setColour(amber);
    g.fillRect(0, 62, getWidth(), 2);

    // Left vault and main rack.
    const int leftW = juce::jlimit(330, 390, getWidth() / 4);
    g.setColour(bg1);
    g.fillRect(0, 64, leftW, getHeight() - 64);
    g.setColour(border);
    g.drawVerticalLine(leftW, 64.0f, (float)getHeight());

    const int rackX = leftW + 1;
    g.setColour(juce::Colour(0xff0a0e13));
    g.fillRect(rackX, 64, getWidth() - rackX, getHeight() - 64);

    // Rack header plate.
    g.setColour(panel2);
    g.fillRoundedRectangle((float)rackX + 14.0f, 76.0f, (float)getWidth() - rackX - 28.0f, 42.0f, 8.0f);
    g.setColour(border);
    g.drawRoundedRectangle((float)rackX + 14.0f, 76.0f, (float)getWidth() - rackX - 28.0f, 42.0f, 8.0f, 1.0f);

    // Meter.
    const float meterW = 160.0f;
    const float meterX = (float)getWidth() - meterW - 34.0f;
    const float meterY = 95.0f;
    g.setColour(juce::Colour(0xff080b0f));
    g.fillRoundedRectangle(meterX, meterY, meterW, 5.0f, 2.0f);
    const float peakNorm = juce::jlimit(0.0f, 1.0f, (audioProcessor.currentPeak + 60.0f) / 60.0f);
    g.setColour(peakNorm > 0.95f ? juce::Colour(0xffff5b63) : cyan);
    g.fillRoundedRectangle(meterX, meterY, meterW * peakNorm, 5.0f, 2.0f);

    // Subtle grid behind rack slots.
    const int areaX = rackX + 14;
    const int areaY = 128;
    const int areaW = getWidth() - areaX - 14;
    const int areaH = getHeight() - areaY - 14;
    const int cols = areaW >= 1250 ? 4 : 3;
    const int rows = (BabyGirl::FxRackDsp::kMaxSlots + cols - 1) / cols;
    const int gap = 9;
    const int cellW = juce::jmax(160, (areaW - gap * (cols - 1)) / cols);
    const int cellH = juce::jmax(120, (areaH - gap * (rows - 1)) / rows);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
        {
            const float x = (float)(areaX + c * (cellW + gap));
            const float y = (float)(areaY + r * (cellH + gap));
            g.setColour(juce::Colour(0xff0b1015));
            g.fillRoundedRectangle(x, y, (float)cellW, (float)cellH, 9.0f);
        }
}

// ---------------------------------------------------------------------------
// WINDOW LAYOUT
// ---------------------------------------------------------------------------
// All screen coordinates live here. When the editor is resized, this function
// calculates where every child component should go.
// ---------------------------------------------------------------------------
void BabyGirlAudioProcessorEditor::resized()
{
    const int leftW = juce::jlimit(330, 390, getWidth() / 4);
    logo.setBounds(18, 13, 150, 30);
    peakLabel.setBounds(getWidth() - 350, 15, 180, 22);

    const int x = 14;
    catalogTitle.setBounds(x, 76, leftW - 28, 20);
    fxCountLabel.setBounds(x, 94, leftW - 28, 14);
    searchBox.setBounds(x, 111, leftW - 28, 29);
    familyBox.setBounds(x, 146, leftW - 28, 28);
    addButton.setBounds(x, 181, (leftW - 40) / 2, 28);
    clearButton.setBounds(x + (leftW - 40) / 2 + 6, 181, (leftW - 40) / 2 - 6, 28);
    selectedLabel.setBounds(x, 214, leftW - 28, 28);
    catalogList.setBounds(x, 246, leftW - 28, juce::jmax(180, getHeight() - 610));

    instrumentTitle.setBounds(x, getHeight() - 352, leftW - 28, 20);
    instrumentCountLabel.setBounds(x, getHeight() - 334, leftW - 28, 14);
    instrumentSearch.setBounds(x, getHeight() - 315, leftW - 28, 28);
    instrumentOn.setBounds(x, getHeight() - 282, 118, 24);
    instrumentLabel.setBounds(x + 120, getHeight() - 282, leftW - 148, 24);
    instrumentList.setBounds(x, getHeight() - 250, leftW - 28, 236);

    const int rackX = leftW + 1;
    rackTitle.setBounds(rackX + 30, 83, 260, 20);
    rackHint.setBounds(rackX + 30, 101, 420, 15);
    randomButton.setBounds(getWidth() - 315, 83, 92, 25);
    clearAllButton.setBounds(getWidth() - 216, 83, 92, 25);

    const int areaX = rackX + 14;
    const int areaY = 128;
    const int areaW = getWidth() - areaX - 14;
    const int areaH = getHeight() - areaY - 14;
    const int cols = areaW >= 1250 ? 4 : 3;
    const int rows = (BabyGirl::FxRackDsp::kMaxSlots + cols - 1) / cols;
    const int gap = 9;
    const int cellW = juce::jmax(160, (areaW - gap * (cols - 1)) / cols);
    const int cellH = juce::jmax(120, (areaH - gap * (rows - 1)) / rows);
    for (int i = 0; i < BabyGirl::FxRackDsp::kMaxSlots; ++i)
    {
        const int col = i % cols;
        const int row = i / cols;
        slotComponents[(size_t)i]->setBounds(areaX + col * (cellW + gap), areaY + row * (cellH + gap), cellW, cellH);
    }
}
