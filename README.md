# sata-dsp

Header-only DSP building blocks for JUCE audio plugins.

## Components

| Header | Description |
|--------|-------------|
| `Saturators.h` | Rational waveshaping functions (symmetric + asymmetric) |
| `OnePoleFilter.h` | One-pole lowpass for envelopes, smoothing, and filtering |
| `PinkNoiseGenerator.h` | Voss-McCartney pink noise (3 octave bands) |
| `AnalogSVF.h` | Nonlinear state variable filter with saturated feedback |

## Usage

Add as a git submodule and include the headers you need:

```cpp
#include "sata-dsp/include/sata/Saturators.h"
#include "sata-dsp/include/sata/OnePoleFilter.h"
```

All classes are header-only for per-sample inlining.
