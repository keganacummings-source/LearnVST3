// ============================================================================
// FX RACK DSP
// ============================================================================
// This file contains the actual audio effects.
//
// The rack owns 16 slots. Each slot has:
//   - a family (drive, delay, reverb, etc.)
//   - a catalog model number
//   - simple macro controls
//   - a generated model seed
//
// The catalog name does not copy a manufacturer's DSP. Instead, the name
// selects a deterministic personality seed and the family selects the DSP
// family. This keeps the project original while making hundreds of named
// machines feel different.
//
// IMPORTANT FOR STUDYING DSP:
// process() loops through the enabled slots. processOne() performs the
// actual effect for one sample. Start with ONE case in processOne() and learn
// it before reading the rest.
// ============================================================================

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <algorithm>
#include "Identity.h"

namespace BabyGirl
{
class FxRackDsp
{
public:
    static constexpr int kMaxSlots = 16;

    struct Slot
    {
        int family = 0;
        int modelIndex = 0;
        uint32_t modelSeed = 0;
        bool enabled = false;
        float mix = 1.0f;
        float drive = 0.35f;
        float tone = 0.55f;
        float time = 0.35f;
        float feedback = 0.25f;
        float width = 0.5f;
    };

    // Called before audio starts. Allocate our delay memory here, NOT inside
    // process(). Allocating during real-time audio can cause glitches.
    void prepare(double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        maxDelaySamples = std::max(1, (int)std::ceil(sr * 4.0));
        for (auto& b : delayBuffers)
        {
            b.setSize(2, maxDelaySamples + maxBlockSize + 8);
            b.clear();
        }
        for (auto& s : slots) s = {};
        reset();
    }

    // Clear all remembered DSP state. This is useful when playback restarts.
    void reset()
    {
        for (auto& b : delayBuffers) b.clear();
        delayPos.fill(0);
        env.fill(0.0f);
        stateA.fill(0.0f);
        stateB.fill(0.0f);
        lfoPhase.fill(0.0f);
        hold.fill(0.0f);
        rng = 0x12345678u;
    }

    Slot& getSlot(int i) { return slots[(size_t)juce::jlimit(0, kMaxSlots - 1, i)]; }
    const Slot& getSlot(int i) const { return slots[(size_t)juce::jlimit(0, kMaxSlots - 1, i)]; }

    void setSlotFamily(int i, int family)
    {
        getSlot(i).family = juce::jlimit(0, 10, family);
    }

    void setSlotModel(int i, int modelIndex, uint32_t seed)
    {
        auto& s = getSlot(i);
        s.modelIndex = std::max(0, modelIndex);
        s.modelSeed = seed;
    }

    // Process one audio block through the enabled slots, in slot order.
    // Think of this as a row of stompboxes connected with patch cables.
    void process(juce::AudioBuffer<float>& buffer)
    {
        const int channels = std::min(2, buffer.getNumChannels());
        const int n = buffer.getNumSamples();
        if (channels == 0 || n <= 0 || !std::isfinite(sr) || sr <= 1000.0) return;

        bool anyEnabled = false;
        for (const auto& s : slots) anyEnabled = anyEnabled || s.enabled;
        if (!anyEnabled) return;

        for (int slotIndex = 0; slotIndex < kMaxSlots; ++slotIndex)
        {
            auto& s = slots[(size_t)slotIndex];
            if (!s.enabled) continue;

            for (int i = 0; i < n; ++i)
            {
                float dryL = buffer.getSample(0, i);
                float dryR = channels > 1 ? buffer.getSample(1, i) : dryL;
                float l = dryL, r = dryR;

                processOne(slotIndex, s, l, r);

                const float mix = juce::jlimit(0.0f, 1.0f, s.mix);
                buffer.setSample(0, i, dryL * (1.0f - mix) + l * mix);
                if (channels > 1)
                    buffer.setSample(1, i, dryR * (1.0f - mix) + r * mix);
            }
        }
    }

private:
    // Process ONE SAMPLE through ONE SLOT.
    // l and r are references, so changing them changes the caller's audio.
    void processOne(int idx, const Slot& s, float& l, float& r)
    {
        const float mono = 0.5f * (l + r);
        const float d = juce::jlimit(0.0f, 1.0f, s.drive);
        const float tone = juce::jlimit(0.0f, 1.0f, s.tone);
        const auto seed = s.modelSeed == 0 ? uint32_t(0x9e3779b9u + uint32_t(s.modelIndex) * 2654435761u) : s.modelSeed;
        const float c1 = float((seed >> 3) & 0xffu) / 255.0f;
        const float c2 = float((seed >> 11) & 0xffu) / 255.0f;
        const float c3 = float((seed >> 19) & 0xffu) / 255.0f;
        const float c4 = float((seed >> 27) & 0x1fu) / 31.0f;

        // The family chooses the broad DSP recipe. The model seed changes the
        // personality inside that recipe, so catalog entries can remain data.
        switch (s.family)
        {
            case 0: // Drive / fuzz: model-specific transfer personality.
            {
                const float gain = 1.0f + d * (8.0f + c1 * 26.0f);
                const float asym = (c2 - 0.5f) * 0.55f;
                auto shape = [gain, asym, c3](float x)
                {
                    x *= gain;
                    const float asymX = x + asym * x * x;
                    switch (int(c3 * 4.0f))
                    {
                        case 0: return std::tanh(asymX);
                        case 1: return (2.0f / juce::MathConstants<float>::pi) * std::atan(asymX * 1.8f);
                        case 2: return juce::jlimit(-1.0f, 1.0f, asymX);
                        default: return asymX / (1.0f + std::abs(asymX));
                    }
                };
                l = shape(l); r = shape(r * (0.97f + tone * 0.08f));
                const float sag = 1.0f / (1.0f + d * (0.25f + c4 * 1.1f));
                l *= sag; r *= sag;
                break;
            }
            case 1: // Dynamics: varied detector, knee and ratio.
            {
                const float level = std::max(std::abs(mono), 1.0e-5f);
                const float attack = 0.02f + d * (0.04f + c1 * 0.18f);
                const float release = 0.001f + (1.0f - d) * (0.012f + c2 * 0.045f);
                env[(size_t)idx] += (level > env[(size_t)idx] ? attack : release) * (level - env[(size_t)idx]);
                const float threshold = 0.08f + (1.0f - tone) * (0.12f + c3 * 0.55f);
                const float ratio = 1.5f + d * (2.0f + c4 * 16.0f);
                const float over = std::max(0.0f, env[(size_t)idx] - threshold);
                const float gain = 1.0f / (1.0f + over * (ratio - 1.0f));
                l *= gain; r *= gain;
                break;
            }
            case 2: // EQ / tone: resonant low-pass + model-dependent tilt.
            {
                const float cutoff = 70.0f + tone * (3500.0f + c1 * 15000.0f);
                const float a = std::exp(-2.0f * float(juce::MathConstants<double>::pi) * cutoff / float(sr));
                stateA[(size_t)idx] = a * stateA[(size_t)idx] + (1.0f - a) * mono;
                const float low = stateA[(size_t)idx];
                const float high = mono - low;
                const float tilt = (tone - 0.5f) * (0.8f + c2 * 2.4f);
                const float presence = high * (0.15f + c3 * 0.85f) * tilt;
                l += low * tilt + presence; r += low * tilt + presence;
                break;
            }
            case 3: // Delay / echo: distinct time, modulation and feedback personalities.
            {
                auto& db = delayBuffers[(size_t)idx];
                const int length = db.getNumSamples();
                const float base = 0.012f + c1 * 0.11f;
                const float span = 0.45f + c2 * 2.9f;
                const int delay = juce::jlimit(1, length - 1, (int)((base + s.time * span) * sr));
                const int pos = delayPos[(size_t)idx];
                const float mod = std::sin(lfoPhase[(size_t)idx] * float(juce::MathConstants<double>::twoPi)) * (c3 * 0.008f * sr);
                const int readOffset = juce::jlimit(1, length - 1, delay + (int)mod);
                const int read = (pos - readOffset + length) % length;
                const float oldL = db.getSample(0, read);
                const float oldR = db.getSample(1, read);
                const float fb = juce::jlimit(0.0f, 0.93f, s.feedback * (0.72f + c4 * 0.3f));
                db.setSample(0, pos, l + oldL * fb); db.setSample(1, pos, r + oldR * fb);
                delayPos[(size_t)idx] = (pos + 1) % length;
                l = l * (1.0f - 0.55f * fb) + oldL;
                r = r * (1.0f - 0.55f * fb) + oldR;
                lfoPhase[(size_t)idx] += float((0.07 + c2 * 1.7) / sr);
                if (lfoPhase[(size_t)idx] >= 1.0f) lfoPhase[(size_t)idx] -= 1.0f;
                break;
            }
            case 4: // Reverb / space: multiple short paths and unique diffusion.
            {
                auto& db = delayBuffers[(size_t)idx];
                const int length = db.getNumSamples();
                const float room = 0.012f + s.time * (0.7f + c1 * 1.9f);
                const int d1 = juce::jlimit(1, length - 1, (int)((0.013f + c2 * 0.031f + room * 0.31f) * sr));
                const int d2 = juce::jlimit(1, length - 1, (int)((0.021f + c3 * 0.041f + room * 0.53f) * sr));
                const int pos = delayPos[(size_t)idx];
                const float a = db.getSample(0, (pos - d1 + length) % length);
                const float b = db.getSample(1, (pos - d2 + length) % length);
                const float diffusion = 0.18f + s.feedback * (0.4f + c4 * 0.45f);
                db.setSample(0, pos, l + a * diffusion);
                db.setSample(1, pos, r + b * diffusion);
                delayPos[(size_t)idx] = (pos + 1) % length;
                l = l * (0.45f - c1 * 0.08f) + a * (0.58f + c2 * 0.28f) + b * 0.12f;
                r = r * (0.45f - c1 * 0.08f) + b * (0.58f + c2 * 0.28f) + a * 0.12f;
                break;
            }
            case 5: // Modulation: chorus, phaser, flanger and tremolo personalities.
            {
                const float rate = 0.08f + s.time * (0.7f + c1 * 8.0f);
                lfoPhase[(size_t)idx] += float(rate / sr);
                if (lfoPhase[(size_t)idx] >= 1.0f) lfoPhase[(size_t)idx] -= 1.0f;
                const float ph = lfoPhase[(size_t)idx] * float(juce::MathConstants<double>::twoPi);
                float mod = 0.0f;
                switch (int(c2 * 4.0f))
                {
                    case 0: mod = std::sin(ph); break;
                    case 1: mod = 2.0f * std::abs(2.0f * (lfoPhase[(size_t)idx] - std::floor(lfoPhase[(size_t)idx] + 0.5f))) - 1.0f; break;
                    case 2: mod = std::sin(ph) * std::sin(ph * (1.0f + c3 * 2.0f)); break;
                    default: mod = std::sin(ph) > 0.0f ? 1.0f : -1.0f; break;
                }
                const float depth = 0.03f + d * (0.07f + c4 * 0.22f);
                const float cross = mod * s.width * depth;
                const float l0 = l, r0 = r;
                l = l0 * (1.0f + mod * depth) + r0 * cross;
                r = r0 * (1.0f - mod * depth) - l0 * cross;
                break;
            }
            case 6: // Filter: low-pass, high-pass or resonant blend.
            {
                const float cutoff = 35.0f + tone * (1200.0f + c1 * 17000.0f);
                const float a = std::exp(-2.0f * float(juce::MathConstants<double>::pi) * cutoff / float(sr));
                stateA[(size_t)idx] = a * stateA[(size_t)idx] + (1.0f - a) * mono;
                const float low = stateA[(size_t)idx];
                const float hp = mono - low;
                const float resonance = s.feedback * (0.1f + c2 * 0.9f);
                const float blend = 0.15f + d * 0.78f;
                const bool high = c3 > 0.52f;
                const float core = high ? hp : low;
                l = l * (1.0f - blend) + (core + (core - mono) * resonance) * blend;
                r = r * (1.0f - blend) + (core + (core - mono) * resonance) * blend;
                break;
            }
            case 7: // Gate / de-ess.
            {
                const float level = std::abs(mono);
                const float threshold = 0.015f + (1.0f - tone) * (0.08f + c1 * 0.3f);
                env[(size_t)idx] = std::max(level, env[(size_t)idx] * (0.985f + c2 * 0.012f));
                const float range = 0.01f + d * (0.15f + c3 * 0.7f);
                if (env[(size_t)idx] < threshold) { l *= range; r *= range; }
                break;
            }
            case 8: // Pitch / doubler: deterministic detune and phase spread.
            {
                const float spread = 0.01f + d * (0.06f + c1 * 0.3f);
                const float mid = mono;
                const float side = 0.5f * (l - r);
                const float detune = std::sin((float)s.modelIndex * 0.17f) * spread;
                l = mid + side * (1.0f + detune);
                r = mid - side * (1.0f - detune);
                l += mono * (c2 - 0.5f) * d * 0.25f;
                r -= mono * (c3 - 0.5f) * d * 0.25f;
                break;
            }
            case 9: // Looper / freeze.
            {
                const float decay = 0.988f + c1 * 0.011f - d * 0.018f;
                hold[(size_t)idx] = hold[(size_t)idx] * decay + mono * (1.0f - decay);
                const float blend = 0.1f + s.feedback * (0.45f + c2 * 0.5f);
                const float drift = std::sin(lfoPhase[(size_t)idx] * float(juce::MathConstants<double>::twoPi)) * c3 * 0.08f;
                l = l * (1.0f - blend) + hold[(size_t)idx] * (blend + drift);
                r = r * (1.0f - blend) + hold[(size_t)idx] * (blend - drift);
                lfoPhase[(size_t)idx] += float((0.03 + c4 * 0.7) / sr);
                if (lfoPhase[(size_t)idx] >= 1.0f) lfoPhase[(size_t)idx] -= 1.0f;
                break;
            }
            default: // Creative / special: bit reduction, ring movement and stereo chaos.
            {
                const float crush = std::max(2.0f, 2.0f + d * (8.0f + c1 * 48.0f));
                l = std::round(l * crush) / crush;
                r = std::round(r * crush) / crush;
                lfoPhase[(size_t)idx] += float((0.1 + c2 * 5.0) / sr);
                if (lfoPhase[(size_t)idx] >= 1.0f) lfoPhase[(size_t)idx] -= 1.0f;
                const float ring = std::sin(lfoPhase[(size_t)idx] * float(juce::MathConstants<double>::twoPi) * (1.0f + c3 * 7.0f));
                const float chaos = 0.65f + ring * (0.08f + tone * 0.35f);
                const float cross = (c4 - 0.5f) * s.width * 0.7f;
                const float l0 = l, r0 = r;
                l = l0 * chaos + r0 * cross;
                r = r0 * chaos - l0 * cross;
                break;
            }
        }

        if (!std::isfinite(l)) l = 0.0f;
        if (!std::isfinite(r)) r = 0.0f;
        const float safety = 0.65f + d * 0.25f;
        l = std::tanh(l * safety);
        r = std::tanh(r * safety);
    }

    double sr = 48000.0;
    int maxDelaySamples = 192000;
    std::array<Slot, kMaxSlots> slots {};
    std::array<juce::AudioBuffer<float>, kMaxSlots> delayBuffers;
    std::array<int, kMaxSlots> delayPos {};
    std::array<float, kMaxSlots> env {};
    std::array<float, kMaxSlots> stateA {};
    std::array<float, kMaxSlots> stateB {};
    std::array<float, kMaxSlots> lfoPhase {};
    std::array<float, kMaxSlots> hold {};
    uint32_t rng = 0x12345678u;
};
}
