// ============================================================================
// PLUGIN EDITOR HEADER
// ============================================================================
// This file describes the controls shown on screen.
//
// FxSlotComponent = one visual FX slot.
// BabyGirlAudioProcessorEditor = the whole plugin window.
//
// The UI talks to the processor through JUCE's AudioProcessorValueTreeState.
// That is what lets controls also work with DAW automation and saved presets.
// ============================================================================

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "Identity.h"

class FxSlotComponent : public juce::Component
{
public:
    FxSlotComponent(BabyGirlAudioProcessor& p, int slotIndex);
    ~FxSlotComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void setModelName(const juce::String& name);
    void setModelIndex(int catalogIndex);
    void setSelected(bool selected);
    std::function<void(int)> onSelected;
    std::function<void(int)> onCleared;

private:
    BabyGirlAudioProcessor& processor;
    int index;
    juce::Label title;
    juce::Label family;
    juce::ToggleButton enabled{"ON"};
    juce::TextButton clear{"×"};
    juce::Slider mix, drive, tone, time, feedback, width;
    std::array<juce::Label, 6> knobLabels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment;
    juce::String modelName{"Empty FX Slot"};
    bool selected = false;
    int modelIndex = 0;
    BabyGirl::FxIdentity identity;

    void setupKnob(juce::Slider&, const juce::String&);
    void mouseDown(const juce::MouseEvent&) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxSlotComponent)
};

class BabyGirlAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit BabyGirlAudioProcessorEditor(BabyGirlAudioProcessor&);
    ~BabyGirlAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class CatalogModel : public juce::ListBoxModel
    {
    public:
        explicit CatalogModel(BabyGirlAudioProcessorEditor& owner) : editor(owner) {}
        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics&, int width, int height, bool selected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    private:
        BabyGirlAudioProcessorEditor& editor;
    };

    class InstrumentModel : public juce::ListBoxModel
    {
    public:
        explicit InstrumentModel(BabyGirlAudioProcessorEditor& owner) : editor(owner) {}
        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics&, int width, int height, bool selected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    private:
        BabyGirlAudioProcessorEditor& editor;
    };

    void timerCallback() override;
    void refreshCatalog();
    void refreshInstrumentCatalog();
    void randomizeRack();
    void clearAllSlots();
    void toggleInstrument();
    bool instrumentMatches(int index) const;
    void loadSelectedInstrument();
    void addSelectedToSlot();
    void clearSelectedSlot();
    void selectSlot(int slot);
    bool catalogMatches(int index) const;

    BabyGirlAudioProcessor& audioProcessor;
    BabyGirl::BabyGirlLookAndFeel lookAndFeel;

    juce::Label logo, catalogTitle, selectedLabel, peakLabel, instrumentTitle, instrumentLabel;
    juce::Label rackTitle, rackHint, fxCountLabel, instrumentCountLabel;
    juce::TextEditor searchBox, instrumentSearch;
    juce::ComboBox familyBox;
    juce::TextButton addButton{"ADD TO SLOT"};
    juce::TextButton clearButton{"CLEAR"};
    juce::TextButton randomButton{"RANDOMIZE"};
    juce::TextButton clearAllButton{"CLEAR RACK"};
    juce::ListBox catalogList, instrumentList;
    CatalogModel catalogModel;
    InstrumentModel instrumentModel;
    juce::ToggleButton instrumentOn{"INSTRUMENT"};
    juce::Slider instrumentCutoff, instrumentResonance, instrumentAttack, instrumentRelease;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> instrumentAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> instrumentEnabledAttachment;
    int selectedInstrument = 0;
    std::array<std::unique_ptr<FxSlotComponent>, BabyGirl::FxRackDsp::kMaxSlots> slotComponents;
    int selectedCatalog = 0;
    int selectedSlot = 0;
    std::vector<int> visibleFxIndices;
    std::vector<int> visibleInstrumentIndices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BabyGirlAudioProcessorEditor)
};
