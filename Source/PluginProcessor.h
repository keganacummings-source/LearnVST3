// ============================================================================
// PLUGIN PROCESSOR HEADER
// ============================================================================
// This is the "engine" half of the plugin.
//
// Beginner idea:
//   PluginEditor = what the human sees.
//   PluginProcessor = what the computer hears/plays.
//
// The processor receives audio and MIDI from the DAW, creates our simple
// instrument sound, sends audio through the FX rack, and reports meters.
// ============================================================================

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FxRackDsp.h"
#include "FxCatalog.h"
#include "InstrumentCatalog.h"

class BabyGirlAudioProcessor : public juce::AudioProcessor
{
public:
    BabyGirlAudioProcessor();
    ~BabyGirlAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BabyGirl FX"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void loadCatalogItem(int slot, int catalogIndex);
    void clearSlot(int slot);
    juce::String getSlotModel(int slot) const;
    int getCatalogSize() const { return BabyGirl::kFxCatalogSize; }
    int getInstrumentCatalogSize() const { return BabyGirl::kInstrumentCatalogSize; }
    void setInstrument(int index);
    juce::String getInstrumentName() const { return instrumentName; }

    juce::AudioProcessorValueTreeState apvts;
    BabyGirl::FxRackDsp rack;
    float currentPeak = 0.0f;
    float currentRms = 0.0f;
    bool instrumentEnabled = true;

private:
    std::array<juce::String, BabyGirl::FxRackDsp::kMaxSlots> slotModels;
    juce::String instrumentName{"Analog Poly"};
    int instrumentIndex = 0;
    double synthPhase[4] { 0.0, 0.0, 0.0, 0.0 };
    double synthPhase2[4] { 0.0, 0.0, 0.0, 0.0 };
    float synthEnv[4] { 0, 0, 0, 0 };
    int synthNotes[4] { -1, -1, -1, -1 };
    float synthVelocity[4] { 0, 0, 0, 0 };
    float synthCutoff = 0.65f;
    float synthResonance = 0.15f;
    float synthAttack = 0.01f;
    float synthRelease = 0.20f;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BabyGirlAudioProcessor)
};
