# Neve Transformer - Build & Usage Guide

## Quick Start

### Run the Application

```bash
cd /Users/juneskaneby/.gemini/antigravity/playground/luminescent-apollo
open "build/NeveTransformer_artefacts/Release/Neve Transformer.app"
```

First time: Right-click → "Open" if macOS blocks it.

Or remove quarantine flag:
```bash
xattr -cr "build/NeveTransformer_artefacts/Release/Neve Transformer.app"
```

---

## Rebuilding

```bash
cd /Users/juneskaneby/.gemini/antigravity/playground/luminescent-apollo/build
cmake --build . --config Release -j8
```

Clean rebuild:
```bash
cd /Users/juneskaneby/.gemini/antigravity/playground/luminescent-apollo
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -j8
```

---

## Parameters

- **Drive** (0-1): Saturation amount
- **Iron** (0-1): LF boost (magnetization)
- **HF Roll** (0-1): High-frequency rolloff (20-30 kHz)
- **Mic Mode**: Harder LF pole for mic signals
- **Hi-Z Load**: Sharper HF resonance
- **Bypass**: A/B comparison

---

## Audio Routing

The app uses CoreAudio:
1. **Input**: System default input or audio interface
2. **Output**: System default output
3. **Latency**: Displayed in UI (target <5ms)

Route audio through the app using:
- **macOS Audio MIDI Setup** for system routing
- **Loopback** / **Audio Hijack** for DAW integration
- **Soundflower** / **BlackHole** as virtual audio cables

---

## Testing

### Frequency Response
```bash
# Generate swept sine (Python)
cd Testing
python3 test_signals.py
```

Route through app, capture output, analyze with `frequency_analysis.py`.

### THD Measurement
- Input: 1 kHz @ +6 dBu, Drive=0.5
- Measure with REW or spectrum analyzer
- Target: 0.5-1.5% THD,3rd harmonic dominant

---

## Technical Details

**DSP Chain**:
1. Iron boost (100 Hz shelf)
2. LF pole (60 Hz lowpass)
3. HF resonance (14 kHz peak)
4. 4x oversample to 192 kHz
5. Waveshaper (tanh, 3rd harmonic bias)
6. Dynamic allpass (AM/PM)
7. Downsample to 48 kHz
8. Post-shelf (80 Hz +0.2 dB)

**Latency**: ~2-4 ms @ 48 kHz (4x oversampling + IIR filters)

---

Built with JUCE 8.0.4, macOS 10.13+ compatible
