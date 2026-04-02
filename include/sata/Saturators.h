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
 * @file Saturators.h
 * @brief Declares stateless saturation helpers shared across LPF9000 analog DSP stages.
 */

#pragma once

/**
 * @brief Namespace containing inline analog-style saturation helper functions.
 */
namespace AnalogDSP
{
    /**
     * @brief Applies a soft odd-symmetric rational saturator.
     * @param x Input sample.
     * @return Saturated output sample.
     */
    inline float saturate(float x)
    {
        return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
    }

    /**
     * @brief Applies an asymmetric saturator that generates additional even harmonics.
     * @param x Input sample.
     * @return Saturated output sample.
     */
    inline float saturateAsymmetric(float x)
    {
        if (x >= 0.0f)
            return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
        else
            return x * (24.0f + x * x) / (24.0f + 9.0f * x * x);
    }
}
