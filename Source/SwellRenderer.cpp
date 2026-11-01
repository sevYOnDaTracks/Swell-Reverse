#include "SwellRenderer.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>

int SwellRenderer::detectTransient (const juce::AudioBuffer<float>& src, double sr)
{
    const int n = src.getNumSamples();
    if (n == 0)
        return 0;

    // |x| (max des canaux) + suiveur d'enveloppe à maintien de crête
    std::vector<float> env ((size_t) n);
    const float rel = (float) std::exp (-1.0 / (0.010 * sr));
    float e = 0.0f, peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        float a = 0.0f;
        for (int ch = 0; ch < src.getNumChannels(); ++ch)
            a = juce::jmax (a, std::abs (src.getSample (ch, i)));
        e = juce::jmax (a, e * rel);
        env[(size_t) i] = e;
        peak = juce::jmax (peak, a);
    }

    if (peak < 1.0e-5f)
        return 0;

    // 1) premier passage au-dessus de -20 dB du pic
    int idx = 0;
    while (idx < n && env[(size_t) idx] < 0.1f * peak)
        ++idx;

    // 2) on recule jusqu'au vrai début de l'attaque (max 30 ms)
    const int limit = juce::jmax (0, idx - (int) (0.030 * sr));
    while (idx > limit && env[(size_t) idx - 1] > 0.02f * peak)
        --idx;

    return idx;
}

SwellRenderer::Result SwellRenderer::render (const juce::AudioBuffer<float>& source, double sr,
                                             const IRSettings& s, float outputGainDb,
                                             const GateSettings& gate)
{
    Result res;
    res.sampleRate = sr;

    // Source en stéréo
    const int N = source.getNumSamples();
    juce::AudioBuffer<float> src (2, juce::jmax (1, N));
    src.clear();
    if (N > 0)
    {
        src.copyFrom (0, 0, source, 0, 0, N);
        src.copyFrom (1, 0, source, source.getNumChannels() > 1 ? 1 : 0, 0, N);
    }

    const int t0 = detectTransient (src, sr);
    res.transientSample = t0;

    const int L   = juce::jmax (16, s.lengthSamples);
    const int gap = juce::jmax (0, s.gapSamples);


    // Convolution hors-ligne avec l'IR inversée (chargée de façon synchrone par prepare)
    juce::dsp::Convolution conv { juce::dsp::Convolution::NonUniform { 1024 } };
    conv.loadImpulseResponse (IRBuilder::build (s, sr), sr,
                              juce::dsp::Convolution::Stereo::yes,
                              juce::dsp::Convolution::Trim::no,
                              juce::dsp::Convolution::Normalise::no);
    const int block = 2048;
    conv.prepare ({ sr, (juce::uint32) block, 2 });
    conv.reset();

    // y[m] correspond au temps source  tau = m - (L - 1) - gap
    // Le clip couvre tau dans [t0 - L - gap, t0)  ->  m dans [t0 - 1, t0 - 1 + L + gap)
    const int clipLen = L + gap;
    const int mStart  = t0 - 1;
    const int mEnd    = mStart + clipLen;

    // Le son après le transient (sustain) nourrit aussi la montée, jusqu'à mEnd
    const int srcLen = juce::jmin (N, mEnd);

    res.clip.setSize (2, clipLen);
    res.clip.clear();

    juce::AudioBuffer<float> buf (2, block);
    for (int pos = 0; pos < mEnd; pos += block)
    {
        const int n = juce::jmin (block, mEnd - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        if (pos < srcLen)
            for (int ch = 0; ch < 2; ++ch)
                buf.copyFrom (ch, 0, src, ch, pos, juce::jmin (n, srcLen - pos));

        juce::dsp::AudioBlock<float> ab (buf);
        conv.process (juce::dsp::ProcessContextReplacing<float> (ab));

        // recopie de la partie qui tombe dans le clip
        const int a = juce::jmax (pos, mStart);
        const int b = juce::jmin (pos + n, mEnd);
        for (int m = a; m < b; ++m)
            for (int ch = 0; ch < 2; ++ch)
                res.clip.setSample (ch, m - mStart, buf.getSample (ch, m - pos));
    }

    // Gate rythmique, calé pour être ouvert jusqu'au transient (fin du clip)
    if (gate.rateBeats > 0.0 && gate.depth > 0.0f)
    {
        const double beatsPerSample = gate.bpm / 60.0 / sr;
        const double periodSec = gate.rateBeats * 60.0 / gate.bpm;
        for (int i = 0; i < clipLen; ++i)
        {
            const double beatsToEnd = (clipLen - i) * beatsPerSample;
            const double phase = 0.5 - beatsToEnd / gate.rateBeats;
            const float g = gateGain (phase, gate.depth, gate.smooth, periodSec);
            for (int ch = 0; ch < 2; ++ch)
                res.clip.setSample (ch, i, res.clip.getSample (ch, i) * g);
        }
    }

    // Fondus : entrée douce (5 ms), sortie très courte (1 ms) pour éviter les clics
    const int fadeIn  = juce::jmin (clipLen / 4, (int) (0.005 * sr));
    const int fadeOut = juce::jmin (clipLen / 4, (int) (0.001 * sr));
    res.clip.applyGainRamp (0, fadeIn, 0.0f, 1.0f);
    res.clip.applyGainRamp (clipLen - fadeOut, fadeOut, 1.0f, 0.0f);

    // Gain de sortie + protection contre la saturation
    res.clip.applyGain (juce::Decibels::decibelsToGain (outputGainDb));
    const float pk = res.clip.getMagnitude (0, clipLen);
    if (pk > 0.98f)
        res.clip.applyGain (0.98f / pk);

    return res;
}
