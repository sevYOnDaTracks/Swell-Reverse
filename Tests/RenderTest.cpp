// Banc de test hors-ligne : passe des transients dans le plugin et vérifie que
// la montée de reverb culmine juste avant chaque transient (latence compensée).
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/SwellRenderer.h"

using namespace juce;

static void setParam (SwellAudioProcessor& p, const char* id, float plainValue)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
}

static AudioBuffer<float> makeInput (double sr, double seconds, const std::vector<double>& hits)
{
    AudioBuffer<float> b (2, (int) (seconds * sr));
    b.clear();
    Random rng (42);
    for (double t : hits)
    {
        const int start = (int) (t * sr);
        for (int n = 0; n < (int) (0.25 * sr) && start + n < b.getNumSamples(); ++n)
        {
            const double tt = n / sr;
            // "snare" synthétique : corps 190 Hz + bruit
            const float body  = (float) (std::sin (MathConstants<double>::twoPi * 190.0 * tt) * std::exp (-tt * 25.0));
            const float noise = (rng.nextFloat() * 2.0f - 1.0f) * (float) std::exp (-tt * 18.0);
            const float v = 0.6f * (0.6f * body + 0.5f * noise);
            b.addSample (0, start + n, v);
            b.addSample (1, start + n, v);
        }
    }
    return b;
}

static AudioBuffer<float> render (SwellAudioProcessor& p, const AudioBuffer<float>& in, double sr, int& latencyOut)
{
    const int block = 512;
    p.setPlayConfigDetails (2, 2, sr, block);
    p.prepareToPlay (sr, block);
    // prepareToPlay charge l'IR de façon synchrone (Convolution::prepare)

    // Préchauffe avec du silence
    {
        AudioBuffer<float> silent (2, block); MidiBuffer m;
        for (int i = 0; i < 40; ++i) { silent.clear(); p.processBlock (silent, m); }
    }

    latencyOut = p.getLatencySamples();
    const int total = in.getNumSamples() + latencyOut;
    AudioBuffer<float> out (2, total);
    out.clear();

    MidiBuffer midi;
    AudioBuffer<float> buf (2, block);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = jmin (block, total - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        for (int ch = 0; ch < 2; ++ch)
            if (pos < in.getNumSamples())
                buf.copyFrom (ch, 0, in, ch, pos, jmin (n, in.getNumSamples() - pos));
        p.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, buf, ch, 0, n);
    }

    // Compensation de latence (ce que fait FL Studio)
    AudioBuffer<float> aligned (2, in.getNumSamples());
    for (int ch = 0; ch < 2; ++ch)
        aligned.copyFrom (ch, 0, out, ch, latencyOut, in.getNumSamples());
    return aligned;
}

static void writeWav (const File& f, const AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    WavAudioFormat wav;
    std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (new FileOutputStream (f), sr, 2, 24, {}, 0));
    w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

int main (int argc, char** argv)
{
    swell::Licence::unlockForTesting(); // rendu hors ligne, sans fichier de licence
    ScopedJuceInitialiser_GUI init;
    const File outDir (argc > 1 ? String (argv[1]) : File::getCurrentWorkingDirectory().getFullPathName());
    const double sr = 48000.0;
    const std::vector<double> hits { 1.5, 3.5 };
    auto input = makeInput (sr, 5.0, hits);
    writeWav (outDir.getChildFile ("00_input.wav"), input, sr);

    bool ok = true;

    // --- Test 1 : wet seul, sync 1/2 à 120 BPM (= 1 s) -> le pic doit être juste avant le transient
    {
        SwellAudioProcessor p;
        setParam (p, ParamID::sync, 1.0f);
        setParam (p, ParamID::division, 3.0f); // 1/2
        setParam (p, ParamID::mix, 1.0f);
        int lat = 0;
        auto out = render (p, input, sr, lat);
        writeWav (outDir.getChildFile ("01_wet_only_half_note.wav"), out, sr);

        std::printf ("[test1] latence = %d samples (%.1f ms), montee = %.3f s\n",
                     lat, 1000.0 * lat / sr, p.getLengthSeconds());

        for (double t : hits)
        {
            const int h = (int) (t * sr);
            // énergie sur des fenêtres de 10 ms avant / après le transient
            auto rms = [&] (int a, int b) {
                double s = 0; for (int n = a; n < b; ++n) s += out.getSample (0, n) * out.getSample (0, n);
                return std::sqrt (s / (b - a)); };
            const double before   = rms (h - 480, h);              // 10 ms avant
            const double early    = rms (h - 48000, h - 47520);    // début de montée (~1 s avant)
            const double after    = rms (h + 4800, h + 9600);      // 100-200 ms après
            int peakIdx = h - 60000; float peak = 0;
            for (int n = h - 60000; n < h + 24000; ++n)
                if (std::abs (out.getSample (0, n)) > peak) { peak = std::abs (out.getSample (0, n)); peakIdx = n; }
            std::printf ("  hit %.2fs : pic a %+.1f ms du transient | rms debut=%.4f  fin=%.4f  apres=%.4f\n",
                         t, 1000.0 * (peakIdx - h) / sr, early, before, after);
            ok &= (peakIdx <= h + 48 && peakIdx > h - 4800); // pic dans les 100 ms avant le transient
            ok &= before > early * 3.0;                      // ça monte bien
            ok &= after < before * 0.1;                      // et ça s'arrête au transient
        }
    }

    // --- Test 2 : gap 150 ms, longueur libre 1.5 s
    {
        SwellAudioProcessor p;
        setParam (p, ParamID::sync, 0.0f);
        setParam (p, ParamID::length, 1.5f);
        setParam (p, ParamID::gap, 150.0f);
        setParam (p, ParamID::mix, 1.0f);
        int lat = 0;
        auto out = render (p, input, sr, lat);
        const int h = (int) (hits[1] * sr); // 2e hit : fenetre d analyse dans le buffer
        int peakIdx = 0; float peak = 0;
        for (int n = h - 90000; n < h + 24000; ++n)
            if (std::abs (out.getSample (0, n)) > peak) { peak = std::abs (out.getSample (0, n)); peakIdx = n; }
        const double offsetMs = 1000.0 * (peakIdx - h) / sr;
        writeWav (outDir.getChildFile ("03_gap150.wav"), out, sr);
        std::printf ("[test2] latence=%d, montee=%.3f\n", lat, p.getLengthSeconds());
        for (int ms = -1800; ms <= 300; ms += 100) {
            double sum=0; int a = h + ms*48; for (int n=a;n<a+4800;++n) sum += out.getSample(0,n)*out.getSample(0,n);
            std::printf ("   %+5d ms : %.4f\n", ms, std::sqrt(sum/4800)); }
        std::printf ("[test2] gap 150 ms : pic a %+.1f ms du transient\n", offsetMs);
        ok &= offsetMs < -140.0 && offsetMs > -260.0;
    }

    // --- Démo à écouter : mix 50 %
    {
        SwellAudioProcessor p;
        setParam (p, ParamID::sync, 1.0f);
        setParam (p, ParamID::division, 3.0f);
        setParam (p, ParamID::mix, 0.5f);
        int lat = 0;
        auto out = render (p, input, sr, lat);
        writeWav (outDir.getChildFile ("02_demo_mix50.wav"), out, sr);
        float mx = out.getMagnitude (0, out.getNumSamples());
        std::printf ("[demo] peak = %.3f (%.1f dBFS)\n", mx, Decibels::gainToDecibels (mx));
        ok &= mx < 1.5f;
    }

    // --- Test 3 : rendu hors-ligne d'un clip (fin du clip = transient)
    {
        IRSettings st; st.lengthSamples = (int) sr; st.gapSamples = 0; st.shape = 0.55f; st.tone = 0.6f; st.lowCutHz = 180; st.width = 0.85f;
        AudioBuffer<float> one (2, (int) (1.0 * sr)); one.clear();
        const int t0 = (int) (0.3 * sr);
        for (int ch = 0; ch < 2; ++ch) one.copyFrom (ch, 0, input, ch, (int) (1.5 * sr) - t0, (int) sr - 0); // snare a 0.3 s
        auto res = SwellRenderer::render (one, sr, st, 0.0f);
        std::printf ("[test3] transient detecte a %.4f s (attendu 0.3000), clip %.3f s\n",
                     res.transientSample / sr, res.clip.getNumSamples() / sr);
        ok &= std::abs (res.transientSample - t0) < (int) (0.002 * sr);
        ok &= res.clip.getNumSamples() == (int) sr;
        // énergie : dernier 10 % >> premier 10 %
        const int L = res.clip.getNumSamples();
        auto rms = [&] (int a, int b) { double q = 0; for (int n = a; n < b; ++n) q += res.clip.getSample (0, n) * res.clip.getSample (0, n); return std::sqrt (q / (b - a)); };
        std::printf ("         rms debut=%.4f  fin=%.4f\n", rms (0, L / 10), rms (L - L / 10, L - 48));
        ok &= rms (L - L / 10, L - 48) > 10.0 * rms (0, L / 10);
        writeWav (outDir.getChildFile ("04_clip_render.wav"), res.clip, sr);

        // avec gate 1/16 a 120 bpm
        SwellRenderer::GateSettings gs; gs.rateBeats = 0.25; gs.depth = 1.0f; gs.smooth = 0.0f; gs.bpm = 120.0;
        auto rg = SwellRenderer::render (one, sr, st, 0.0f, gs);
        writeWav (outDir.getChildFile ("05_clip_gate16.wav"), rg.clip, sr);
        // 1/16 a 120 bpm = 125 ms ; la moitie fermee juste avant la derniere moitie ouverte doit etre ~silencieuse
        const int per = (int) (0.125 * sr);
        double closed = 0, open = 0;
        for (int n = L - per / 2 + 200; n < L - 200; ++n)       open   += std::abs (rg.clip.getSample (0, n));
        for (int n = L - per + 200;     n < L - per / 2 - 200; ++n) closed += std::abs (rg.clip.getSample (0, n));
        std::printf ("[test4] gate 1/16 : ouvert=%.3f ferme=%.6f\n", open, closed);
        ok &= closed < open * 0.01;
    }

    // --- Test 5 : capture depuis le flux audio puis rendu fichier
    {
        SwellAudioProcessor p;
        setParam (p, ParamID::sync, 1.0f);
        setParam (p, ParamID::division, 3.0f);
        p.setPlayConfigDetails (2, 2, sr, 512);
        p.prepareToPlay (sr, 512);
        p.toggleCapture();
        MidiBuffer m; AudioBuffer<float> b (2, 512);
        for (int pos = 0; pos + 512 <= input.getNumSamples(); pos += 512)
        {
            for (int ch = 0; ch < 2; ++ch) b.copyFrom (ch, 0, input, ch, pos, 512);
            p.processBlock (b, m);
        }
        p.toggleCapture();               // stop
        b.clear(); p.processBlock (b, m); // le thread audio clot la capture
        std::printf ("[test5] capture = %.2f s\n", p.getCaptureSeconds());
        p.finishCaptureNow();
        for (int i = 0; i < 100 && p.getRenderInfo().busy; ++i) Thread::sleep (50);
        auto info = p.getRenderInfo();
        std::printf ("         status: %s\n         fichier: %s\n", info.status.toRawUTF8(), info.file.getFullPathName().toRawUTF8());
        ok &= info.file.existsAsFile();
    }

    // --- Test 6 : re-rendu automatique quand on change le gate
    {
        SwellAudioProcessor p;
        p.setPlayConfigDetails (2, 2, sr, 512);
        p.prepareToPlay (sr, 512);
        p.renderFromBuffer (input, sr, "auto");
        for (int i = 0; i < 100 && p.getRenderInfo().busy; ++i) Thread::sleep (50);
        const auto first = p.getRenderInfo().file;
        setParam (p, ParamID::gate, 2.0f); // 1/8
        MessageManager::getInstance()->runDispatchLoopUntil (900);
        for (int i = 0; i < 100 && p.getRenderInfo().busy; ++i) Thread::sleep (50);
        const auto second = p.getRenderInfo().file;
        std::printf ("[test6] re-rendu auto : %s -> %s\n", first.getFileName().toRawUTF8(), second.getFileName().toRawUTF8());
        ok &= second.existsAsFile() && second != first;
    }

    // --- Capture de l'interface
    {
        SwellAudioProcessor p;
        p.setPlayConfigDetails (2, 2, sr, 512);
        p.prepareToPlay (sr, 512);
        setParam (p, ParamID::gate, 4.0f);
        setParam (p, ParamID::gateSmooth, 0.6f);
        p.renderFromBuffer (input, sr, "snare_demo");
        for (int i = 0; i < 100 && p.getRenderInfo().busy; ++i) Thread::sleep (50);
        std::unique_ptr<AudioProcessorEditor> ed (p.createEditor());
        ed->setBounds (0, 0, ed->getWidth(), ed->getHeight());
        MessageManager::getInstance()->runDispatchLoopUntil (400);
        auto snap = [&] (const char* name)
        {
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
            PNGImageFormat png; auto f = outDir.getChildFile (name); f.deleteFile();
            FileOutputStream os (f); png.writeImageToStream (img, os);
        };
        snap ("ui.png");
        auto* view = dynamic_cast<SwellMainView*> (ed->getChildComponent (0));
        view->showSettings();
        MessageManager::getInstance()->runDispatchLoopUntil (100);
        snap ("ui_settings.png");
        view->showHelp();
        MessageManager::getInstance()->runDispatchLoopUntil (100);
        snap ("ui_help.png");

        // Test 7 : pré-écoute -> le clip sort bien du plugin
        p.togglePreview();
        AudioBuffer<float> b (2, 512); MidiBuffer m; float mx = 0;
        for (int i = 0; i < 100; ++i) { b.clear(); p.processBlock (b, m); mx = jmax (mx, b.getMagnitude (0, 512)); }
        std::printf ("[test7] pre-ecoute : crete sortie = %.3f, lecture=%d\n", mx, (int) p.isPreviewPlaying());
        ok &= mx > 0.01f;

        // Test 8 : bypass -> plus de reverb, seulement le dry retardé
        setParam (p, ParamID::bypass, 1.0f);
        p.stopPreview();
        p.prepareToPlay (sr, 512);
        int lat = 0; auto outB = render (p, input, sr, lat);
        double diff = 0; for (int n = 0; n < input.getNumSamples(); ++n) diff += std::abs (outB.getSample (0, n) - input.getSample (0, n));
        std::printf ("[test8] bypass : ecart avec l'entree = %.5f\n", diff);
        ok &= diff < 1.0;
    }

    std::printf (ok ? "RESULT: PASS\n" : "RESULT: FAIL\n");
    return ok ? 0 : 1;
}
