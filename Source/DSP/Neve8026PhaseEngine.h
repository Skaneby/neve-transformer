#pragma once
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

// Neve 8026 Marinair transformer phase character — three cascaded 2nd-order
// allpass biquad stages modelling specific hardware components:
//   Stage 0 — LO1166 "Sub-vikt":  low-frequency weight, bass density
//   Stage 1 — LO1166 "Punch":     low-mid punch and body
//   Stage 2 — 10468  "Silk":      HF transformer signature
//
// Reference coefficients (from Micke, tuned at 48 kHz):
//   Stage 0: b = {0.9961, -1.9923, 1.0}   a = {-1.9923, 0.9961}
//   Stage 1: b = {0.9895, -1.9791, 1.0}   a = {-1.9791, 0.9895}
//   Stage 2: b = {0.3541, -1.3340, 1.0}   a = {-1.3340, 0.3541}
//
// For a 2nd-order allpass:  b0 = a2,  b1 = a1,  b2 = 1
// The pole frequency and Q are derived from a2 and a1:
//   r  = sqrt(a2)           (pole radius)
//   cos(w0) = -a1 / (2*r)   (pole angle → frequency)
//
// Coefficients are recomputed at prepare() for any sample rate.

class Neve8026PhaseEngine {
public:
    Neve8026PhaseEngine() { reset(); }

    void prepare(double sampleRate) {
        // Reference parameters extracted from 48kHz coefficients
        // For a 2nd-order allpass: a2 = r^2,  a1 = -2r*cos(w0)
        // We solve for r and w0 at 48kHz, then recompute at target rate.
        const double refRate = 48000.0;

        // ---- Stage 0: LO1166 Sub-vikt ----
        {
            const double a2ref = 0.9961, a1ref = -1.9923;
            const double r  = std::sqrt(a2ref);
            const double w0_ref = std::acos(-a1ref / (2.0 * r)); // at 48kHz
            const double freq = w0_ref * refRate / (2.0 * juce::MathConstants<double>::pi);
            setAllpassCoeffs(stages[0], freq, r, sampleRate);
        }

        // ---- Stage 1: LO1166 Punch ----
        {
            const double a2ref = 0.9895, a1ref = -1.9791;
            const double r  = std::sqrt(a2ref);
            const double w0_ref = std::acos(-a1ref / (2.0 * r));
            const double freq = w0_ref * refRate / (2.0 * juce::MathConstants<double>::pi);
            setAllpassCoeffs(stages[1], freq, r, sampleRate);
        }

        // ---- Stage 2: 10468 Silk ----
        {
            const double a2ref = 0.3541, a1ref = -1.3340;
            const double r  = std::sqrt(a2ref);
            // a1ref / (2*r) might be outside [-1,1] — clamp before acos
            double cosArg = juce::jlimit(-1.0, 1.0, -a1ref / (2.0 * r));
            const double w0_ref = std::acos(cosArg);
            const double freq = w0_ref * refRate / (2.0 * juce::MathConstants<double>::pi);
            setAllpassCoeffs(stages[2], freq, r, sampleRate);
        }

        reset();
    }

    void reset() {
        for (auto &s : stages) s.z1 = s.z2 = 0.0;
    }

    // Process one sample through all three allpass stages in series.
    inline double processSample(double input) {
        double x = processStage(input,   stages[0]);
               x = processStage(x,       stages[1]);
               x = processStage(x,       stages[2]);
        return x;
    }

private:
    struct Stage {
        // Coefficients (allpass: b0=a2, b1=a1, b2=1)
        double b0 = 1.0, b1 = 0.0;   // b2 always 1.0
        double a1 = 0.0, a2 = 1.0;
        // State (Direct Form II Transposed)
        double z1 = 0.0, z2 = 0.0;
    };

    Stage stages[3];

    // Re-derive allpass coefficients for a given pole frequency and radius
    // at the target sample rate.
    static void setAllpassCoeffs(Stage &s, double freq, double r, double sampleRate) {
        const double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
        s.a1 = -2.0 * r * std::cos(w0);
        s.a2 = r * r;
        // Allpass: b0 = a2, b1 = a1, b2 = 1
        s.b0 = s.a2;
        s.b1 = s.a1;
    }

    inline double processStage(double in, Stage &s) const {
        // Direct Form II Transposed
        double out = in * s.b0 + s.z1;
        const_cast<double&>(s.z1) = in * s.b1 - out * s.a1 + s.z2;
        const_cast<double&>(s.z2) = in          - out * s.a2;

        // Denormal protection
        if (!(s.z1 > 1e-18 || s.z1 < -1e-18)) const_cast<double&>(s.z1) = 0.0;
        if (!(s.z2 > 1e-18 || s.z2 < -1e-18)) const_cast<double&>(s.z2) = 0.0;

        return out;
    }
};
