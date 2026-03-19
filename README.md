<p align="center">
  <strong>sata-dsp</strong><br>
  <em>Header-only DSP building blocks for JUCE audio plugins</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/language-C%2B%2B17-blue?style=flat-square" alt="C++17">
  <img src="https://img.shields.io/badge/framework-JUCE-orange?style=flat-square" alt="JUCE">
  <img src="https://img.shields.io/badge/build-header--only-brightgreen?style=flat-square" alt="Header-only">
  <img src="https://img.shields.io/badge/license-MIT-lightgrey?style=flat-square" alt="MIT License">
</p>

---

**sata-dsp** is a collection of real-time-safe, header-only DSP primitives designed for audio plugin development with [JUCE](https://juce.com). Every class is engineered for per-sample processing with zero allocations, making them safe to call from the audio thread without locks or blocking.

All components are **header-only by design** — per-sample DSP functions must be visible at the call site for the compiler to inline them. This follows the same convention used by JUCE's own `dsp` module.

## Components

| Header | Class | Description |
|--------|-------|-------------|
| [`Saturators.h`](#saturatorsh) | `AnalogDSP` | Rational waveshaping functions — symmetric and asymmetric |
| [`OnePoleFilter.h`](#onepolefilterh) | `OnePoleFilter` | One-pole lowpass for envelopes, smoothing, and spectral splitting |
| [`PinkNoiseGenerator.h`](#pinknoisegeneratorh) | `PinkNoiseGenerator` | Voss-McCartney pink noise generator (3 octave bands) |
| [`AnalogSVF.h`](#analogsvfh) | `AnalogSVF` | Nonlinear state variable filter with saturated feedback |

---

## Saturators.h

Stateless waveshaping functions in the `AnalogDSP` namespace. These use rational polynomial transfer functions — computationally cheap (no transcendentals), smooth at the origin, and free of discontinuities that cause aliasing.

### `AnalogDSP::saturate(x)`

**Symmetric soft saturator.** Odd-symmetric transfer function that generates only odd harmonics (3rd, 5th, 7th...), producing a clean, transparent warmth similar to transformer or tape saturation.

```
Transfer function:   f(x) = x(27 + x²) / (27 + 9x²)
```

```
Output
  1.0 ┤                          ·····························
      │                    ·····
      │                ····
      │             ···
      │           ··
      │         ··
      │        ·
      │       ·
      │      ·
      │     ·
  0.0 ┤····                                            Input
      │     ·
      │      ·
      │       ·
      │        ·
      │         ··
      │           ··
      │             ···
      │                ····
      │                    ·····
 -1.0 ┤                          ·····························
      └──────────────────────────────────────────────────────
     -5.0                       0.0                        5.0
```

**Characteristics:**
- Unity gain at origin — `f(0) = 0`, `f'(0) = 1`
- Soft limiting toward ±1.0 at high input levels
- No even harmonics — preserves spectral symmetry
- Zero-crossing is smooth (no kink like `tanh`)

```cpp
float shaped = AnalogDSP::saturate(input * drive) / drive;
```

### `AnalogDSP::saturateAsymmetric(x)`

**Asymmetric soft saturator.** Uses different polynomial coefficients for positive vs. negative half-cycles, simulating the asymmetric clipping behavior of real diodes and transistors. The negative half clips ~10% harder (coefficient 24 vs. 27).

```
Transfer function:   f(x) = x(27 + x²) / (27 + 9x²)    for x >= 0
                     f(x) = x(24 + x²) / (24 + 9x²)    for x <  0
```

```
Output
  1.0 ┤                          ·····························
      │                    ·····
      │                ····
      │             ···
      │           ··
      │         ··
      │        ·
      │       ·
      │      ·
      │     ·
  0.0 ┤····                                            Input
      │     ·
      │      ·
      │       ·
      │       ·
      │        ··
      │          ··
      │            ···
      │               ·····
      │                    ·······
 -0.9 ┤                          ·····························
      └──────────────────────────────────────────────────────
     -5.0                       0.0                        5.0
```

**Characteristics:**
- Generates **even harmonics** (2nd, 4th) — the primary source of "analog warmth"
- Negative peaks compress slightly more than positive peaks
- Models class-A amplifier / diode clipper asymmetry
- Pairs well with a DC-blocking filter downstream

```cpp
// Drive into saturation, normalize back, then DC-block
float warm = AnalogDSP::saturateAsymmetric(input * 6.0f) / 6.0f;
```

### Saturation Comparison

| Function | Harmonics | Character | Use Case |
|----------|-----------|-----------|----------|
| `saturate` | Odd only (3rd, 5th) | Clean, transparent | Feedback paths, subtle warming |
| `saturateAsymmetric` | Odd + Even (2nd, 3rd, 4th) | Warm, full, analog | Parallel saturation, exciter stages |

---

## OnePoleFilter.h

A minimal one-pole IIR lowpass filter. Despite its simplicity, this is one of the most versatile building blocks in audio DSP — it appears inside envelope followers, smoothing filters, DC blockers, spectral splitters, and feedback networks.

### Signal Flow

```
                 ┌─────────────────────────────┐
                 │         OnePoleFilter        │
                 │                              │
  input ───────►│ z1 += a0 * (input - z1)  ───►│──── output (z1)
                 │                              │
                 │  a0 = 1 - e^(-2πf/sr)       │
                 └─────────────────────────────┘
```

### Frequency Response

```
   0 dB ┤·····
        │      ····
        │          ···
  -3 dB ┤─ ─ ─ ─ ─ ─ ·── ─ ─ ─ ─ ─    ← cutoff frequency
        │              ··
        │                ··
  -6 dB ┤                  ·
        │                   ··
        │                     ···
 -12 dB ┤                        ····
        │                            ·····
        │                                 ·········
        └──────────────────────────────────────────
        20 Hz                              20 kHz

        Slope: -6 dB/octave (-20 dB/decade)
```

### API

```cpp
OnePoleFilter filter;

// Set cutoff frequency (call once in prepare, or when frequency changes)
filter.setCutoff(3000.0f, sampleRate);

// Process one sample (call per-sample in audio callback)
float output = filter.process(input);

// Reset state (call on prepare/reset to clear transients)
filter.reset();
```

### Common Applications

**Envelope Follower**
```cpp
// Track signal amplitude with asymmetric attack/release
OnePoleFilter envFilter;
envFilter.setCutoff(20.0f, sampleRate);   // ~8ms time constant

float envelope = envFilter.process(std::abs(input));
```

**Spectral Splitter (Crossover)**
```cpp
// Split signal into low and high bands at 3kHz
OnePoleFilter crossover;
crossover.setCutoff(3000.0f, sampleRate);

float low  = crossover.process(input);
float high = input - low;   // complementary highpass
```

**Parameter Smoothing**
```cpp
// Smooth a stepped parameter to avoid zipper noise
OnePoleFilter smoother;
smoother.setCutoff(10.0f, sampleRate);   // ~16ms smoothing

float smoothedParam = smoother.process(rawParam);
```

---

## PinkNoiseGenerator.h

A **Voss-McCartney** pink noise generator using 3 octave bands. Produces noise with a **-3 dB/octave** spectral slope, matching the frequency distribution of most natural and musical signals. This makes it ideal for adding analog-style noise floors that sit naturally in a mix.

### Power Spectral Density

```
  Power
 (dB)
   0 ┤·
     │ ··
     │   ··
  -3 ┤─ ─ ··─ ─ ─ ─ ─ ─ ─ ─ ─ ─    ← -3 dB/octave
     │      ··
  -6 ┤        ··
     │          ··
     │            ··
  -9 ┤              ··
     │                ···
     │                   ···
 -12 ┤                      ···
     │                         ····
     │                             ·····
     └──────────────────────────────────
     20 Hz        1 kHz        20 kHz


     White noise: flat spectrum (equal energy per Hz)
     Pink noise:  -3 dB/octave (equal energy per octave)
     Brown noise: -6 dB/octave (random walk)
```

### How It Works

The Voss-McCartney algorithm generates pink noise by summing multiple random generators updated at different rates — each "octave band" updates half as often as the one above it.

```
  Sample:   0  1  2  3  4  5  6  7  8  ...
            ┌──┬──┬──┬──┬──┬──┬──┬──┬──
  b0        ■  ■  ■  ■  ■  ■  ■  ■  ■    ← updated every sample
  b1        ■     ■     ■     ■     ■      ← updated every 2nd sample
  b2        ■           ■           ■      ← updated every 4th sample
            └──┴──┴──┴──┴──┴──┴──┴──┴──
  output  = (b0 + b1 + b2) × scale

  Each band uses exponential decay (0.999×) for smooth transitions.
```

### API

```cpp
PinkNoiseGenerator pink;
juce::Random rng(42);   // seed for reproducibility

// Generate one sample of pink noise
float noise = pink.nextSample(rng);

// Reset state (call on prepare/reset)
pink.reset();
```

### Usage Patterns

**Analog Noise Floor**
```cpp
// Add subtle noise floor gated by signal level
float envelope = envFollower.process(std::abs(input));
float gate = juce::jlimit(0.0f, 1.0f, envelope * 50.0f);

output += pink.nextSample(rng) * noiseLevel * gate;
```

**Stereo Decorrelation**
```cpp
// Use different seeds per channel for natural stereo image
juce::Random rngL(42), rngR(71);
PinkNoiseGenerator pinkL, pinkR;

float noiseL = pinkL.nextSample(rngL);
float noiseR = pinkR.nextSample(rngR);
```

---

## AnalogSVF.h

A custom **State Variable Filter** based on the Topology-Preserving Transform (TPT) structure — the same zero-delay feedback topology used by JUCE's `StateVariableTPTFilter` — but with **nonlinear saturation injected into the feedback path** for analog character.

### Architecture

The filter mirrors a real analog SVF circuit where op-amp saturation, capacitor nonlinearity, and component tolerances create the characteristic warmth and compression of hardware filters.

```
                          AnalogSVF Signal Flow
  ┌─────────────────────────────────────────────────────────────┐
  │                                                             │
  │  input ──►(+)──────────────────────► yHP (highpass out)     │
  │            │                          │                     │
  │            │    ┌──────────────────┐  │                     │
  │            │◄───│ saturate(s1) ×   │◄─┘                     │
  │            │    │ feedbackSmoother │   ×g                   │
  │            │    └──────────────────┘  │                     │
  │            │                         ▼                      │
  │            │    ┌──────────────────┐                        │
  │            │    │ preSatFilter     │                        │
  │            │    │ saturate(yBP)    │──► yBP (bandpass)      │
  │            │    │ postSatFilter    │   │                    │
  │            │    └──────────────────┘   │                    │
  │            │                          │  ×g                │
  │            │    ┌──────────────────┐  │                     │
  │            │◄───│ saturate(s2)     │◄─┘                     │
  │            │    └──────────────────┘  │                     │
  │            │                         ▼                      │
  │            │                        yLP (lowpass out)       │
  │                                                             │
  │  Nonlinear elements:                                        │
  │   • State variable saturation (s1, s2) — asymmetric         │
  │   • Bandpass saturation — symmetric                         │
  │   • Feedback smoothing — 12kHz one-pole                     │
  │   • Level-dependent resonance modulation                    │
  └─────────────────────────────────────────────────────────────┘
```

### Nonlinear Features

| Feature | Description | Effect |
|---------|-------------|--------|
| **State saturation** | `saturateAsymmetric()` on s1 and s2 before feedback | Soft compression at resonance peaks, even harmonics |
| **Feedback smoothing** | 12 kHz one-pole on saturated s1 | Prevents harsh aliasing from saturated feedback |
| **Bandpass saturation** | Pre-filter → `saturate()` → post-filter on yBP | Warm mid-range coloration, controlled by 8–10 kHz band |
| **Level-dependent Q** | Envelope follower modulates R2 | Resonance subtly increases with louder input — like a real VCF |

### API

```cpp
AnalogSVF filter;

// Initialize (call in prepareToPlay)
filter.prepare(sampleRate);
filter.setType(AnalogSVF::Type::lowpass);    // or ::highpass
filter.setCoefficients(1000.0f, 0.707f);     // frequency, resonance

// Process (call per-sample in audio callback)
float output = filter.processSample(input);

// Reset state (call on prepare/reset)
filter.reset();
```

### Comparison with JUCE StateVariableTPTFilter

| | `juce::dsp::StateVariableTPTFilter` | `AnalogSVF` |
|---|---|---|
| Topology | TPT (zero-delay feedback) | TPT (zero-delay feedback) |
| Saturation | None (linear) | Asymmetric state + symmetric bandpass |
| Resonance | Fixed Q | Level-dependent Q modulation |
| Character | Clean, transparent | Warm, compressed, analog |
| Use case | Precision filtering | Analog modeling, character |

---

## Integration

### As a Git Submodule

```bash
# Add to your project
cd your-project/Source
git submodule add git@github.com:LPF9000/sata-dsp.git

# Include in your code
#include "sata-dsp/include/sata/Saturators.h"
#include "sata-dsp/include/sata/OnePoleFilter.h"
#include "sata-dsp/include/sata/PinkNoiseGenerator.h"
#include "sata-dsp/include/sata/AnalogSVF.h"
```

### Cloning a Project That Uses sata-dsp

```bash
# Clone with submodules
git clone --recurse-submodules git@github.com:your-org/your-project.git

# Or, if already cloned without submodules
git submodule update --init --recursive
```

### Dependencies

- **JUCE** — uses `juce::MathConstants`, `juce::jlimit`, and `juce::Random`
- **C++17** or later
- No additional libraries required

---

## Design Principles

- **Zero allocation** — no `new`, `malloc`, or STL containers in the audio path
- **Header-only** — all functions visible for inlining; no link-time dependencies
- **Per-sample processing** — designed to be called inside sample loops, not block processors
- **Deterministic** — no branching on uninitialized state; safe to call immediately after `prepare()`
- **Real-time safe** — no locks, no syscalls, no exceptions in the processing path

---

## License

MIT License. See [LICENSE](LICENSE) for details.

---

<p align="center">
  <strong>SATA</strong><br>
  <em>Built for sound.</em>
</p>
