// ============================================================================
// INSTRUMENT CATALOG
// ============================================================================
// This is also mostly data. Every line names an instrument idea and assigns
// it to an InstrumentFamily. The processor chooses a simple synthesis engine
// from that family.
//
// Beginner exercise:
//   Copy one line. Change only the brand/model text. Rebuild.
// ============================================================================

#pragma once
#include <juce_core/juce_core.h>

namespace BabyGirl
{
    enum class InstrumentFamily { Synth, Drum, Bass, PianoKeys, Organ, Guitar, Strings, Brass, Woodwind, Sampler, Texture };
    struct InstrumentCatalogItem { const char* brand; const char* model; InstrumentFamily family; };

    // Reference catalog only. The plugin uses original DSP and original UI/assets.
    inline constexpr InstrumentCatalogItem kInstrumentCatalog[] = {
        {"Roland", "Juno-style Poly", InstrumentFamily::Synth},
        {"Roland", "Jupiter-style Poly", InstrumentFamily::Synth},
        {"Roland", "SH-style Mono", InstrumentFamily::Synth},
        {"Roland", "TB-style Bass", InstrumentFamily::Bass},
        {"Korg", "MS-style Semi-Modular", InstrumentFamily::Synth},
        {"Korg", "Poly-style Analog", InstrumentFamily::Synth},
        {"Korg", "Wavetable-style Keys", InstrumentFamily::Synth},
        {"Korg", "Mini-style Mono", InstrumentFamily::Synth},
        {"Moog", "Ladder Mono", InstrumentFamily::Synth},
        {"Moog", "Classic Poly", InstrumentFamily::Synth},
        {"Sequential", "Prophet-style Poly", InstrumentFamily::Synth},
        {"Sequential", "Mono Lead", InstrumentFamily::Synth},
        {"Oberheim", "SEM-style Voice", InstrumentFamily::Synth},
        {"Oberheim", "OB-style Poly", InstrumentFamily::Synth},
        {"Yamaha", "FM-style 6 Operator", InstrumentFamily::Synth},
        {"Yamaha", "DX-style EP", InstrumentFamily::PianoKeys},
        {"Yamaha", "CS-style Analog", InstrumentFamily::Synth},
        {"Casio", "Phase Distortion Keys", InstrumentFamily::PianoKeys},
        {"Arturia", "Analog Lab-style Poly", InstrumentFamily::Synth},
        {"Novation", "Superwave-style", InstrumentFamily::Synth},
        {"Behringer", "Classic Mono", InstrumentFamily::Synth},
        {"Behringer", "Classic Poly", InstrumentFamily::Synth},
        {"Elektron", "Analog Drum Voice", InstrumentFamily::Drum},
        {"Elektron", "FM Percussion", InstrumentFamily::Drum},
        {"Akai", "MPC-style Drums", InstrumentFamily::Drum},
        {"Akai", "Sampler-style Keys", InstrumentFamily::Sampler},
        {"Roland", "808-style Drum Machine", InstrumentFamily::Drum},
        {"Roland", "909-style Drum Machine", InstrumentFamily::Drum},
        {"Roland", "606-style Drum Machine", InstrumentFamily::Drum},
        {"Roland", "707-style Drum Machine", InstrumentFamily::Drum},
        {"Roland", "CR-style Rhythm Box", InstrumentFamily::Drum},
        {"Korg", "Volca-style Drums", InstrumentFamily::Drum},
        {"Korg", "Wavestation-style Texture", InstrumentFamily::Texture},
        {"Korg", "M1-style Keys", InstrumentFamily::PianoKeys},
        {"Yamaha", "CP-style Electric Piano", InstrumentFamily::PianoKeys},
        {"Fender", "Rhodes-style EP", InstrumentFamily::PianoKeys},
        {"Wurlitzer", "200-style EP", InstrumentFamily::PianoKeys},
        {"Hammond", "Tonewheel Organ", InstrumentFamily::Organ},
        {"Vox", "Continental Organ", InstrumentFamily::Organ},
        {"Farfisa", "Combo Organ", InstrumentFamily::Organ},
        {"Nord", "Stage Piano", InstrumentFamily::PianoKeys},
        {"Nord", "Lead Synth", InstrumentFamily::Synth},
        {"Clavia", "Drum Synth", InstrumentFamily::Drum},
        {"Dave Smith", "Analog Poly", InstrumentFamily::Synth},
        {"Sequential", "Prophet-style Bass", InstrumentFamily::Bass},
        {"Moog", "Sub Bass", InstrumentFamily::Bass},
        {"Roland", "JX-style Strings", InstrumentFamily::Strings},
        {"Solina", "String Ensemble", InstrumentFamily::Strings},
        {"Mellotron", "Tape Chamber", InstrumentFamily::Strings},
        {"Hohner", "Clavinet-style", InstrumentFamily::PianoKeys},
        {"Martin", "Steel Acoustic", InstrumentFamily::Guitar},
        {"Fender", "Vintage Electric", InstrumentFamily::Guitar},
        {"Gibson", "Humbucker Electric", InstrumentFamily::Guitar},
        {"Rickenbacker", "Jangle Electric", InstrumentFamily::Guitar},
        {"Music Man", "Modern Bass", InstrumentFamily::Bass},
        {"Fender", "Precision Bass", InstrumentFamily::Bass},
        {"Fender", "Jazz Bass", InstrumentFamily::Bass},
        {"Gibson", "Hollowbody", InstrumentFamily::Guitar},
        {"Ibanez", "Modern Guitar", InstrumentFamily::Guitar},
        {"Gretsch", "Hollowbody", InstrumentFamily::Guitar},
        {"Taylor", "Steel Acoustic", InstrumentFamily::Guitar},
        {"Rickenbacker", "Bass", InstrumentFamily::Bass},
        {"Orchestral", "Violin", InstrumentFamily::Strings},
        {"Orchestral", "Viola", InstrumentFamily::Strings},
        {"Orchestral", "Cello", InstrumentFamily::Strings},
        {"Orchestral", "Double Bass", InstrumentFamily::Strings},
        {"Orchestral", "String Ensemble", InstrumentFamily::Strings},
        {"Orchestral", "Brass Section", InstrumentFamily::Brass},
        {"Orchestral", "Trumpet", InstrumentFamily::Brass},
        {"Orchestral", "Trombone", InstrumentFamily::Brass},
        {"Orchestral", "French Horn", InstrumentFamily::Brass},
        {"Orchestral", "Tuba", InstrumentFamily::Brass},
        {"Orchestral", "Flute", InstrumentFamily::Woodwind},
        {"Orchestral", "Clarinet", InstrumentFamily::Woodwind},
        {"Orchestral", "Oboe", InstrumentFamily::Woodwind},
        {"Orchestral", "Bassoon", InstrumentFamily::Woodwind},
        {"Orchestral", "Saxophone", InstrumentFamily::Woodwind},
        {"Orchestral", "Woodwind Section", InstrumentFamily::Woodwind},
        {"Percussion", "Acoustic Drum Kit", InstrumentFamily::Drum},
        {"Percussion", "Electronic Kit", InstrumentFamily::Drum},
        {"Percussion", "808-style Kit", InstrumentFamily::Drum},
        {"Percussion", "909-style Kit", InstrumentFamily::Drum},
        {"Percussion", "909-style Hats", InstrumentFamily::Drum},
        {"Percussion", "Clap Machine", InstrumentFamily::Drum},
        {"Percussion", "Conga", InstrumentFamily::Drum},
        {"Percussion", "Bongo", InstrumentFamily::Drum},
        {"Percussion", "Timpani", InstrumentFamily::Drum},
        {"Percussion", "Marimba", InstrumentFamily::PianoKeys},
        {"Texture", "Granular Cloud", InstrumentFamily::Texture},
        {"Texture", "Tape Loop", InstrumentFamily::Texture},
        {"Texture", "Noise Machine", InstrumentFamily::Texture},
        {"Texture", "Drone Generator", InstrumentFamily::Texture},
        {"Sampler", "One-Shot Sampler", InstrumentFamily::Sampler},
        {"Sampler", "Multi-Sample Keyboard", InstrumentFamily::Sampler},
        {"Sampler", "Drum Pad Sampler", InstrumentFamily::Sampler},
        {"Sampler", "Granular Sampler", InstrumentFamily::Sampler},
    };
    inline constexpr int kInstrumentCatalogSize = (int)(sizeof(kInstrumentCatalog) / sizeof(kInstrumentCatalog[0]));

    inline const char* instrumentFamilyName(InstrumentFamily f)
    {
        switch (f) {
            case InstrumentFamily::Synth: return "SYNTH"; case InstrumentFamily::Drum: return "DRUM";
            case InstrumentFamily::Bass: return "BASS"; case InstrumentFamily::PianoKeys: return "KEYS";
            case InstrumentFamily::Organ: return "ORGAN"; case InstrumentFamily::Guitar: return "GUITAR";
            case InstrumentFamily::Strings: return "STRINGS"; case InstrumentFamily::Brass: return "BRASS";
            case InstrumentFamily::Woodwind: return "WOODWIND"; case InstrumentFamily::Sampler: return "SAMPLER";
            case InstrumentFamily::Texture: return "TEXTURE";
        }
        return "INSTRUMENT";
    }
}
