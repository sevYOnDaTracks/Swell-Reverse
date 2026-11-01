#include "IRBuilder.h"
#include <algorithm>
#include <cmath>

namespace
{
    // Passe-haut biquad (RBJ), 12 dB/oct
    struct HighPass
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        HighPass (double fc, double sr)
        {
            const double w0 = juce::MathConstants<double>::twoPi * fc / sr;
            const double cw = std::cos (w0), sw = std::sin (w0);
            const double q = 0.7071, alpha = sw / (2.0 * q);
            const double a0 = 1.0 + alpha;
            b0 = ((1.0 + cw) * 0.5) / a0;
            b1 = -(1.0 + cw) / a0;
            b2 = b0;
            a1 = (-2.0 * cw) / a0;
            a2 = (1.0 - alpha) / a0;
        }

        float process (float x)
        {
            const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            return (float) y;
        }
    };
}

juce::AudioBuffer<float> IRBuilder::build (const IRSettings& s, double sr)
{
    const int L = juce::jmax (16, s.lengthSamples);
    juce::AudioBuffer<float> ir (2, L);

    // Bruit déterministe : la même IR est reconstruite à chaque fois (pas de "saut" de son)
    juce::Random rngA (0x5e7a1), rngB (0x5e7b2);

    // Courbe de décroissance : plus "shape" est haut, plus la montée inversée est tardive
    const double k = juce::jmap ((double) s.shape, 1.2, 11.0);

    // Filtrage qui s'assombrit au fil de la queue (comme une vraie salle)
    const double nyqSafe = 0.45 * sr;
    const double fcStart = juce::jmin (18000.0, nyqSafe);
    const double fcEnd   = juce::jmin (nyqSafe, 500.0 * std::pow (30.0, (double) s.tone)); // 500 Hz .. 15 kHz

    const float  w     = juce::jlimit (0.0f, 1.0f, s.width);
    const float  normW = 1.0f / std::sqrt ((1.0f - w) * (1.0f - w) + w * w);

    const int fadeIn  = juce::jmax (1, (int) (0.003 * sr));  // 3 ms : adoucit l'attaque de la queue
    const int fadeOut = juce::jmax (1, L / 8);                // la fin de queue part du silence

    double lpA1 = 0, lpA2 = 0, lpB1 = 0, lpB2 = 0;
    HighPass hpL (s.lowCutHz, sr), hpR (s.lowCutHz, sr);

    auto* l = ir.getWritePointer (0);
    auto* r = ir.getWritePointer (1);

    for (int n = 0; n < L; ++n)
    {
        const double t  = (double) n / (double) L;
        const double fc = fcStart * std::pow (fcEnd / fcStart, t);
        const double a  = 1.0 - std::exp (-juce::MathConstants<double>::twoPi * fc / sr);

        const double na = rngA.nextFloat() * 2.0 - 1.0;
        const double nb = rngB.nextFloat() * 2.0 - 1.0;

        // deux passe-bas 1 pôle en cascade (12 dB/oct)
        lpA1 += a * (na - lpA1);  lpA2 += a * (lpA1 - lpA2);
        lpB1 += a * (nb - lpB1);  lpB2 += a * (lpB1 - lpB2);

        double env = std::exp (-k * t);
        if (n < fadeIn)
            env *= 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * n / fadeIn);
        if (n >= L - fadeOut)
        {
            const double u = (double) (L - 1 - n) / (double) fadeOut;
            env *= 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * u);
        }

        const float left  = (float) (lpA2 * env);
        const float right = (float) (((1.0f - w) * lpA2 + w * lpB2) * normW * env);

        l[n] = hpL.process (left);
        r[n] = hpR.process (right);
    }

    // Normalisation en énergie : volume perçu stable quelle que soit la longueur
    double energy = 0.0;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = ir.getReadPointer (ch);
        for (int n = 0; n < L; ++n)
            energy += (double) d[n] * d[n];
    }
    energy *= 0.5;
    if (energy > 0.0)
        ir.applyGain ((float) (0.8 / std::sqrt (energy)));

    // Inversion dans le temps : c'est ce qui crée l'effet "reverse"
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = ir.getWritePointer (ch);
        std::reverse (d, d + L);
    }

    return ir;
}

std::vector<float> IRBuilder::envelope (const juce::AudioBuffer<float>& ir, int nbBins)
{
    std::vector<float> env ((size_t) nbBins, 0.0f);
    const int L = ir.getNumSamples();
    if (L == 0 || nbBins <= 0)
        return env;

    float peak = 0.0f;
    for (int b = 0; b < nbBins; ++b)
    {
        const int start = (int) ((int64_t) b * L / nbBins);
        const int end   = juce::jmax (start + 1, (int) ((int64_t) (b + 1) * L / nbBins));
        double sum = 0.0;
        for (int ch = 0; ch < ir.getNumChannels(); ++ch)
        {
            auto* d = ir.getReadPointer (ch);
            for (int n = start; n < end; ++n)
                sum += (double) d[n] * d[n];
        }
        const float rms = (float) std::sqrt (sum / ((end - start) * ir.getNumChannels()));
        env[(size_t) b] = rms;
        peak = juce::jmax (peak, rms);
    }

    if (peak > 0.0f)
        for (auto& v : env)
            v /= peak;

    return env;
}
