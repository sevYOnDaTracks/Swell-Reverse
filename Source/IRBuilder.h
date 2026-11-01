#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// Paramètres qui définissent la réponse impulsionnelle (IR) inversée.
struct IRSettings
{
    int   lengthSamples = 0;   // durée de la queue de reverb (= durée de la montée)
    int   gapSamples    = 0;   // silence entre la fin de la montée et le transient
    float shape         = 0.5f; // 0 = montée progressive, 1 = montée tardive et sèche
    float tone          = 0.5f; // 0 = sombre, 1 = brillant
    float lowCutHz      = 150.0f;
    float width         = 1.0f; // 0 = mono, 1 = stéréo large

    bool operator== (const IRSettings& o) const
    {
        return lengthSamples == o.lengthSamples && gapSamples == o.gapSamples
            && shape == o.shape && tone == o.tone && lowCutHz == o.lowCutHz && width == o.width;
    }
    bool operator!= (const IRSettings& o) const { return ! (*this == o); }
};

namespace IRBuilder
{
    // Construit une IR stéréo de reverb synthétique, puis l'inverse dans le temps.
    // Convoluer un signal avec cette IR donne la "montée" de reverb qui culmine
    // à la fin de l'IR : avec une latence compensée égale à (longueur - 1 + gap),
    // la montée se termine pile sur le transient (ou "gap" ms avant).
    juce::AudioBuffer<float> build (const IRSettings& s, double sampleRate);

    // Enveloppe (0..1) sur nbBins points, pour l'affichage.
    std::vector<float> envelope (const juce::AudioBuffer<float>& ir, int nbBins);
}
