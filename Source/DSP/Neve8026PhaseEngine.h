#pragma once
#include <cmath>

// Neve 8026 Marinair transformer phase character.
// Three cascaded 2nd-order allpass biquad stages named after hardware components:
//   Stage 0 — LO1166 Sub/Weight:   low-frequency transformer weight
//   Stage 1 — LO1166 Punch/Body:   low-mid body and punch
//   Stage 2 — 10468  Silk/Air:     HF transformer signature
//
// 7 instrument presets tune the phase character to each source type.
// Coefficients computed at prepare() via AudioEQ Cookbook allpass formula.
// Runs at 4x oversampled rate (192kHz for 48kHz input).
// Double precision throughout. Spec by Micke.

enum class SoundType { Kick = 0, Snare, BassGuitar, ElectricGuitar, Vocals, Piano, MixBus };
static constexpr int kNumSoundTypes = 7;

class Neve8026PhaseEngine {
public:
    Neve8026PhaseEngine() { reset(); }

    void prepare(double sampleRate) {
        setupPresets(sampleRate);
        currentPreset = allPresets[0]; // default: Kick
        reset();
    }

    void reset() {
        for (int i = 0; i < 3; ++i)
            stages[i].z1 = stages[i].z2 = 0.0;
    }

    // Plain struct copy — safe to call from audio thread, no allocation.
    void setSound(SoundType type) {
        int idx = static_cast<int>(type);
        if (idx >= 0 && idx < kNumSoundTypes)
            currentPreset = allPresets[idx];
    }

    inline double processSample(double input) {
        double x = processStage(input, currentPreset.c[0], stages[0]);
               x = processStage(x,     currentPreset.c[1], stages[1]);
               x = processStage(x,     currentPreset.c[2], stages[2]);
        return x;
    }

private:
    // 5-coefficient allpass biquad (Direct Form II Transposed)
    struct Coeffs { double b0, b1, b2, a1, a2; };

    // State (z1, z2) kept separate from coefficients —
    // coefficients live in Preset, state lives per-channel in stages[].
    struct State { double z1 = 0.0, z2 = 0.0; };

    struct Preset { Coeffs c[3]; }; // c[0]=Sub, c[1]=Punch, c[2]=Silk

    State  stages[3];
    Preset currentPreset;
    Preset allPresets[kNumSoundTypes];

    // AudioEQ Cookbook 2nd-order allpass.
    // Allpass identity: b0=a2, b1=a1, b2=1 (normalized by a0).
    static Coeffs makeAllpass(double freq, double Q, double sr) {
        constexpr double kPi = 3.141592653589793;
        const double omega = 2.0 * kPi * freq / sr;
        const double alpha = std::sin(omega) / (2.0 * Q);
        const double cosw  = std::cos(omega);
        const double a0    = 1.0 + alpha;
        return {
            (1.0 - alpha) / a0,   // b0 = a2
            (-2.0 * cosw) / a0,   // b1 = a1
            (1.0 + alpha) / a0,   // b2 = 1.0 (== 1.0 normalized)
            (-2.0 * cosw) / a0,   // a1
            (1.0 - alpha) / a0    // a2
        };
    }

    void setupPresets(double sr) {
        // Each row: { f_sub, Q_sub, f_punch, Q_punch, f_silk, Q_silk }
        struct P3 { double f1, q1, f2, q2, f3, q3; };
        const P3 defs[kNumSoundTypes] = {
            {   35, 0.6,    80, 0.5, 18000, 0.4 }, // Kick
            {  150, 0.5,   400, 0.4, 15000, 0.5 }, // Snare
            {   60, 0.6,   120, 0.5, 12000, 0.4 }, // Bass Guitar
            {  300, 0.4,   800, 0.3, 10000, 0.6 }, // Electric Guitar
            {  100, 0.3,   200, 0.3, 20000, 0.5 }, // Vocals
            {   80, 0.4,   500, 0.3, 16000, 0.4 }, // Piano
            {   40, 0.5,   100, 0.4, 19000, 0.4 }, // Mix Bus
        };

        const double nyq = sr * 0.48;
        for (int i = 0; i < kNumSoundTypes; ++i) {
            allPresets[i].c[0] = makeAllpass(std::fmin(defs[i].f1, nyq), defs[i].q1, sr);
            allPresets[i].c[1] = makeAllpass(std::fmin(defs[i].f2, nyq), defs[i].q2, sr);
            allPresets[i].c[2] = makeAllpass(std::fmin(defs[i].f3, nyq), defs[i].q3, sr);
        }
    }

    inline double processStage(double in, const Coeffs &c, State &s) {
        double out = in * c.b0 + s.z1;
        s.z1 = in * c.b1 - out * c.a1 + s.z2;
        s.z2 = in * c.b2 - out * c.a2;
        // Denormal protection
        if (!(s.z1 > 1e-18 || s.z1 < -1e-18)) s.z1 = 0.0;
        if (!(s.z2 > 1e-18 || s.z2 < -1e-18)) s.z2 = 0.0;
        return out;
    }
};
