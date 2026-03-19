/*
  ==============================================================================

    AnalogSVF.h
    Custom SVF with nonlinear feedback — mirrors JUCE StateVariableTPTFilter
    topology but injects saturation at specific points. Mono (one instance
    per channel).

    Part of sata-dsp — header-only DSP building blocks for JUCE audio plugins.

  ==============================================================================
*/

#pragma once

#include "OnePoleFilter.h"
#include "Saturators.h"

class AnalogSVF
{
public:
    enum class Type { lowpass, highpass };

    void prepare(float newSampleRate)
    {
        sampleRate = newSampleRate;
        feedbackSmoother.setCutoff(12000.0f, sampleRate);
        preSatFilter.setCutoff(8000.0f, sampleRate);
        postSatFilter.setCutoff(10000.0f, sampleRate);
    }

    void setCoefficients(float freqHz, float res)
    {
        cutoffFrequency = juce::jlimit(20.0f, sampleRate * 0.49f, freqHz);
        resonance = res;

        g = std::tan(juce::MathConstants<float>::pi * cutoffFrequency / sampleRate);
        R2 = 1.0f / resonance;
    }

    void setType(Type newType) { type = newType; }

    float processSample(float input)
    {
        // Stage 1: Saturate state variables before HP calculation
        float s1sat = AnalogDSP::saturateAsymmetric(s1);
        s1sat = feedbackSmoother.process(s1sat);
        float s2sat = AnalogDSP::saturateAsymmetric(s2);

        // Level-dependent resonance: envelope follower modulates R2
        levelTracker += 0.0005f * (std::abs(input) - levelTracker);
        float R2mod = R2 * (1.0f + levelTracker * 0.08f);
        float hMod = 1.0f / (1.0f + R2mod * g + g * g);

        // SVF core (same topology as JUCE, using saturated states)
        float yHP = hMod * (input - s1sat * (g + R2mod) - s2sat);
        float yBP = yHP * g + s1;

        // Stage 2: Saturate bandpass with pre/post filtering
        yBP = preSatFilter.process(yBP);
        yBP = AnalogDSP::saturate(yBP);
        yBP = postSatFilter.process(yBP);

        // Update integrator states
        s1 = yHP * g + yBP;
        float yLP = yBP * g + s2;
        s2 = yBP * g + yLP;

        return (type == Type::lowpass) ? yLP : yHP;
    }

    void reset()
    {
        s1 = 0.0f;
        s2 = 0.0f;
        levelTracker = 0.0f;
        feedbackSmoother.reset();
        preSatFilter.reset();
        postSatFilter.reset();
    }

private:
    float s1 = 0.0f, s2 = 0.0f;
    float g = 0.0f, R2 = 0.0f;
    float levelTracker = 0.0f;
    float cutoffFrequency = 1000.0f;
    float resonance = 0.707f;
    float sampleRate = 44100.0f;
    Type type = Type::lowpass;

    OnePoleFilter feedbackSmoother;
    OnePoleFilter preSatFilter;
    OnePoleFilter postSatFilter;
};
