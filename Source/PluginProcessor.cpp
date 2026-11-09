#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace
{
    // Divisions en temps (noires)
    const juce::StringArray divisionNames { "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars" };
    constexpr double divisionBeats[] = { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0 };

    const juce::StringArray gateNames { "Off", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
    constexpr double gateBeats[] = { 0.0, 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };

    // Presets d'usine : (id, valeur réelle)
    struct FactoryPreset { const char* name; std::vector<std::pair<const char*, float>> values; };
    const std::vector<FactoryPreset>& factoryPresets()
    {
        static const std::vector<FactoryPreset> p {
            { "Init",             { } },
            { "Vocal Swell",      { { "sync", 1 }, { "division", 3 }, { "shape", 0.45f }, { "tone", 0.7f }, { "lowcut", 250 }, { "width", 0.9f }, { "mix", 0.45f } } },
            { "Snare Suck",       { { "sync", 1 }, { "division", 2 }, { "shape", 0.8f }, { "tone", 0.55f }, { "lowcut", 200 }, { "width", 0.8f }, { "mix", 0.5f } } },
            { "Long Ghost",       { { "sync", 1 }, { "division", 5 }, { "shape", 0.3f }, { "tone", 0.35f }, { "lowcut", 300 }, { "width", 1.0f }, { "mix", 0.6f } } },
            { "Afro Perc Wou",    { { "sync", 1 }, { "division", 3 }, { "shape", 0.6f }, { "tone", 0.65f }, { "lowcut", 180 }, { "gate", 4 }, { "gatedepth", 0.85f }, { "gatesmooth", 0.7f }, { "mix", 0.5f } } },
            { "Stutter 1/16",     { { "sync", 1 }, { "division", 4 }, { "shape", 0.55f }, { "tone", 0.75f }, { "gate", 4 }, { "gatedepth", 1.0f }, { "gatesmooth", 0.0f }, { "mix", 0.5f } } },
            { "Triplet Wave",     { { "sync", 1 }, { "division", 4 }, { "shape", 0.5f }, { "tone", 0.6f }, { "gate", 3 }, { "gatedepth", 0.9f }, { "gatesmooth", 1.0f }, { "mix", 0.5f } } },
            { "Gap Rise",         { { "sync", 1 }, { "division", 3 }, { "shape", 0.7f }, { "tone", 0.6f }, { "gap", 120 }, { "mix", 0.5f } } },
        };
        return p;
    }
}

SwellAudioProcessor::SwellAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SwellState", createLayout())
{
    pLength   = apvts.getRawParameterValue (ParamID::length);
    pSync     = apvts.getRawParameterValue (ParamID::sync);
    pDivision = apvts.getRawParameterValue (ParamID::division);
    pShape    = apvts.getRawParameterValue (ParamID::shape);
    pTone     = apvts.getRawParameterValue (ParamID::tone);
    pLowCut   = apvts.getRawParameterValue (ParamID::lowCut);
    pWidth    = apvts.getRawParameterValue (ParamID::width);
    pGap      = apvts.getRawParameterValue (ParamID::gap);
    pMix      = apvts.getRawParameterValue (ParamID::mix);
    pOutput   = apvts.getRawParameterValue (ParamID::output);
    pGate       = apvts.getRawParameterValue (ParamID::gate);
    pGateDepth  = apvts.getRawParameterValue (ParamID::gateDepth);
    pGateSmooth = apvts.getRawParameterValue (ParamID::gateSmooth);
    bypassParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamID::bypass));

    startTimerHz (20);
}

SwellAudioProcessor::~SwellAudioProcessor()
{
    stopTimer();
}

juce::AudioProcessorValueTreeState::ParameterLayout SwellAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto secRange = NormalisableRange<float> (0.1f, 4.0f, 0.001f);
    secRange.setSkewForCentre (1.0f);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::length, 1 }, "Length", secRange, 1.0f,
        AudioParameterFloatAttributes().withLabel ("s")
            .withStringFromValueFunction ([] (float v, int) { return String (v, 2) + " s"; })));

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::sync, 1 }, "Sync", true));

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::division, 1 }, "Division", divisionNames, 3));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::shape, 1 }, "Shape",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + " %"; })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::tone, 1 }, "Tone",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.6f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + " %"; })));

    auto lcRange = NormalisableRange<float> (20.0f, 1000.0f, 1.0f);
    lcRange.setSkewForCentre (200.0f);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::lowCut, 1 }, "Low Cut", lcRange, 180.0f,
        AudioParameterFloatAttributes().withLabel ("Hz")
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " Hz"; })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::width, 1 }, "Width",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.85f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + " %"; })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::gap, 1 }, "Gap",
        NormalisableRange<float> (0.0f, 500.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("ms")
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::mix, 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + " %"; })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::output, 1 }, "Output",
        NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::gate, 1 }, "Gate", gateNames, 0));

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::bypass, 1 }, "Bypass", false));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::gateDepth, 1 }, "Gate Depth",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + " %"; })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::gateSmooth, 1 }, "Gate Smooth",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + " %"; })));

    return { p.begin(), p.end() };
}

bool SwellAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

IRSettings SwellAudioProcessor::computeSettings (double sampleRateToUse) const
{
    double seconds;
    if (pSync->load() > 0.5f)
    {
        const int idx = juce::jlimit (0, (int) std::size (divisionBeats) - 1, (int) pDivision->load());
        const double bpm = juce::jlimit (20.0, 999.0, hostBpm.load());
        seconds = divisionBeats[idx] * 60.0 / bpm;
    }
    else
    {
        seconds = pLength->load();
    }
    seconds = juce::jlimit (0.05, maxLengthSeconds, seconds);

    IRSettings s;
    s.lengthSamples = juce::jmax (16, (int) std::round (seconds * sampleRateToUse));
    s.gapSamples    = (int) std::round (juce::jlimit (0.0, maxGapSeconds, (double) pGap->load() / 1000.0) * sampleRateToUse);
    s.shape         = pShape->load();
    s.tone          = pTone->load();
    s.lowCutHz      = pLowCut->load();
    s.width         = pWidth->load();
    return s;
}

void SwellAudioProcessor::applySettings (const IRSettings& s)
{
    auto ir  = IRBuilder::build (s, currentSampleRate);
    auto env = IRBuilder::envelope (ir, 240);

    convolution.loadImpulseResponse (std::move (ir), currentSampleRate,
                                     juce::dsp::Convolution::Stereo::yes,
                                     juce::dsp::Convolution::Trim::no,
                                     juce::dsp::Convolution::Normalise::no);

    // Le pic de l'IR inversée est à l'indice (L - 1). En déclarant cette latence
    // à l'hôte (+ le gap), FL Studio recale tout : la montée finit sur le transient.
    const int latency = s.lengthSamples - 1 + s.gapSamples;
    setLatencySamples (latency);
    targetDelay.store (latency);

    appliedSettings = s;
    pendingSettings = s;

    {
        const juce::SpinLock::ScopedLockType lock (displayLock);
        displayEnv = std::move (env);
    }
    lengthSeconds.store ((float) (s.lengthSamples / currentSampleRate));
    latencyMs.store ((float) (1000.0 * latency / currentSampleRate));
    ++displayVersion;
}

void SwellAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    applySettings (computeSettings());
    convolution.prepare (spec);
    convolution.reset();

    const int maxDelay = (int) std::ceil ((maxLengthSeconds + maxGapSeconds) * sampleRate) + 16;
    dryDelay.prepare (spec);
    dryDelay.setMaximumDelayInSamples (maxDelay);
    dryDelay.reset();
    currentDelay = -1;

    wetBuffer.setSize (2, samplesPerBlock);

    // Tampon de capture (alloué ici, jamais dans le thread audio)
    const int capLen = (int) (maxCaptureSeconds * sampleRate);
    if (captureBuffer.getNumSamples() != capLen)
    {
        captureState.store ((int) CaptureState::idle);
        captureBuffer.setSize (2, capLen);
    }

    mixSmooth.reset (sampleRate, 0.05);
    gainSmooth.reset (sampleRate, 0.05);
    bypassSmooth.reset (sampleRate, 0.03);
    bypassSmooth.setCurrentAndTargetValue (bypassParam->get() ? 1.0f : 0.0f);
    mixSmooth.setCurrentAndTargetValue (pMix->load());
    gainSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    prepared = true;
}

void SwellAudioProcessor::timerCallback()
{
    if (captureDone.exchange (false))
        finishCapture();

    // Re-rendu automatique du clip quand un réglage change (anti-rebond ~300 ms)
    if (lastSource != nullptr && ! getRenderInfo().busy)
    {
        const auto key = renderKey (lastSourceRate);
        if (key != pendingRenderKey)
        {
            pendingRenderKey = key;
            renderKeyTicks = 0;
        }
        else if (key != lastRenderKey && ++renderKeyTicks >= 6)
        {
            auto src = lastSource;
            renderFromBuffer (*src, lastSourceRate, lastSourceName);
        }
    }

    if (! prepared)
        return;

    // Anti-rebond : on reconstruit l'IR quand les réglages sont stables ~100 ms
    const auto s = computeSettings();
    if (s != pendingSettings)
    {
        pendingSettings = s;
        pendingTicks = 0;
        return;
    }
    if (s != appliedSettings && ++pendingTicks >= 2)
        applySettings (s);
}

void SwellAudioProcessor::forceRebuild()
{
    applySettings (computeSettings());
}

void SwellAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{ if (! licence->isUnlocked()) { buffer.clear(); return; } 
    juce::ScopedNoDenormals noDenormals;

    // Tempo et état de lecture de l'hôte
    bool hostPlaying = false, hostKnowsPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm())
                if (*bpm > 0.0)
                    hostBpm.store (*bpm);
            hostPlaying = pos->getIsPlaying();
            hostKnowsPlaying = true;
        }

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, buffer.getNumChannels());
    if (numSamples == 0 || numChannels == 0)
        return;

    // --- Capture de l'entrée (mode Render) ---
    {
        auto state = (CaptureState) captureState.load (std::memory_order_acquire);
        if (state != CaptureState::idle)
        {
            int start = numSamples;
            if (state == CaptureState::armed)
            {
                // démarre au premier son non silencieux (-60 dB)
                for (int n = 0; n < numSamples && start == numSamples; ++n)
                    for (int ch = 0; ch < numChannels; ++ch)
                        if (std::abs (buffer.getSample (ch, n)) > 0.001f) { start = n; break; }
                if (start < numSamples)
                {
                    // garde 10 ms avant l'attaque si possible (dans ce bloc)
                    start = juce::jmax (0, start - (int) (0.010 * currentSampleRate));
                    state = CaptureState::recording;
                    captureState.store ((int) state, std::memory_order_release);
                }
            }
            else
            {
                start = 0;
            }

            if (state == CaptureState::recording)
            {
                const int w = captureWrite.load();
                const int n = juce::jmin (numSamples - start, captureBuffer.getNumSamples() - w);
                if (n > 0)
                {
                    captureBuffer.copyFrom (0, w, buffer, 0, start, n);
                    captureBuffer.copyFrom (1, w, buffer, numChannels > 1 ? 1 : 0, start, n);
                    captureWrite.store (w + n);
                }

                const bool full    = captureWrite.load() >= captureBuffer.getNumSamples();
                const bool stopped = hostKnowsPlaying && hostWasPlaying && ! hostPlaying;
                if (full || stopped || captureStopRequest.load())
                {
                    captureStopRequest.store (false);
                    captureState.store ((int) CaptureState::idle, std::memory_order_release);
                    captureDone.store (true);
                }
            }
        }
        hostWasPlaying = hostPlaying;
    }

    // Copie stéréo du signal pour la voie "wet" (mono -> dupliqué)
    wetBuffer.setSize (2, numSamples, false, false, true);
    wetBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
    wetBuffer.copyFrom (1, 0, buffer, numChannels > 1 ? 1 : 0, 0, numSamples);

    {
        juce::dsp::AudioBlock<float> block (wetBuffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        convolution.process (ctx);
    }

    // Gate rythmique sur la voie wet (optionnel), calé sur le tempo de l'hôte.
    // La sortie correspond au temps musical (position d'entrée - latence).
    {
        const auto g = currentGateSettings();
        if (g.rateBeats > 0.0 && g.depth > 0.0f)
        {
            const double beatsPerSample = g.bpm / 60.0 / currentSampleRate;
            double startBeats = gateFreePhaseBeats;
            if (auto* ph = getPlayHead())
                if (auto pos = ph->getPosition())
                    if (pos->getIsPlaying())
                        if (auto ppq = pos->getPpqPosition())
                            startBeats = *ppq;
            startBeats -= getLatencySamples() * beatsPerSample;
            const double periodSec = g.rateBeats * 60.0 / g.bpm;

            auto* w0 = wetBuffer.getWritePointer (0);
            auto* w1 = wetBuffer.getWritePointer (1);
            for (int n = 0; n < numSamples; ++n)
            {
                const double phase = (startBeats + n * beatsPerSample) / g.rateBeats + 0.5;
                const float gg = SwellRenderer::gateGain (phase, g.depth, g.smooth, periodSec);
                w0[n] *= gg;
                w1[n] *= gg;
            }
            gateFreePhaseBeats += numSamples * beatsPerSample;
        }
    }

    // Voie "dry" retardée de la même latence pour rester calée
    const int td = targetDelay.load();
    if (td != currentDelay)
    {
        dryDelay.setDelay ((float) td);
        currentDelay = td;
    }

    mixSmooth.setTargetValue (pMix->load());
    gainSmooth.setTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    auto* outL = buffer.getWritePointer (0);
    auto* outR = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;
    const auto* wL = wetBuffer.getReadPointer (0);
    const auto* wR = wetBuffer.getReadPointer (1);

    bypassSmooth.setTargetValue (bypassParam->get() ? 1.0f : 0.0f);

    for (int n = 0; n < numSamples; ++n)
    {
        const float mix  = mixSmooth.getNextValue();
        const float gain = gainSmooth.getNextValue();
        const float byp  = bypassSmooth.getNextValue();
        // 0 % = dry seul, 50 % = dry + wet à fond, 100 % = wet seul
        // Bypass : dry seul (toujours retardé pour garder la latence constante)
        const float dryG = juce::jmin (1.0f, 2.0f * (1.0f - mix)) * gain * (1.0f - byp) + byp;
        const float wetG = juce::jmin (1.0f, 2.0f * mix) * gain * (1.0f - byp);

        dryDelay.pushSample (0, outL[n]);
        const float dL = dryDelay.popSample (0);

        if (outR != nullptr)
        {
            dryDelay.pushSample (1, outR[n]);
            const float dR = dryDelay.popSample (1);
            outL[n] = dL * dryG + wL[n] * wetG;
            outR[n] = dR * dryG + wR[n] * wetG;
        }
        else
        {
            outL[n] = dL * dryG + 0.5f * (wL[n] + wR[n]) * wetG;
        }
    }

    // Pré-écoute du clip rendu, mixée par-dessus la sortie
    if (previewPlaying.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (previewLock);
        if (lock.isLocked())
        {
            const int len = previewBuffer.getNumSamples();
            const int n = juce::jmin (numSamples, len - previewPos);
            if (n > 0)
            {
                buffer.addFrom (0, 0, previewBuffer, 0, previewPos, n);
                if (numChannels > 1)
                    buffer.addFrom (1, 0, previewBuffer, 1, previewPos, n);
                previewPos += n;
            }
            if (previewPos >= len)
                previewPlaying.store (false);
            previewPosAtomic.store (previewPos);
        }
    }

    // Vumètre (crête du bloc)
    peakL.store (buffer.getMagnitude (0, 0, numSamples));
    peakR.store (buffer.getMagnitude (numChannels > 1 ? 1 : 0, 0, numSamples));
}

juce::AudioProcessorParameter* SwellAudioProcessor::getBypassParameter() const
{
    return bypassParam;
}

// ============================================================ Pré-écoute
void SwellAudioProcessor::togglePreview()
{
    if (previewPlaying.load())
    {
        previewPlaying.store (false);
        return;
    }
    {
        const juce::SpinLock::ScopedLockType lock (previewLock);
        if (previewBuffer.getNumSamples() == 0)
            return;
        previewPos = 0;
        previewPosAtomic.store (0);
    }
    previewPlaying.store (true);
}

float SwellAudioProcessor::getPreviewProgress() const
{
    const int len = previewLength.load();
    return len > 0 ? (float) previewPosAtomic.load() / (float) len : 0.0f;
}

void SwellAudioProcessor::buildPreview (const juce::AudioBuffer<float>& clip, const juce::AudioBuffer<float>& source,
                                        int transientSample, double sr)
{
    // clip (finit au transient) + le son original à partir du transient (2,5 s max)
    const int srcPart = previewWithSource.load()
        ? juce::jlimit (0, juce::jmax (0, source.getNumSamples() - transientSample), (int) (2.5 * sr)) : 0;
    const int total = clip.getNumSamples() + srcPart;

    juce::AudioBuffer<float> joined (2, juce::jmax (1, total));
    joined.clear();
    for (int ch = 0; ch < 2; ++ch)
    {
        joined.copyFrom (ch, 0, clip, ch, 0, clip.getNumSamples());
        if (srcPart > 0)
            joined.copyFrom (ch, clip.getNumSamples(), source, juce::jmin (ch, source.getNumChannels() - 1),
                             transientSample, srcPart);
    }

    // Rééchantillonnage vers la fréquence de l'hôte si besoin
    juce::AudioBuffer<float> out;
    const double hostSr = currentSampleRate;
    if (std::abs (sr - hostSr) < 1.0)
    {
        out = std::move (joined);
    }
    else
    {
        const double ratio = sr / hostSr;
        const int outLen = (int) std::floor (total / ratio);
        out.setSize (2, juce::jmax (1, outLen));
        for (int ch = 0; ch < 2; ++ch)
        {
            juce::LagrangeInterpolator interp;
            interp.process (ratio, joined.getReadPointer (ch), out.getWritePointer (ch), outLen);
        }
    }

    previewClipFrac.store (total > 0 ? (float) clip.getNumSamples() / (float) total : 1.0f);
    const bool wasPlaying = previewPlaying.exchange (false);
    {
        const juce::SpinLock::ScopedLockType lock (previewLock);
        std::swap (previewBuffer, out);
        previewPos = 0;
        previewPosAtomic.store (0);
        previewLength.store (previewBuffer.getNumSamples());
    }
    juce::ignoreUnused (wasPlaying);
}

void SwellAudioProcessor::rebuildPreviewFromLast()
{
    const juce::ScopedLock sl (lastResultLock);
    if (lastClip.getNumSamples() > 0 && lastSource != nullptr)
        buildPreview (lastClip, *lastSource, lastTransient, lastClipRate);
}

// ============================================================ Mode Render
static juce::String fr (const char* utf8) { return juce::String (juce::CharPointer_UTF8 (utf8)); }

juce::File SwellAudioProcessor::getRenderFolder()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Swell Reverse");
}

void SwellAudioProcessor::toggleCapture()
{
    const auto state = getCaptureState();
    if (state == CaptureState::idle)
    {
        if (captureBuffer.getNumSamples() == 0)
        {
            setRenderStatus (fr ("Start playback once before capturing"), false);
            return;
        }
        captureStopRequest.store (false);
        captureDone.store (false);
        captureWrite.store (0);
        captureState.store ((int) CaptureState::armed, std::memory_order_release);
        setRenderStatus (fr ("Capture armed: start playback in your DAW"), false);
    }
    else if (state == CaptureState::armed)
    {
        captureState.store ((int) CaptureState::idle);
        setRenderStatus (fr ("Capture cancelled"), false);
    }
    else
    {
        captureStopRequest.store (true); // le thread audio clôt la capture au prochain bloc
    }
}

float SwellAudioProcessor::getCaptureSeconds() const
{
    return (float) (captureWrite.load() / juce::jmax (1.0, currentSampleRate));
}

void SwellAudioProcessor::finishCapture()
{
    const int n = captureWrite.load();
    if (n < (int) (0.02 * currentSampleRate))
    {
        setRenderStatus (fr ("Capture too short"), false);
        return;
    }
    juce::AudioBuffer<float> src (2, n);
    for (int ch = 0; ch < 2; ++ch)
        src.copyFrom (ch, 0, captureBuffer, ch, 0, n);
    renderFromBuffer (src, currentSampleRate, "capture");
}

void SwellAudioProcessor::renderFromFile (const juce::File& f)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (f));
    if (reader == nullptr)
    {
        setRenderStatus (fr ("Unsupported format: ") + f.getFileName(), false);
        return;
    }
    const auto maxLen = (juce::int64) (maxCaptureSeconds * reader->sampleRate);
    const int len = (int) juce::jmin (reader->lengthInSamples, maxLen);
    juce::AudioBuffer<float> src ((int) juce::jlimit (1u, 2u, reader->numChannels), juce::jmax (1, len));
    reader->read (&src, 0, len, 0, true, true);
    renderFromBuffer (src, reader->sampleRate, f.getFileNameWithoutExtension());
}

void SwellAudioProcessor::renderFromBuffer (const juce::AudioBuffer<float>& source, double sr, const juce::String& name)
{
    const auto settings = computeSettings (sr);
    const float gainDb  = pOutput->load();
    const auto gate     = currentGateSettings();
    const juce::String tag = pSync->load() > 0.5f
        ? juce::String (juce::roundToInt (hostBpm.load())) + "bpm"
        : juce::String (settings.lengthSamples / sr, 2) + "s";

    setRenderStatus (fr ("Rendering…"), true);

    auto srcCopy = std::make_shared<juce::AudioBuffer<float>> (source);
    lastSource      = srcCopy;
    lastSourceRate  = sr;
    lastSourceName  = name;
    lastRenderKey   = renderKey (sr);
    pendingRenderKey = lastRenderKey;
    renderPool.addJob ([this, srcCopy, sr, settings, gainDb, gate, name, tag]
    {
        auto res = SwellRenderer::render (*srcCopy, sr, settings, gainDb, gate);

        auto folder = getRenderFolder();
        folder.createDirectory();
        const auto safeName = juce::File::createLegalFileName (name).replaceCharacter (' ', '_');
        auto file = folder.getChildFile ("Swell_" + safeName + "_" + tag + "_"
                                         + juce::Time::getCurrentTime().formatted ("%H%M%S")
                                         + juce::String (juce::Time::getCurrentTime().getMilliseconds()).paddedLeft ('0', 3) + ".wav");

        bool ok = false;
        {
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::OutputStream> os (std::make_unique<juce::FileOutputStream> (file));
            if (auto* fos = dynamic_cast<juce::FileOutputStream*> (os.get()); fos != nullptr && fos->openedOk())
            {
                auto opts = juce::AudioFormatWriterOptions{}.withSampleRate (sr).withNumChannels (2).withBitsPerSample (24);
                if (auto writer = wav.createWriterFor (os, opts))
                    ok = writer->writeFromAudioSampleBuffer (res.clip, 0, res.clip.getNumSamples());
            }
        }

        {
            const juce::ScopedLock sl (lastResultLock);
            lastClip      = res.clip;
            lastTransient = res.transientSample;
            lastClipRate  = sr;
            buildPreview (res.clip, *srcCopy, res.transientSample, sr);
        }

        const juce::ScopedLock sl (renderLock);
        renderInfo.busy = false;
        if (ok)
        {
            renderInfo.file = file;
            renderInfo.envelope = IRBuilder::envelope (res.clip, 160);
            renderInfo.status = fr ("Transient at ") + juce::String (res.transientSample / sr, 2)
                              + fr (" s  |  clip ") + juce::String (res.clip.getNumSamples() / sr, 2)
                              + fr (" s: drop it right before the sound");
        }
        else
        {
            renderInfo.status = fr ("Could not write the file to Music/Swell Reverse");
        }
        ++renderVersion;
    });
}

juce::String SwellAudioProcessor::renderKey (double sr) const
{
    const auto s = computeSettings (sr);
    const auto g = currentGateSettings();
    juce::String k;
    k << s.lengthSamples << "|" << s.gapSamples << "|" << s.shape << "|" << s.tone << "|"
      << s.lowCutHz << "|" << s.width << "|" << g.rateBeats << "|" << g.depth << "|"
      << g.smooth << "|" << juce::roundToInt (g.bpm * 10) << "|" << pOutput->load();
    return k;
}

void SwellAudioProcessor::setRenderStatus (const juce::String& s, bool busy)
{
    const juce::ScopedLock sl (renderLock);
    renderInfo.status = s;
    renderInfo.busy = busy;
    ++renderVersion;
}

SwellAudioProcessor::RenderInfo SwellAudioProcessor::getRenderInfo() const
{
    const juce::ScopedLock sl (renderLock);
    return renderInfo;
}

SwellRenderer::GateSettings SwellAudioProcessor::currentGateSettings() const
{
    SwellRenderer::GateSettings g;
    const int idx = juce::jlimit (0, (int) std::size (gateBeats) - 1, (int) pGate->load());
    g.rateBeats = gateBeats[idx];
    g.depth     = pGateDepth->load();
    g.smooth    = pGateSmooth->load();
    g.bpm       = juce::jlimit (20.0, 999.0, hostBpm.load());
    return g;
}

// ============================================================ Presets
juce::File SwellAudioProcessor::getPresetFolder()
{
    return getRenderFolder().getChildFile ("Presets");
}

juce::StringArray SwellAudioProcessor::getFactoryPresetNames() const
{
    juce::StringArray names;
    for (auto& p : factoryPresets())
        names.add (p.name);
    return names;
}

juce::StringArray SwellAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (auto& f : getPresetFolder().findChildFiles (juce::File::findFiles, false, "*.swellpreset"))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

void SwellAudioProcessor::loadFactoryPreset (int index)
{
    const auto& list = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    // repart des valeurs par défaut, puis applique le preset
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (rp != bypassParam)
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (auto& [id, value] : list[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));

    apvts.state.setProperty ("presetName", juce::String (list[(size_t) index].name), nullptr);
}

bool SwellAudioProcessor::loadUserPreset (const juce::String& name)
{
    auto f = getPresetFolder().getChildFile (name + ".swellpreset");
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    apvts.state.setProperty ("presetName", name, nullptr);
    return true;
}

bool SwellAudioProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty())
        return false;
    auto folder = getPresetFolder();
    folder.createDirectory();
    apvts.state.setProperty ("presetName", name, nullptr);
    if (auto xml = apvts.copyState().createXml())
        return xml->writeTo (folder.getChildFile (name + ".swellpreset"));
    return false;
}

juce::String SwellAudioProcessor::getCurrentPresetName() const
{
    return apvts.state.getProperty ("presetName", "Init").toString();
}

std::vector<float> SwellAudioProcessor::getDisplayEnvelope() const
{
    const juce::SpinLock::ScopedLockType lock (displayLock);
    return displayEnv;
}

void SwellAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SwellAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* SwellAudioProcessor::createEditor()
{
    return swell::LicenceGuard::attach (new SwellAudioProcessorEditor (*this));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SwellAudioProcessor();
}
