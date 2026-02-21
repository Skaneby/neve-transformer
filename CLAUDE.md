# Neve Transformer

JUCE 8.0.12 standalone audio app emulating Neve transformer saturation.
macOS 12.0+ universal binary (arm64 + x86_64). Company: HERRSTROM.

## Build
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -j8
```

## Output Rule
All exported files go to: `/Users/macminim1/Library/CloudStorage/GoogleDrive-johan.skaneby@gmail.com/My Drive/Egna projekt/herrstrom`
Filename format: `{original_name}_{YYYY-MM-DD}_{HH-MM-SS}.wav` (or `.aiff`)

## Audio Formats
WAV and AIFF only. No MP3/FLAC — professional audio mixing context.

## Key Architecture
- Real-time file loop playback: load file, play through DSP, adjust knobs live, export when satisfied
- DSP filter updates are thread-safe via atomic dirty flag (recalculated on audio thread only)
- Wet/dry mix and A/B comparison built in
- Pre-allocated buffers (8192 minimum) — no allocations on audio thread
