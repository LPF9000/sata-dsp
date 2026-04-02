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
 * @file PinkNoiseGenerator.h
 * @brief Declares a compact pink-noise generator used by LPF9000 analog coloration paths.
 */

#pragma once

#include <JuceHeader.h>

/**
 * @brief Pink-noise generator based on octave-rate filtered white-noise updates.
 */
class PinkNoiseGenerator
{
public:
    /**
     * @brief Generates the next pink-noise sample.
     * @param rng Random generator supplying white-noise input.
     * @return Pink-noise output sample.
     */
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

    /**
     * @brief Resets the internal noise-generator state.
     */
    void reset()
    {
        b0 = 0.0f;
        b1 = 0.0f;
        b2 = 0.0f;
        counter = 0;
    }

private:
    /** @brief Filter state for the fastest-updating pink-noise band. */
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    /** @brief Counter controlling octave-rate updates of the slower bands. */
    unsigned int counter = 0;
};
