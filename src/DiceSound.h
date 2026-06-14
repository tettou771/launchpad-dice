#pragma once

// =============================================================================
// DiceSound - roulette blips + one melody per result
// =============================================================================
// Everything is pre-built once in setup() so triggering during the roll only
// calls play() (no per-hit ChipSound build). Blips are a chromatic ladder of
// short square pings; each result 1..6 has its own little tune, from a sad
// low fall (1) to a bright winning fanfare (6).
// =============================================================================

#include <TrussC.h>

#include <array>
#include <cmath>

using namespace tc;

class DiceSound {
public:
    static constexpr int kBlips = 16;

    void setup() {
        // Roulette ticks: short square pings, A4 upward by semitones.
        for (int i = 0; i < kBlips; ++i) {
            float hz = 440.0f * std::pow(2.0f, i / 12.0f);
            ChipSoundNote n{Wave::Square, hz, 0.07f, 0.22f};
            n.attack = 0.002f; n.decay = 0.03f; n.sustain = 0.2f; n.release = 0.03f;
            blips_[i] = n.build();
        }
        for (int d = 1; d <= 6; ++d) melody_[d] = buildMelody(d);
    }

    // Play roulette tick i (clamped into range).
    void blip(int i) {
        if (i < 0) i = 0;
        if (i >= kBlips) i = kBlips - 1;
        blips_[i].stop();
        blips_[i].play();
    }

    // Play the result tune for n (1..6).
    void playMelody(int n) {
        if (n < 1 || n > 6) return;
        melody_[n].stop();
        melody_[n].play();
    }

private:
    std::array<Sound, kBlips> blips_;
    std::array<Sound, 7>      melody_;  // index 1..6

    // Add one note (by MIDI number) to a bundle.
    static void add(ChipSoundBundle& b, Wave w, int midi, float t, float dur,
                    float vol, float release = 0.08f) {
        float hz = 440.0f * std::pow(2.0f, (midi - 69) / 12.0f);
        ChipSoundNote n{w, hz, dur, vol};
        n.attack = 0.004f; n.decay = 0.05f; n.sustain = 0.5f; n.release = release;
        b.add(n, t);
    }

    static Sound buildMelody(int n) {
        ChipSoundBundle b;
        switch (n) {
            case 6:  // jackpot: fast rising arpeggio with a sparkly finish
                add(b, Wave::Square,   72, 0.00f, 0.10f, 0.28f);
                add(b, Wave::Square,   76, 0.10f, 0.10f, 0.28f);
                add(b, Wave::Square,   79, 0.20f, 0.10f, 0.28f);
                add(b, Wave::Square,   84, 0.30f, 0.10f, 0.30f);
                add(b, Wave::Triangle, 88, 0.40f, 0.06f, 0.26f);
                add(b, Wave::Square,   84, 0.46f, 0.06f, 0.26f);
                add(b, Wave::Square,   91, 0.54f, 0.30f, 0.30f, 0.18f);
                break;
            case 5:  // nice: confident rising third
                add(b, Wave::Triangle, 71, 0.00f, 0.14f, 0.30f);
                add(b, Wave::Triangle, 74, 0.16f, 0.14f, 0.30f);
                add(b, Wave::Square,   79, 0.32f, 0.26f, 0.30f, 0.14f);
                break;
            case 4:  // ok: a small two-note lift
                add(b, Wave::Square,   67, 0.00f, 0.16f, 0.28f);
                add(b, Wave::Square,   72, 0.18f, 0.26f, 0.30f, 0.14f);
                break;
            case 3:  // plain: two flat notes
                add(b, Wave::Triangle, 69, 0.00f, 0.18f, 0.28f);
                add(b, Wave::Triangle, 69, 0.22f, 0.20f, 0.26f);
                break;
            case 2:  // meh: a half-step sag
                add(b, Wave::Square,   64, 0.00f, 0.18f, 0.26f);
                add(b, Wave::Square,   61, 0.22f, 0.26f, 0.26f, 0.12f);
                break;
            case 1:  // sad: slow low fall, "womp womp"
                add(b, Wave::Sawtooth, 53, 0.00f, 0.30f, 0.30f, 0.10f);
                add(b, Wave::Sawtooth, 48, 0.34f, 0.55f, 0.32f, 0.22f);
                break;
        }
        return b.build();
    }
};
