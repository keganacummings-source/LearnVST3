// ============================================================================
// PLUGIN PROCESSOR IMPLEMENTATION
// ============================================================================
// This file contains the actual audio-engine code declared in
// PluginProcessor.h. Read it from top to bottom.
//
// A very simple mental model:
//   MIDI note -> tiny synth -> FX rack -> output audio
//
// A DAW repeatedly calls processBlock(). That function must be fast because
// it runs on the real-time audio thread. Avoid file I/O, network calls,
// long loops, or memory allocations there unless you really know why.
// ============================================================================

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Identity.h"

// ---------------------------------------------------------------------------
// STEP 1: Include the headers.
// ---------------------------------------------------------------------------
// PluginProcessor.h contains the class declaration.
// PluginEditor.h is needed because createEditor() creates the UI class.
// Identity.h contains the small hash helper used for model personalities.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CONSTRUCTOR
// ---------------------------------------------------------------------------
// The constructor runs once when the plugin instance is created. We create
// stereo input/output buses and build the list of automatable parameters.
// ---------------------------------------------------------------------------
BabyGirlAudioProcessor::BabyGirlAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (auto& n : slotModels) n = "Empty FX Slot";
}

// ---------------------------------------------------------------------------
// PARAMETER LIST
// ---------------------------------------------------------------------------
// Every knob/button that a DAW should be able to save or automate needs a
// parameter. We create them here once when the processor is constructed.
// ---------------------------------------------------------------------------
juce::AudioProcessorValueTreeState::ParameterLayout BabyGirlAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    for (int i = 0; i < BabyGirl::FxRackDsp::kMaxSlots; ++i)
    {
        const auto n = juce::String(i + 1);
        p.push_back(std::make_unique<juce::AudioParameterBool>("SLOT" + n + "_EN", "Slot " + n + " Enabled", false));
        juce::StringArray types;
        types.add("Drive/Fuzz"); types.add("Dynamics"); types.add("EQ/Tone");
        types.add("Delay/Echo"); types.add("Reverb/Space"); types.add("Modulation");
        types.add("Filter"); types.add("Gate/De-Ess"); types.add("Pitch/Doubler");
        types.add("Looper/Freeze"); types.add("Creative");
        p.push_back(std::make_unique<juce::AudioParameterChoice>("SLOT" + n + "_TYPE", "Slot " + n + " Family", types, 0));
        p.push_back(std::make_unique<juce::AudioParameterInt>("SLOT" + n + "_MODEL", "Slot " + n + " Model", 0, BabyGirl::kFxCatalogSize - 1, 0));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("SLOT" + n + "_MIX", "Slot " + n + " Mix", 0.0f, 1.0f, 0.85f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("SLOT" + n + "_DRIVE", "Slot " + n + " Drive", 0.0f, 1.0f, 0.35f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("SLOT" + n + "_TONE", "Slot " + n + " Tone", 0.0f, 1.0f, 0.55f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("SLOT" + n + "_TIME", "Slot " + n + " Time", 0.0f, 1.0f, 0.35f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("SLOT" + n + "_FEEDBACK", "Slot " + n + " Feedback", 0.0f, 0.95f, 0.25f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("SLOT" + n + "_WIDTH", "Slot " + n + " Width", 0.0f, 1.0f, 0.5f));
    }
    p.push_back(std::make_unique<juce::AudioParameterBool>("INST_EN", "Instrument Enabled", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("INST_ENGINE", "Instrument Engine",
        juce::StringArray{"Analog Poly", "Mono Synth", "FM Keys", "Bass Synth", "Drum Synth", "Organ", "EPiano", "Strings", "Brass", "Sampler", "Texture"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>("INST_MODEL", "Instrument Model", 0, BabyGirl::kInstrumentCatalogSize - 1, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("INST_CUTOFF", "Instrument Filter", 0.02f, 1.0f, 0.65f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("INST_RESONANCE", "Instrument Resonance", 0.0f, 0.95f, 0.15f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("INST_ATTACK", "Instrument Attack", 0.001f, 1.0f, 0.01f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("INST_RELEASE", "Instrument Release", 0.01f, 3.0f, 0.20f));
    return { p.begin(), p.end() };
}

// ---------------------------------------------------------------------------
// PREPARE TO PLAY
// ---------------------------------------------------------------------------
// The DAW calls this before audio starts. We receive the sample rate and
// maximum block size and give those values to our DSP objects.
// ---------------------------------------------------------------------------
void BabyGirlAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    rack.prepare(sampleRate, samplesPerBlock);
    std::fill(std::begin(synthPhase), std::end(synthPhase), 0.0);
    std::fill(std::begin(synthPhase2), std::end(synthPhase2), 0.0);
    std::fill(std::begin(synthEnv), std::end(synthEnv), 0.0f);
    std::fill(std::begin(synthNotes), std::end(synthNotes), -1);
}

void BabyGirlAudioProcessor::releaseResources()
{
    rack.reset();
}

bool BabyGirlAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out;
}

// ---------------------------------------------------------------------------
// THE AUDIO LOOP
// ---------------------------------------------------------------------------
// THIS IS THE MOST IMPORTANT FUNCTION IN THE WHOLE PROJECT.
//
// The DAW calls processBlock() over and over. Each call contains a small
// block of audio samples plus any MIDI messages that happened in that block.
//
// Our simplified signal flow is:
//   1. Read instrument parameters.
//   2. Turn MIDI notes into oscillator samples.
//   3. Copy parameter values into the FX rack.
//   4. Process the audio through the 16-slot rack.
//   5. Measure peak/RMS for the UI.
// ---------------------------------------------------------------------------
void BabyGirlAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto* en = apvts.getRawParameterValue("INST_EN");
    const auto* engine = apvts.getRawParameterValue("INST_ENGINE");
    const auto* instModel = apvts.getRawParameterValue("INST_MODEL");
    const auto* cutoff = apvts.getRawParameterValue("INST_CUTOFF");
    const auto* resonance = apvts.getRawParameterValue("INST_RESONANCE");
    const auto* attack = apvts.getRawParameterValue("INST_ATTACK");
    const auto* release = apvts.getRawParameterValue("INST_RELEASE");
    instrumentEnabled = en == nullptr || en->load() > 0.5f;
    synthCutoff = cutoff ? cutoff->load() : 0.65f;
    synthResonance = resonance ? resonance->load() : 0.15f;
    synthAttack = attack ? attack->load() : 0.01f;
    synthRelease = release ? release->load() : 0.20f;

    // MIDI is an event list. We only care about note-on and note-off here.
    // A future beginner exercise could add pitch bend, sustain, modulation,
    // or MIDI CC messages.
    for (const auto metadata : midiMessages)
    {
        const auto m = metadata.getMessage();
        if (m.isNoteOn())
        {
            int voice = -1;
            for (int i = 0; i < 4; ++i) if (synthNotes[i] < 0) { voice = i; break; }
            if (voice < 0) voice = 0;
            synthNotes[voice] = m.getNoteNumber();
            synthVelocity[voice] = m.getFloatVelocity();
            synthEnv[voice] = 0.0f;
            synthPhase[voice] = 0.0;
            synthPhase2[voice] = 0.0;
        }
        else if (m.isNoteOff())
        {
            for (int i = 0; i < 4; ++i)
                if (synthNotes[i] == m.getNoteNumber()) synthEnv[i] *= 0.65f;
        }
    }

    // ---------------------------------------------------------------
    // SYNTHESIZER SECTION
    // ---------------------------------------------------------------
    // If the instrument is enabled, generate a tiny four-voice synth.
    // This is intentionally simple so a beginner can trace every step.
    // ---------------------------------------------------------------
    if (instrumentEnabled && buffer.getNumSamples() > 0)
    {
        const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        const int mode = engine ? (int)engine->load() : 0;
        const int model = instModel ? (int)instModel->load() : instrumentIndex;
        const auto modelSeed = (model >= 0 && model < BabyGirl::kInstrumentCatalogSize)
            ? BabyGirl::stableHash(BabyGirl::kInstrumentCatalog[model].brand, BabyGirl::kInstrumentCatalog[model].model) : 0x12345678u;
        const float modelA = float((modelSeed >> 4) & 0xffu) / 255.0f;
        const float modelB = float((modelSeed >> 12) & 0xffu) / 255.0f;
        const float modelC = float((modelSeed >> 20) & 0xffu) / 255.0f;
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            float sample = 0.0f;
            for (int v = 0; v < 4; ++v)
            {
                if (synthNotes[v] < 0 || synthEnv[v] < 0.0005f) continue;
                const double hz = 440.0 * std::pow(2.0, (synthNotes[v] - 69) / 12.0);
                const double inc = hz * (0.992 + modelA * 0.016) / sr;
                const double p = synthPhase[v];
                const double p2 = synthPhase2[v];
                double osc = 0.0;
                switch (mode)
                {
                    case 1: osc = (p < 0.5 ? 1.0 : -1.0); break;
                    case 2: osc = 2.0 * p - 1.0; break;
                    case 3: osc = std::sin(juce::MathConstants<double>::twoPi * p) * (0.65 + 0.35 * std::sin(juce::MathConstants<double>::twoPi * p2)); break;
                    case 4: osc = std::sin(juce::MathConstants<double>::twoPi * p) * 0.8 + std::sin(juce::MathConstants<double>::twoPi * p * 2.01) * 0.2; break;
                    case 5: osc = std::sin(juce::MathConstants<double>::twoPi * p) + 0.35 * std::sin(juce::MathConstants<double>::twoPi * p * 3.0); break;
                    case 6: osc = std::sin(juce::MathConstants<double>::twoPi * p) * 0.75 + std::sin(juce::MathConstants<double>::twoPi * p * 2.0) * 0.25; break;
                    case 7: osc = std::sin(juce::MathConstants<double>::twoPi * p) * 0.55 + std::sin(juce::MathConstants<double>::twoPi * p * 0.998) * 0.45; break;
                    case 8: osc = std::sin(juce::MathConstants<double>::twoPi * p) + 0.25 * std::sin(juce::MathConstants<double>::twoPi * p * 2.0) + 0.15 * std::sin(juce::MathConstants<double>::twoPi * p * 3.0); break;
                    case 9: osc = std::sin(juce::MathConstants<double>::twoPi * p) * (0.5 + 0.5 * synthEnv[v]); break;
                    default: osc = 0.5 * std::sin(juce::MathConstants<double>::twoPi * p) + 0.25 * std::sin(juce::MathConstants<double>::twoPi * p * 2.0) + 0.15 * std::sin(juce::MathConstants<double>::twoPi * p * 3.0); break;
                }
                const float character = 0.75f + modelB * 0.5f;
                const float fmDepth = (mode == 2 || mode == 10) ? (0.15f + modelC * 1.2f) : (0.02f + modelC * 0.18f);
                osc += std::sin(juce::MathConstants<double>::twoPi * p2 * (1.0 + modelA * 5.0)) * fmDepth * 0.08;
                osc *= character;
                const float attackCoeff = 1.0f - std::exp(-1.0f / (float)(sr * std::max(0.001, (double)synthAttack)));
                const float releaseCoeff = std::exp(-1.0f / (float)(sr * std::max(0.01, (double)synthRelease)));
                synthEnv[v] += attackCoeff * (1.0f - synthEnv[v]);
                sample += (float)osc * synthVelocity[v] * synthEnv[v] * 0.18f;
                synthPhase[v] += inc;
                synthPhase2[v] += inc * 1.007;
                if (synthPhase[v] >= 1.0) synthPhase[v] -= 1.0;
                if (synthPhase2[v] >= 1.0) synthPhase2[v] -= 1.0;
                synthEnv[v] *= releaseCoeff;
                if (synthEnv[v] < 0.0005f) synthNotes[v] = -1;
            }
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addSample(ch, n, sample);
        }
    }

    // ---------------------------------------------------------------
    // COPY UI PARAMETERS INTO THE DSP RACK
    // ---------------------------------------------------------------
    // AudioProcessorValueTreeState stores the host-facing values. The rack
    // has its own small Slot structs for fast audio processing. Here we copy
    // the current values into those structs before calling rack.process().
    // ---------------------------------------------------------------
    for (int slot = 0; slot < BabyGirl::FxRackDsp::kMaxSlots; ++slot)
    {
        const auto n = juce::String(slot + 1);
        auto& s = rack.getSlot(slot);
        auto loadParam = [this](const juce::String& id, float fallback) {
            if (auto* v = apvts.getRawParameterValue(id)) return v->load();
            return fallback;
        };
        s.enabled = loadParam("SLOT" + n + "_EN", 0.0f) > 0.5f;
        s.family = juce::jlimit(0, 10, (int) loadParam("SLOT" + n + "_TYPE", 0.0f));
        s.modelIndex = juce::jlimit(0, BabyGirl::kFxCatalogSize - 1, (int) loadParam("SLOT" + n + "_MODEL", 0.0f));
        if (s.modelIndex >= 0 && s.modelIndex < BabyGirl::kFxCatalogSize)
            s.modelSeed = BabyGirl::stableHash(BabyGirl::kFxCatalog[s.modelIndex].brand, BabyGirl::kFxCatalog[s.modelIndex].model);
        s.mix = juce::jlimit(0.0f, 1.0f, loadParam("SLOT" + n + "_MIX", 0.85f));
        s.drive = juce::jlimit(0.0f, 1.0f, loadParam("SLOT" + n + "_DRIVE", 0.35f));
        s.tone = juce::jlimit(0.0f, 1.0f, loadParam("SLOT" + n + "_TONE", 0.55f));
        s.time = juce::jlimit(0.0f, 1.0f, loadParam("SLOT" + n + "_TIME", 0.35f));
        s.feedback = juce::jlimit(0.0f, 0.95f, loadParam("SLOT" + n + "_FEEDBACK", 0.25f));
        s.width = juce::jlimit(0.0f, 1.0f, loadParam("SLOT" + n + "_WIDTH", 0.5f));
    }
    // Now the audio passes through every enabled FX slot in order.
    rack.process(buffer);

    float peak = 0.0f; double sum = 0.0;
    const int channels = std::min(2, buffer.getNumChannels());
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        for (int ch = 0; ch < channels; ++ch) { const float x = buffer.getSample(ch, i); peak = std::max(peak, std::abs(x)); sum += double(x) * double(x); }
    currentPeak = peak;
    currentRms = channels > 0 && buffer.getNumSamples() > 0 ? (float)std::sqrt(sum / double(channels * buffer.getNumSamples())) : 0.0f;
}

// ---------------------------------------------------------------------------
// LOAD A CATALOG ITEM INTO A RACK SLOT
// ---------------------------------------------------------------------------
// The UI calls this when the user chooses an effect. Notice that the catalog
// only supplies data; this function turns that data into parameter values.
// ---------------------------------------------------------------------------
void BabyGirlAudioProcessor::loadCatalogItem(int slot, int catalogIndex)
{
    if (slot < 0 || slot >= BabyGirl::FxRackDsp::kMaxSlots ||
        catalogIndex < 0 || catalogIndex >= BabyGirl::kFxCatalogSize)
        return;

    const auto& item = BabyGirl::kFxCatalog[catalogIndex];
    rack.setSlotFamily(slot, item.family);
    rack.setSlotModel(slot, catalogIndex, BabyGirl::stableHash(item.brand, item.model));
    slotModels[(size_t)slot] = juce::String(item.brand) + " • " + juce::String(item.model);

    const auto id = juce::String(slot + 1);
    if (auto* en = apvts.getParameter("SLOT" + id + "_EN"))
        en->setValueNotifyingHost(1.0f);
    if (auto* model = apvts.getParameter("SLOT" + id + "_MODEL"))
        model->setValueNotifyingHost(model->getNormalisableRange().convertTo0to1((float)catalogIndex));
    if (auto* ty = apvts.getParameter("SLOT" + id + "_TYPE"))
    {
        const float normalized = (float)item.family / 10.0f;
        ty->setValueNotifyingHost(normalized);
    }
}

// ---------------------------------------------------------------------------
// CLEAR ONE SLOT
// ---------------------------------------------------------------------------
void BabyGirlAudioProcessor::clearSlot(int slot)
{
    if (slot < 0 || slot >= BabyGirl::FxRackDsp::kMaxSlots) return;
    slotModels[(size_t)slot] = "Empty FX Slot";
    rack.setSlotModel(slot, 0, 0);
    const auto id = juce::String(slot + 1);
    if (auto* en = apvts.getParameter("SLOT" + id + "_EN"))
        en->setValueNotifyingHost(0.0f);
}

juce::String BabyGirlAudioProcessor::getSlotModel(int slot) const
{
    if (slot < 0 || slot >= BabyGirl::FxRackDsp::kMaxSlots) return {};
    return slotModels[(size_t)slot];
}

// ---------------------------------------------------------------------------
// SELECT AN INSTRUMENT
// ---------------------------------------------------------------------------
// The catalog tells us what kind of instrument this is. We translate that
// family into one of the small synthesis engines used in processBlock().
// ---------------------------------------------------------------------------
void BabyGirlAudioProcessor::setInstrument(int index)
{
    if (index < 0 || index >= BabyGirl::kInstrumentCatalogSize) return;
    instrumentIndex = index;
    const auto& item = BabyGirl::kInstrumentCatalog[index];
    instrumentName = juce::String(item.brand) + " • " + juce::String(item.model);
    if (auto* p = apvts.getParameter("INST_ENGINE"))
    {
        int engine = 0;
        switch (item.family) {
            case BabyGirl::InstrumentFamily::Bass: engine = 3; break;
            case BabyGirl::InstrumentFamily::Drum: engine = 4; break;
            case BabyGirl::InstrumentFamily::PianoKeys: engine = 6; break;
            case BabyGirl::InstrumentFamily::Organ: engine = 5; break;
            case BabyGirl::InstrumentFamily::Strings: engine = 7; break;
            case BabyGirl::InstrumentFamily::Brass: engine = 8; break;
            case BabyGirl::InstrumentFamily::Woodwind: engine = 8; break;
            case BabyGirl::InstrumentFamily::Sampler: engine = 9; break;
            case BabyGirl::InstrumentFamily::Texture: engine = 10; break;
            default: engine = 0; break;
        }
        p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1((float)engine));
    }
    if (auto* p = apvts.getParameter("INST_MODEL"))
        p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1((float)index));
    if (auto* p = apvts.getParameter("INST_EN")) p->setValueNotifyingHost(1.0f);
}

// ---------------------------------------------------------------------------
// SAVE PLUGIN STATE
// ---------------------------------------------------------------------------
// DAWs call this when saving a project/preset. JUCE serializes our parameter
// tree into XML so the next session can restore it.
// ---------------------------------------------------------------------------
void BabyGirlAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    auto xml = state.createXml();
    auto instrument = std::make_unique<juce::XmlElement>("INSTRUMENT");
    instrument->setAttribute("index", instrumentIndex);
    instrument->setAttribute("name", instrumentName);
    xml->addChildElement(instrument.release());
    auto models = std::make_unique<juce::XmlElement>("FXMODELS");
    for (int i = 0; i < BabyGirl::FxRackDsp::kMaxSlots; ++i)
        models->setAttribute("slot" + juce::String(i), slotModels[(size_t)i]);
    xml->addChildElement(models.release());
    copyXmlToBinary(*xml, destData);
}

// ---------------------------------------------------------------------------
// RESTORE PLUGIN STATE
// ---------------------------------------------------------------------------
// This is the opposite of getStateInformation().
// ---------------------------------------------------------------------------
void BabyGirlAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr || !xml->hasTagName(apvts.state.getType())) return;

    if (auto* instrument = xml->getChildByName("INSTRUMENT"))
    {
        instrumentIndex = instrument->getIntAttribute("index", 0);
        instrumentName = instrument->getStringAttribute("name", "Analog Poly");
    }
    if (auto* models = xml->getChildByName("FXMODELS"))
        for (int i = 0; i < BabyGirl::FxRackDsp::kMaxSlots; ++i)
            slotModels[(size_t)i] = models->getStringAttribute("slot" + juce::String(i), "Empty FX Slot");

    apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BabyGirlAudioProcessor();
}
