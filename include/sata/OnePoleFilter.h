/*
  ==============================================================================

    OnePoleFilter.h
    Simple one-pole lowpass for envelope following, DC blocking,
    pre/post saturation filtering, and feedback smoothing.

    Part of sata-dsp — header-only DSP building blocks for JUCE audio plugins.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cmath>

class OnePoleFilter
{
public:
    void setCutoff(float freqHz, float sampleRate)
    {
        a0 = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * freqHz / sampleRate);
    }

    float process(float x)
    {
        z1 += a0 * (x - z1);
        return z1;
    }

    void reset() { z1 = 0.0f; }

private:
    float a0 = 0.1f;
    float z1 = 0.0f;
};
