/*
  ==============================================================================

    PinkNoiseGenerator.h
    Pink noise generator using Paul Kellett filtered white noise
    with octave-rate scheduling (3 bands).

    Part of sata-dsp — header-only DSP building blocks for JUCE audio plugins.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class PinkNoiseGenerator
{
public:
    float nextSample(juce::Random& rng)
    {
        // b0 updated every sample, b1 every 2nd, b2 every 4th
        // Random values centered around 0 [-1, +1) to avoid DC accumulation
        b0 = 0.99886f * b0 + (rng.nextFloat() * 2.0f - 1.0f) * 0.0555179f;

        if ((counter & 1) == 0)
            b1 = 0.99332f * b1 + (rng.nextFloat() * 2.0f - 1.0f) * 0.0750759f;

        if ((counter & 3) == 0)
            b2 = 0.96900f * b2 + (rng.nextFloat() * 2.0f - 1.0f) * 0.1538520f;

        ++counter;

        return (b0 + b1 + b2) * 0.00125f;  // scaled to ~0.00025 peak
    }

    void reset()
    {
        b0 = 0.0f;
        b1 = 0.0f;
        b2 = 0.0f;
        counter = 0;
    }

private:
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    unsigned int counter = 0;
};
