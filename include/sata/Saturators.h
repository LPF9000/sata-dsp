/*
  ==============================================================================

    Saturators.h
    Rational waveshaping functions for analog-style saturation.

    Part of sata-dsp — header-only DSP building blocks for JUCE audio plugins.

  ==============================================================================
*/

#pragma once

namespace AnalogDSP
{
    // Rational saturator (odd-symmetric, soft)
    inline float saturate(float x)
    {
        return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
    }

    // Asymmetric variant (simulates diode asymmetry — ~10% harder negative)
    // Generates even harmonics (2nd, 4th) for warmth
    inline float saturateAsymmetric(float x)
    {
        if (x >= 0.0f)
            return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
        else
            return x * (24.0f + x * x) / (24.0f + 9.0f * x * x);
    }
}
