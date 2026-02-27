#pragma once
#include <cmath>

// Neve 8026 "Bättre" — fixed Marinair transformer phase signature.
// Three cascaded 2nd-order allpass biquad stages:
//   Stage 0 — LO1166 Sub-weight:  38 Hz / Q 0.55
//   Stage 1 — LO1166 Body:        95 Hz / Q 0.45
//   Stage 2 — 10468  Silk/Air: 18500 Hz / Q 0.40
//
// No presets, no controls — pure fixed phase character.
// Coefficients recomputed at prepare() for the actual oversampled rate.
// Double precision throughout.

class Neve8026PhaseEngine {
public:
    Neve8026PhaseEngine() { reset(); }

    void prepare(double sampleRate) {
        coeffs[0] = makeAllpass(38.0,    0.55, sampleRate);
        coeffs[1] = makeAllpass(95.0,    0.45, sampleRate);
        coeffs[2] = makeAllpass(18500.0, 0.40, sampleRate);
        reset();
    }

    void reset() {
        for (int i = 0; i < 3; ++i)
            state[i].z1 = state[i].z2 = 0.0;
    }

    inline double processSample(double input) {
        double x = processStage(input, coeffs[0], state[0]);
               x = processStage(x,     coeffs[1], state[1]);
               x = processStage(x,     coeffs[2], state[2]);
        return x;
    }

private:
    struct Coeffs { double b0, b1, b2, a1, a2; };
    struct State  { double z1 = 0.0, z2 = 0.0; };

    Coeffs coeffs[3];
    State  state[3];

    // AudioEQ Cookbook 2nd-order allpass. Allpass identity: b0=a2, b1=a1, b2=1 (normalized).
    static Coeffs makeAllpass(double freq, double Q, double sr) {
        constexpr double kPi = 3.141592653589793;
        const double omega = 2.0 * kPi * freq / sr;
        const double alpha = std::sin(omega) / (2.0 * Q);
        const double cosw  = std::cos(omega);
        const double a0    = 1.0 + alpha;
        return {
            (1.0 - alpha) / a0,   // b0
            (-2.0 * cosw)  / a0,  // b1
            (1.0 + alpha)  / a0,  // b2
            (-2.0 * cosw)  / a0,  // a1
            (1.0 - alpha)  / a0   // a2
        };
    }

    inline double processStage(double in, const Coeffs& c, State& s) {
        double out = in * c.b0 + s.z1;
        s.z1 = in * c.b1 - out * c.a1 + s.z2;
        s.z2 = in * c.b2 - out * c.a2;
        if (!(s.z1 > 1e-18 || s.z1 < -1e-18)) s.z1 = 0.0;
        if (!(s.z2 > 1e-18 || s.z2 < -1e-18)) s.z2 = 0.0;
        return out;
    }
};
