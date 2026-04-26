/*
============================================================
                         LPF9000
============================================================

 _       ______ _______ ____   ______  ______  ______ 
/ /     (_____ (_______) __ \ / __   |/ __   |/ __   |
/ /      _____) )____ ( (__) ) | //| | | //| | | //| |
/ /     |  ____/  ___) \__  /| |// | | |// | | |// | |
/ /_____| |    | |       / / |  /__| |  /__| |  /__| |
/_______)_|    |_|      /_/   \_____/ \_____/ \_____/ 

============================================================
*/

/**
 * @file AnalogSVF.h
 * @brief Declares a nonlinear state-variable filter used by LPF9000 analog processing stages.
 */

#pragma once

#include "OnePoleFilter.h"
#include "Saturators.h"

/**
 * @brief Mono state-variable filter with saturation-aware state updates and nonlinear feedback shaping.
 */
class AnalogSVF
{
public:
    /**
     * @brief Output mode for the state-variable filter.
     */
    enum class Type { lowpass, highpass };

    /**
     * @brief Prepares the filter for a new sample rate.
     * @param newSampleRate Active sample rate in hertz.
     */
    void prepare(float newSampleRate)
    {
        sampleRate = newSampleRate;
        const float nyquist = sampleRate * 0.49f;
        envCoeff = 1.0f - std::exp(-1.0f / (0.045f * sampleRate));
        feedbackSmoother.setCutoff(juce::jmin(12000.0f, nyquist), sampleRate);
        preSatFilter.setCutoff(juce::jmin(8000.0f, nyquist), sampleRate);
        postSatFilter.setCutoff(juce::jmin(10000.0f, nyquist), sampleRate);
        setCoefficients(cutoffFrequency, resonance);
    }

    /**
     * @brief Updates cutoff and resonance coefficients.
     * @param freqHz Cutoff frequency in hertz.
     * @param res Resonance amount.
     */
    void setCoefficients(float freqHz, float res)
    {
        cutoffFrequency = juce::jlimit(20.0f, sampleRate * 0.49f, freqHz);
        resonance = juce::jmax(0.001f, res);

        g = std::tan(juce::MathConstants<float>::pi * cutoffFrequency / sampleRate);
        R2 = 1.0f / resonance;
    }

    /**
     * @brief Selects the output response to return from processing.
     * @param newType Desired response type.
     */
    void setType(Type newType) { type = newType; }

    /**
     * @brief Processes one sample through the nonlinear state-variable filter.
     * @param input Input sample.
     * @return Filtered output sample.
     */
    float processSample(float input)
    {
        // Stage 1: Saturate state variables before HP calculation
        float s1sat = AnalogDSP::saturateAsymmetric(s1);
        s1sat = feedbackSmoother.process(s1sat);
        float s2sat = AnalogDSP::saturateAsymmetric(s2);

        // Level-dependent resonance: envelope follower modulates R2
        levelTracker += envCoeff * (std::abs(input) - levelTracker);
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

    /**
     * @brief Resets all internal state and helper filters.
     */
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
    /** @brief First integrator state. */
    float s1 = 0.0f, s2 = 0.0f;
    /** @brief Precomputed TPT coefficient and reciprocal resonance term. */
    float g = 0.0f, R2 = 0.0f;
    /** @brief Envelope coefficient for level-dependent resonance tracking. */
    float envCoeff = 0.0f;
    /** @brief Running level estimate used to modulate resonance. */
    float levelTracker = 0.0f;
    /** @brief Cached cutoff frequency in hertz. */
    float cutoffFrequency = 1000.0f;
    /** @brief Cached resonance value. */
    float resonance = 0.707f;
    /** @brief Active sample rate in hertz. */
    float sampleRate = 44100.0f;
    /** @brief Currently selected filter output type. */
    Type type = Type::lowpass;

    /** @brief Smoothing filter applied to the saturated feedback state. */
    OnePoleFilter feedbackSmoother;
    /** @brief Prefilter before the band-pass saturation stage. */
    OnePoleFilter preSatFilter;
    /** @brief Postfilter after the band-pass saturation stage. */
    OnePoleFilter postSatFilter;
};
