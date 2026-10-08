// ============================================================================
// MACHINE IDENTITY
// ============================================================================
// A catalog can contain hundreds of names without needing hundreds of
// separate DSP classes. We solve that with a stable hash.
//
// Same brand + model -> same number every time.
// Different brand/model -> usually a different number.
//
// We turn that number into small values such as character, bite, body and
// motion. The DSP and UI can use those values to give a machine its own
// repeatable personality.
// ============================================================================

#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <cstdint>

namespace BabyGirl
{
    // A small deterministic hash.
    // We use it instead of random numbers so the same machine always feels
    // the same after reopening a project.
    inline uint32_t stableHash(const char* brand, const char* model)
    {
        uint32_t hash = 2166136261u;

        auto addCharacter = [&hash](unsigned char character)
        {
            hash ^= character;
            hash *= 16777619u;
        };

        for (auto character = brand; *character != '\\0'; ++character)
            addCharacter(static_cast<unsigned char>(*character));

        // This separator prevents simple combinations such as AB+C and A+BC
        // from being treated as the same string.
        addCharacter(0xff);

        for (auto character = model; *character != '\\0'; ++character)
            addCharacter(static_cast<unsigned char>(*character));

        return hash;
    }

    // These values are the small personality knobs used by the UI/DSP.
    struct FxIdentity
    {
        uint32_t seed = 0;
        float colorPhase = 0.0f;
        float character = 0.5f;
        float bite = 0.5f;
        float body = 0.5f;
        float motion = 0.5f;
        int archetype = 0;
    };

    // Turn the catalog's name into repeatable personality values.
    inline FxIdentity makeFxIdentity(const char* brand, const char* model)
    {
        const uint32_t hash = stableHash(brand, model);

        auto byteAsUnit = [hash](int shift)
        {
            return static_cast<float>((hash >> shift) & 0xffu) / 255.0f;
        };

        FxIdentity identity;
        identity.seed = hash;
        identity.colorPhase = byteAsUnit(0);
        identity.character = byteAsUnit(8);
        identity.bite = byteAsUnit(16);
        identity.body = byteAsUnit(24);
        identity.motion = static_cast<float>((hash >> 5) & 0x7fu) / 127.0f;
        identity.archetype = static_cast<int>((hash >> 28) % 6u);

        return identity;
    }

    inline juce::Colour identityColour(const FxIdentity& x)
    {
        const float hue = std::fmod(0.035f + x.colorPhase * 0.93f, 1.0f);
        return juce::Colour::fromHSV(hue, 0.62f, 0.96f, 1.0f);
    }

    inline const char* macroLabel(int family, int macro)
    {
        static constexpr const char* labels[11][6] = {
            {"MIX", "DRIVE", "TONE", "PRESENCE", "BITE", "OUTPUT"},
            {"MIX", "THRESH", "ATTACK", "RELEASE", "RATIO", "MAKEUP"},
            {"MIX", "LOW", "MID", "HIGH", "Q", "OUTPUT"},
            {"MIX", "TIME", "TONE", "FEEDBACK", "MOD", "WIDTH"},
            {"MIX", "SIZE", "TONE", "DECAY", "DAMP", "WIDTH"},
            {"MIX", "RATE", "DEPTH", "TONE", "FEEDBACK", "WIDTH"},
            {"MIX", "CUTOFF", "RESONANCE", "DRIVE", "ENV", "WIDTH"},
            {"MIX", "THRESH", "ATTACK", "RELEASE", "RANGE", "HOLD"},
            {"MIX", "PITCH", "DETUNE", "TONE", "GLIDE", "WIDTH"},
            {"MIX", "TIME", "FEEDBACK", "AGE", "PITCH", "WIDTH"},
            {"MIX", "CRUSH", "TONE", "MOTION", "CHAOS", "WIDTH"}
        };
        return labels[juce::jlimit(0, 10, family)][juce::jlimit(0, 5, macro)];
    }
}
