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
 * @file OnePoleFilter.h
 * @brief Declares a lightweight one-pole low-pass filter for smoothing and spectral splitting.
 */

#pragma once

#include <JuceHeader.h>
#include <cmath>

/**
 * @brief Minimal one-pole low-pass filter used throughout the LPF9000 support DSP.
 */
class OnePoleFilter
{
public:
    /**
     * @brief Sets the filter cutoff frequency.
     * @param freqHz Cutoff frequency in hertz.
     * @param sampleRate Active sample rate in hertz.
     */
    void setCutoff(float freqHz, float sampleRate)
    {
        a0 = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * freqHz / sampleRate);
    }

    /**
     * @brief Processes one input sample.
     * @param x Input sample.
     * @return Filtered output sample.
     */
    float process(float x)
    {
        z1 += a0 * (x - z1);
        return z1;
    }

    /**
     * @brief Resets the filter state.
     */
    void reset() { z1 = 0.0f; }

private:
    /** @brief Low-pass coefficient derived from the current cutoff. */
    float a0 = 0.1f;
    /** @brief One-sample state value. */
    float z1 = 0.0f;
};
