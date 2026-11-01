#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "IRBuilder.h"
#include <cmath>

// Rendu hors-ligne : à partir d'un son (capturé ou chargé), fabrique le clip
// de reverse reverb dont la FIN tombe pile sur le transient du son.
namespace SwellRenderer
{
    // Gate rythmique : phase 0..1 sur une période ; la moitié 0..0.5 est "ouverte".
    // smooth = 0 -> découpe sèche, 1 -> vague douce (wou-wou)
    inline float gateGain (double phase, float depth, float smooth, double periodSec)
    {
        phase -= std::floor (phase);
        const double ramp = juce::jlimit (0.001, 0.25, 0.002 / juce::jmax (1.0e-4, periodSec));
        double hard;
        if (phase < ramp)             hard = phase / ramp;
        else if (phase < 0.5 - ramp)  hard = 1.0;
        else if (phase < 0.5)         hard = (0.5 - phase) / ramp;
        else                          hard = 0.0;
        const double soft = 0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * phase);
        const double g = hard + (soft - hard) * smooth;
        return (float) (1.0 - depth * (1.0 - g));
    }

    struct GateSettings
    {
        double rateBeats = 0.0;   // 0 = gate désactivé
        float  depth = 1.0f;
        float  smooth = 0.3f;
        double bpm = 120.0;
    };

    struct Result
    {
        juce::AudioBuffer<float> clip;  // stéréo, durée = montée + gap
        int    transientSample = 0;     // position du transient dans le son source
        double sampleRate = 44100.0;
    };

    // Premier transient (attaque) du son, en échantillons.
    int detectTransient (const juce::AudioBuffer<float>& source, double sampleRate);

    // Calcule le clip : reverb de la source, inversée, coupée au transient.
    Result render (const juce::AudioBuffer<float>& source, double sampleRate,
                   const IRSettings& settings, float outputGainDb,
                   const GateSettings& gate = {});
}
