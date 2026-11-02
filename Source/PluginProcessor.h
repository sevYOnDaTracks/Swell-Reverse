#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "IRBuilder.h"
#include "SwellRenderer.h"

namespace ParamID
{
    static constexpr const char* length   = "length";
    static constexpr const char* sync     = "sync";
    static constexpr const char* division = "division";
    static constexpr const char* shape    = "shape";
    static constexpr const char* tone     = "tone";
    static constexpr const char* lowCut   = "lowcut";
    static constexpr const char* width    = "width";
    static constexpr const char* gap      = "gap";
    static constexpr const char* mix      = "mix";
    static constexpr const char* output   = "output";
    static constexpr const char* gate     = "gate";
    static constexpr const char* gateDepth  = "gatedepth";
    static constexpr const char* gateSmooth = "gatesmooth";
    static constexpr const char* bypass     = "bypass";
}

#ifndef SWELL_VERSION
 #define SWELL_VERSION "2.1.0"
#endif

class SwellAudioProcessor : public juce::AudioProcessor,
                            private juce::Timer
{
public:
    SwellAudioProcessor();
    ~SwellAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Swell Reverse"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    juce::AudioProcessorParameter* getBypassParameter() const override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // --- Pour l'interface ---
    std::vector<float> getDisplayEnvelope() const;
    int   getDisplayVersion() const   { return displayVersion.load(); }
    float getLengthSeconds() const    { return lengthSeconds.load(); }
    float getLatencyMs() const        { return latencyMs.load(); }
    double getHostBpm() const         { return hostBpm.load(); }

    // --- Mode Render (clip à glisser dans la playlist) ---
    enum class CaptureState { idle = 0, armed, recording };
    void toggleCapture();                         // arme / arrête la capture de l'entrée
    CaptureState getCaptureState() const { return (CaptureState) captureState.load(); }
    float getCaptureSeconds() const;
    void renderFromFile (const juce::File& f);    // sample glissé ou chargé
    void renderFromBuffer (const juce::AudioBuffer<float>& src, double sr, const juce::String& name);

    struct RenderInfo
    {
        juce::File file;
        std::vector<float> envelope;
        juce::String status;
        bool busy = false;
    };
    RenderInfo getRenderInfo() const;
    int getRenderVersion() const { return renderVersion.load(); }
    static juce::File getRenderFolder();

    // --- Pré-écoute du clip ---
    void togglePreview();
    void stopPreview()                  { previewPlaying.store (false); }
    bool isPreviewPlaying() const       { return previewPlaying.load(); }
    float getPreviewProgress() const;   // 0..1 dans le buffer de pré-écoute
    float getPreviewClipFraction() const { return previewClipFrac.load(); } // part du clip (avant le son)
    void setPreviewWithSource (bool b)  { previewWithSource.store (b); rebuildPreviewFromLast(); }
    bool getPreviewWithSource() const   { return previewWithSource.load(); }

    // --- Vumètre de sortie ---
    float getOutputPeak (int ch) const  { return ch == 0 ? peakL.load() : peakR.load(); }

    // --- Presets ---
    juce::StringArray getFactoryPresetNames() const;
    juce::StringArray getUserPresetNames() const;
    void loadFactoryPreset (int index);
    bool loadUserPreset (const juce::String& name);
    bool saveUserPreset (const juce::String& name);
    juce::String getCurrentPresetName() const;
    static juce::File getPresetFolder();

    // --- Pour le banc de test (hors temps réel) ---
    void forceRebuild();
    void finishCaptureNow() { finishCapture(); }

    static constexpr double maxLengthSeconds = 6.0;
    static constexpr double maxGapSeconds    = 0.5;

private:
    IRSettings computeSettings() const { return computeSettings (currentSampleRate); }
    IRSettings computeSettings (double sampleRate) const;
    void finishCapture();
    void setRenderStatus (const juce::String& s, bool busy);
    void applySettings (const IRSettings& s);
    void timerCallback() override;

    juce::dsp::Convolution convolution { juce::dsp::Convolution::NonUniform { 512 } };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 2 };
    juce::AudioBuffer<float> wetBuffer;

    double currentSampleRate = 44100.0;
    bool   prepared = false;

    std::atomic<double> hostBpm { 120.0 };
    std::atomic<int>    targetDelay { 0 };
    int                 currentDelay = -1;

    IRSettings appliedSettings;
    IRSettings pendingSettings;
    int        pendingTicks = 0;

    juce::SmoothedValue<float> mixSmooth, gainSmooth;

    std::atomic<float>* pLength   = nullptr;
    std::atomic<float>* pSync     = nullptr;
    std::atomic<float>* pDivision = nullptr;
    std::atomic<float>* pShape    = nullptr;
    std::atomic<float>* pTone     = nullptr;
    std::atomic<float>* pLowCut   = nullptr;
    std::atomic<float>* pWidth    = nullptr;
    std::atomic<float>* pGap      = nullptr;
    std::atomic<float>* pMix      = nullptr;
    std::atomic<float>* pOutput   = nullptr;
    std::atomic<float>* pGate       = nullptr;
    std::atomic<float>* pGateDepth  = nullptr;
    std::atomic<float>* pGateSmooth = nullptr;
    double gateFreePhaseBeats = 0.0;
    SwellRenderer::GateSettings currentGateSettings() const;

    mutable juce::SpinLock displayLock;
    std::vector<float>     displayEnv;
    std::atomic<int>       displayVersion { 0 };
    std::atomic<float>     lengthSeconds { 1.0f };
    std::atomic<float>     latencyMs { 0.0f };

    // Capture
    juce::AudioBuffer<float> captureBuffer;
    std::atomic<int>  captureState { 0 };
    std::atomic<int>  captureWrite { 0 };
    std::atomic<bool> captureDone { false };
    std::atomic<bool> captureStopRequest { false };
    bool hostWasPlaying = false;
    static constexpr double maxCaptureSeconds = 30.0;

    // Rendu
    mutable juce::CriticalSection renderLock;
    RenderInfo renderInfo;
    std::atomic<int> renderVersion { 0 };
    // Dernier son rendu : re-rendu auto quand un réglage change
    std::shared_ptr<juce::AudioBuffer<float>> lastSource;
    double       lastSourceRate = 44100.0;
    juce::String lastSourceName;
    juce::String lastRenderKey, pendingRenderKey;
    int          renderKeyTicks = 0;
    juce::String renderKey (double sr) const;

    // Pré-écoute
    void buildPreview (const juce::AudioBuffer<float>& clip, const juce::AudioBuffer<float>& source,
                       int transientSample, double sr);
    void rebuildPreviewFromLast();
    juce::SpinLock previewLock;
    juce::AudioBuffer<float> previewBuffer;
    int previewPos = 0;
    std::atomic<int>   previewLength { 0 };
    std::atomic<int>   previewPosAtomic { 0 };
    std::atomic<bool>  previewPlaying { false };
    std::atomic<bool>  previewWithSource { true };
    std::atomic<float> previewClipFrac { 1.0f };
    juce::CriticalSection lastResultLock;
    juce::AudioBuffer<float> lastClip;
    int    lastTransient = 0;
    double lastClipRate = 44100.0;

    // Bypass + vumètre
    juce::AudioParameterBool* bypassParam = nullptr;
    juce::SmoothedValue<float> bypassSmooth;
    std::atomic<float> peakL { 0.0f }, peakR { 0.0f };

    juce::ThreadPool renderPool { 1 };   // déclaré en dernier : détruit en premier (attend le rendu)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwellAudioProcessor)
};
