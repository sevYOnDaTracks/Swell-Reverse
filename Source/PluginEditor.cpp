#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

static String fr (const char* utf8) { return String (CharPointer_UTF8 (utf8)); }

// ============================================================ Thème
namespace
{
    Colour gAccent { 0xffff7a3d };
}

Colour SwellColours::accent()  { return gAccent; }
Colour SwellColours::accent2() { return gAccent.interpolatedWith (Colours::white, 0.3f); }
void   SwellColours::setAccent (Colour c) { gAccent = c; }

const std::vector<SwellColours::Swatch>& SwellColours::swatches()
{
    static const std::vector<Swatch> s {
        { "Orange",  0xffff7a3d }, { "Rouge",   0xffef4444 }, { "Rose",    0xffec4899 },
        { "Violet",  0xff8b5cf6 }, { "Bleu",    0xff3b82f6 }, { "Cyan",    0xff22d3ee },
        { "Vert",    0xff22c55e }, { "Or",      0xfff5b301 }, { "Blanc",   0xffe8e6e1 },
    };
    return s;
}

PropertiesFile& SwellSettings::file()
{
    static PropertiesFile pf ([]
    {
        PropertiesFile::Options o;
        o.applicationName     = "Swell Reverse";
        o.filenameSuffix      = ".settings";
        o.folderName          = "Swell Reverse";
        o.osxLibrarySubFolder = "Application Support";
        o.millisecondsBeforeSaving = 0;   // sauvegarde immédiate
        return o;
    }());
    return pf;
}

// ============================================================ LookAndFeel
SwellLookAndFeel::SwellLookAndFeel() { refreshColours(); }

void SwellLookAndFeel::refreshColours()
{
    setColour (Slider::textBoxTextColourId,       SwellColours::text);
    setColour (Slider::textBoxOutlineColourId,    Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId,  SwellColours::accent().withAlpha (0.4f));
    setColour (Label::textColourId,               SwellColours::dim);
    setColour (ComboBox::backgroundColourId,      SwellColours::panel);
    setColour (ComboBox::textColourId,            SwellColours::text);
    setColour (ComboBox::outlineColourId,         SwellColours::track);
    setColour (ComboBox::arrowColourId,           SwellColours::accent());
    setColour (PopupMenu::backgroundColourId,     SwellColours::panel);
    setColour (PopupMenu::textColourId,           SwellColours::text);
    setColour (PopupMenu::headerTextColourId,     SwellColours::accent());
    setColour (PopupMenu::highlightedBackgroundColourId, SwellColours::accent());
    setColour (PopupMenu::highlightedTextColourId, SwellColours::bg);
    setColour (TextButton::buttonColourId,        SwellColours::panel);
    setColour (TextButton::textColourOffId,       SwellColours::text);
    setColour (TextButton::textColourOnId,        SwellColours::bg);
    setColour (AlertWindow::backgroundColourId,   SwellColours::panel);
    setColour (AlertWindow::textColourId,         SwellColours::text);
    setColour (AlertWindow::outlineColourId,      SwellColours::track);
    setColour (TextEditor::backgroundColourId,    SwellColours::bg);
    setColour (TextEditor::textColourId,          SwellColours::text);
    setColour (TextEditor::outlineColourId,       SwellColours::track);
    setColour (TextEditor::focusedOutlineColourId, SwellColours::accent());
    setColour (TextEditor::highlightColourId,     SwellColours::accent().withAlpha (0.35f));
    setColour (ScrollBar::thumbColourId,          SwellColours::track.brighter (0.2f));
    setColour (TooltipWindow::backgroundColourId, SwellColours::panel.brighter (0.06f));
    setColour (TooltipWindow::textColourId,       SwellColours::text);
    setColour (TooltipWindow::outlineColourId,    SwellColours::accent());
}

void SwellLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (5.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto  centre = bounds.getCentre();
    const float lineW  = jmax (3.0f, radius * 0.13f);
    const float arcR   = radius - lineW * 0.5f;
    const float angle  = startAngle + pos * (endAngle - startAngle);
    const float alpha  = s.isEnabled() ? 1.0f : 0.3f;
    const bool  hover  = s.isMouseOverOrDragging() && s.isEnabled();

    Path bgArc;
    bgArc.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (SwellColours::track.withAlpha (alpha));
    g.strokePath (bgArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));

    if (pos > 0.0f)
    {
        Path valArc;
        valArc.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setGradientFill (ColourGradient (SwellColours::accent2().withAlpha (alpha), bounds.getBottomLeft(),
                                           SwellColours::accent().withAlpha (alpha), bounds.getTopRight(), false));
        g.strokePath (valArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));
    }

    const float knobR = arcR - lineW * 1.4f;
    g.setColour (SwellColours::panel.brighter (hover ? 0.14f : 0.08f).withAlpha (alpha));
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);

    const Point<float> tip  (centre.x + (knobR - 4.0f) * std::sin (angle), centre.y - (knobR - 4.0f) * std::cos (angle));
    const Point<float> base (centre.x + knobR * 0.35f * std::sin (angle),  centre.y - knobR * 0.35f * std::cos (angle));
    g.setColour (SwellColours::text.withAlpha (alpha));
    g.drawLine ({ base, tip }, 2.5f);
}

void SwellLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const auto onColour = b.getProperties()["danger"] ? SwellColours::rec : SwellColours::accent();
    g.setColour (on ? onColour : SwellColours::panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? onColour : (highlighted ? SwellColours::dim : SwellColours::track));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
    g.setColour (on ? SwellColours::bg : SwellColours::text);
    g.setFont (FontOptions (12.5f, Font::bold));
    g.drawText (b.getButtonText(), r, Justification::centred);
}

void SwellLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour& colour, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto c = colour;
    if (down)             c = c.brighter (0.15f);
    else if (highlighted) c = c.brighter (0.07f);
    g.setColour (c.withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.4f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour ((colour == SwellColours::panel ? (highlighted ? SwellColours::dim : SwellColours::track) : colour)
                     .withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.4f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
}

Font SwellLookAndFeel::getTextButtonFont (TextButton&, int)
{
    return FontOptions (12.5f, Font::bold);
}

void SwellLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    const float alpha = box.isEnabled() ? 1.0f : 0.35f;
    g.setColour (SwellColours::panel.withAlpha (alpha));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour ((box.isMouseOver (true) ? SwellColours::dim : SwellColours::track).withAlpha (alpha));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);

    Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (SwellColours::accent().withAlpha (alpha));
    g.fillPath (arrow);
}

Rectangle<int> SwellLookAndFeel::getTooltipBounds (const String& tipText, Point<int> screenPos, Rectangle<int> parentArea)
{
    AttributedString s;
    s.append (tipText, FontOptions (13.0f), SwellColours::text);
    TextLayout tl;
    tl.createLayoutWithBalancedLineLengths (s, 260.0f);
    const int w = (int) tl.getWidth() + 20, h = (int) tl.getHeight() + 14;
    return Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                           screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6)  : screenPos.y + 6,
                           w, h).constrainedWithin (parentArea);
}

void SwellLookAndFeel::drawTooltip (Graphics& g, const String& text, int w, int h)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h);
    g.setColour (findColour (TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (SwellColours::accent().withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

    AttributedString s;
    s.setJustification (Justification::centredLeft);
    s.append (text, FontOptions (13.0f), SwellColours::text);
    TextLayout tl;
    tl.createLayoutWithBalancedLineLengths (s, 260.0f);
    tl.draw (g, r.reduced (10.0f, 7.0f));
}

// ============================================================ Courbe temps réel + vumètre
void SwellDisplay::refresh()
{
    env = proc.getDisplayEnvelope();
    repaint();
}

void SwellDisplay::setMeter (float l, float r)
{
    meterL = l; meterR = r;
    repaint (getLocalBounds().removeFromRight (34));
}

void SwellDisplay::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (SwellColours::panel);
    g.fillRoundedRectangle (r, 10.0f);

    // Vumètre de sortie (à droite)
    auto meterArea = r.removeFromRight (34.0f).reduced (8.0f, 14.0f);
    meterArea.removeFromBottom (14.0f);
    {
        auto drawBar = [&] (Rectangle<float> bar, float level)
        {
            g.setColour (SwellColours::track);
            g.fillRoundedRectangle (bar, 2.0f);
            const float db = Decibels::gainToDecibels (level, -60.0f);
            const float norm = jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            auto fillBar = bar.withTop (bar.getBottom() - bar.getHeight() * norm);
            g.setColour (level > 0.98f ? SwellColours::rec : SwellColours::accent());
            g.fillRoundedRectangle (fillBar, 2.0f);
        };
        auto left = meterArea.removeFromLeft (meterArea.getWidth() * 0.5f - 1.0f);
        meterArea.removeFromLeft (2.0f);
        drawBar (left, meterL);
        drawBar (meterArea, meterR);
        g.setColour (SwellColours::dim);
        g.setFont (FontOptions (9.0f));
        g.drawText ("OUT", Rectangle<float> (left.getX() - 6.0f, left.getBottom() + 2.0f, 30.0f, 12.0f), Justification::centred);
    }

    auto plot = r.reduced (16.0f, 12.0f).withTrimmedRight (-6.0f);
    plot.removeFromBottom (14.0f);

    g.setColour (SwellColours::track.withAlpha (0.5f));
    for (int i = 1; i < 4; ++i)
        g.drawVerticalLine ((int) (plot.getX() + plot.getWidth() * (float) i / 4.0f), plot.getY(), plot.getBottom());

    const float gapRatio = jlimit (0.0f, 0.3f, proc.getLatencyMs() > 0.0f
        ? 1.0f - proc.getLengthSeconds() * 1000.0f / proc.getLatencyMs() : 0.0f);
    auto swellArea = plot.withTrimmedRight (plot.getWidth() * (0.10f + gapRatio));
    const float transientX = plot.getRight() - plot.getWidth() * 0.08f;

    if (env.size() > 1)
    {
        SwellRenderer::GateSettings gs;
        {
            static const double beats[] = { 0.0, 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
            const int idx = (int) proc.apvts.getRawParameterValue (ParamID::gate)->load();
            gs.rateBeats = beats[jlimit (0, 6, idx)];
            gs.depth  = proc.apvts.getRawParameterValue (ParamID::gateDepth)->load();
            gs.smooth = proc.apvts.getRawParameterValue (ParamID::gateSmooth)->load();
            gs.bpm    = proc.getHostBpm();
        }
        const double swellBeats = proc.getLengthSeconds() * gs.bpm / 60.0;

        Path fill, line;
        const auto n = env.size();
        fill.startNewSubPath (swellArea.getX(), swellArea.getBottom());
        for (size_t i = 0; i < n; ++i)
        {
            const float t = (float) i / (float) (n - 1);
            const float x = swellArea.getX() + swellArea.getWidth() * t;
            float v = std::sqrt (jlimit (0.0f, 1.0f, env[i]));
            if (gs.rateBeats > 0.0)
                v *= SwellRenderer::gateGain (0.5 - (1.0 - t) * swellBeats / gs.rateBeats, gs.depth, gs.smooth, 0.05);
            const float y = swellArea.getBottom() - v * swellArea.getHeight();
            fill.lineTo (x, y);
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
        }
        fill.lineTo (swellArea.getRight(), swellArea.getBottom());
        fill.closeSubPath();

        g.setGradientFill (ColourGradient (SwellColours::accent().withAlpha (0.55f), swellArea.getRight(), swellArea.getY(),
                                           SwellColours::accent().withAlpha (0.02f), swellArea.getX(), swellArea.getBottom(), false));
        g.fillPath (fill);
        g.setColour (SwellColours::accent2());
        g.strokePath (line, PathStrokeType (1.6f));
    }

    g.setColour (SwellColours::text);
    g.fillRect (transientX - 1.5f, plot.getY(), 3.0f, plot.getHeight());
    g.setFont (FontOptions (11.0f));
    g.setColour (SwellColours::dim);
    g.drawText ("TRANSIENT", Rectangle<float> (transientX - 40.0f, plot.getBottom() + 1.0f, 80.0f, 13.0f), Justification::centred);
    g.drawText (fr ("MONTÉE"), Rectangle<float> (swellArea.getX(), plot.getBottom() + 1.0f, 80.0f, 13.0f), Justification::left);
}

// ============================================================ Tuile glissable
void DragTile::setClip (const File& f, std::vector<float> envelope)
{
    file = f;
    env = std::move (envelope);
    repaint();
}

void DragTile::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool ready = file.existsAsFile();
    const bool hover = isMouseOver() && ready;

    g.setColour (SwellColours::bg);
    g.fillRoundedRectangle (r, 8.0f);

    auto inner = r.reduced (10.0f, 8.0f);
    auto label = inner.removeFromBottom (16.0f);

    if (ready && env.size() > 1)
    {
        Path p;
        const auto n = env.size();
        const float mid = inner.getCentreY(), half = inner.getHeight() * 0.48f;
        p.startNewSubPath (inner.getX(), mid);
        for (size_t i = 0; i < n; ++i)
            p.lineTo (inner.getX() + inner.getWidth() * (float) i / (float) (n - 1),
                      mid - std::sqrt (jlimit (0.0f, 1.0f, env[i])) * half);
        for (size_t i = n; i-- > 0;)
            p.lineTo (inner.getX() + inner.getWidth() * (float) i / (float) (n - 1),
                      mid + std::sqrt (jlimit (0.0f, 1.0f, env[i])) * half);
        p.closeSubPath();
        g.setColour (SwellColours::accent().withAlpha (hover ? 0.95f : 0.75f));
        g.fillPath (p);

        // tête de lecture de la pré-écoute (la partie clip occupe toute la tuile)
        if (playhead >= 0.0f && clipFraction > 0.0f)
        {
            const float pos = playhead / clipFraction;
            if (pos <= 1.0f)
            {
                const float x = inner.getX() + inner.getWidth() * pos;
                g.setColour (SwellColours::text);
                g.fillRect (x - 1.0f, inner.getY(), 2.0f, inner.getHeight());
            }
            else
            {
                g.setColour (SwellColours::text.withAlpha (0.8f));
                g.setFont (FontOptions (11.0f, Font::bold));
                g.drawText (fr ("SON ORIGINAL >"), inner.removeFromRight (120.0f).removeFromTop (14.0f), Justification::centredRight);
            }
        }

        g.setColour (SwellColours::text);
        g.setFont (FontOptions (12.0f, Font::bold));
        g.drawText ("GLISSE DANS LA PLAYLIST", label, Justification::centredLeft);
        g.setColour (SwellColours::dim);
        g.setFont (FontOptions (11.0f));
        g.drawText (file.getFileName(), label.withTrimmedLeft (190.0f), Justification::centredRight);
    }
    else
    {
        g.setColour (SwellColours::dim);
        g.setFont (FontOptions (13.0f));
        g.drawFittedText (fr ("Glisse un sample ici, ou CAPTURE la piste\npuis glisse le clip rendu dans la playlist"),
                          inner.withTrimmedBottom (-16.0f).toNearestInt(), Justification::centred, 2);
    }

    g.setColour (highlight ? SwellColours::accent() : (hover ? SwellColours::accent2() : SwellColours::track));
    g.drawRoundedRectangle (r, 8.0f, highlight ? 2.0f : 1.2f);
}

void DragTile::mouseDown (const MouseEvent&) { dragging = false; }

void DragTile::mouseDrag (const MouseEvent& e)
{
    if (dragging || ! file.existsAsFile() || e.getDistanceFromDragStart() < 6)
        return;
    dragging = true;
    DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

// ============================================================ Overlays
OverlayPanel::OverlayPanel (const String& t) : title (t)
{
    closeButton.onClick = [this] { if (onClose) onClose(); };
    closeButton.setTooltip ("Fermer");
    addAndMakeVisible (closeButton);
    setWantsKeyboardFocus (true);
}

void OverlayPanel::mouseDown (const MouseEvent& e)
{
    // clic en dehors de la carte = fermer
    if (! card.contains (e.getPosition()) && onClose)
        onClose();
}

void OverlayPanel::paint (Graphics& g)
{
    g.fillAll (Colours::black.withAlpha (0.6f));
    g.setColour (SwellColours::panel);
    g.fillRoundedRectangle (card.toFloat(), 12.0f);
    g.setColour (SwellColours::accent().withAlpha (0.6f));
    g.drawRoundedRectangle (card.toFloat().reduced (0.5f), 12.0f, 1.2f);

    g.setColour (SwellColours::text);
    g.setFont (FontOptions (18.0f, Font::bold));
    g.drawText (title, card.reduced (22, 0).removeFromTop (54), Justification::centredLeft);
}

void OverlayPanel::resized()
{
    card = getLocalBounds().reduced (50, 40);
    if (cardHeight > 0)
        card = card.withSizeKeepingCentre (card.getWidth(), jmin (card.getHeight(), cardHeight));
    closeButton.setBounds (card.getRight() - 46, card.getY() + 14, 30, 28);
    content = card.reduced (22, 0).withTrimmedTop (56).withTrimmedBottom (20);
    layoutContent (content);
}

HelpOverlay::HelpOverlay() : OverlayPanel (fr ("AIDE  —  Comment utiliser SWELL"))
{
    text.setMultiLine (true, true);
    text.setReadOnly (true);
    text.setCaretVisible (false);
    text.setScrollbarsShown (true);
    text.setColour (TextEditor::backgroundColourId, SwellColours::bg);
    text.setColour (TextEditor::outlineColourId, Colours::transparentBlack);
    text.setFont (FontOptions (14.0f));
    text.setBorder (BorderSize<int> (12));
    const String help = (fr (
        "LE PRINCIPE\n"
        "Swell crée une reverb inversée : la reverb monte AVANT le son et s'arrête pile sur son transient "
        "(l'attaque). Deux façons de l'utiliser :\n"
        "  1. En temps réel sur une piste du mixer (FL compense la latence automatiquement).\n"
        "  2. En mode RENDER : Swell fabrique un clip audio que tu glisses dans la playlist.\n\n"

        "MODE RENDER → PLAYLIST (recommandé)\n"
        "  1. CAPTURE puis lance la lecture : la capture démarre au premier son et s'arrête quand tu stoppes "
        "(ou clique sur STOP).  Ou glisse un sample sur le plugin / bouton CHARGER.\n"
        "  2. Swell détecte le transient et calcule le clip. Tourne les boutons : le clip se recalcule tout seul.\n"
        "  3. PLAY pour pré-écouter. Avec « + SON ORIGINAL », tu entends la montée suivie du son.\n"
        "  4. Glisse la tuile dans la playlist et place la FIN du clip juste au début de ton son.\n"
        "     En mode SYNC, la durée du clip tombe pile sur la grille.\n"
        "  DOSSIER ouvre le dossier où tous les clips sont enregistrés (Musique/Swell Reverse).\n\n"

        "DURÉE DE LA MONTÉE\n"
        "  SYNC + division : durée calée sur le tempo (1/16 à 2 mesures).\n"
        "  LENGTH : durée libre en secondes (quand SYNC est coupé).\n\n"

        "COULEUR DU SON\n"
        "  SHAPE : 0 % = montée longue et progressive, 100 % = montée tardive qui « aspire ».\n"
        "  TONE : sombre ↔ brillant.\n"
        "  LOW CUT : retire le grave de la reverb pour garder le mix propre.\n"
        "  WIDTH : mono ↔ stéréo large.\n"
        "  GAP : petit silence entre la fin de la montée et le transient.\n\n"

        "GATE (optionnel)\n"
        "  GATE : hache la montée en rythme (1/4 à 1/32, triolets).  Off = montée continue.\n"
        "  GATE DEPTH : intensité du hachage.\n"
        "  GATE SMOOTH : 0 % = découpe sèche (stutter), 100 % = vague douce « wou-wou ».\n"
        "  Le gate est toujours ouvert au moment du transient.\n\n"

        "SORTIE\n"
        "  MIX : 0 % = son seul, 50 % = son + reverb à fond, 100 % = reverb seule (idéal en send).\n"
        "  OUTPUT : volume de sortie.  BYPASS : coupe l'effet sans changer la latence.\n\n"

        "PRESETS\n"
        "  Menu en haut : presets d'usine et tes presets.  < > pour passer au suivant.\n"
        "  SAVE enregistre tes réglages (Musique/Swell Reverse/Presets).\n\n"

        "ASTUCES\n"
        "  • Double-clic sur un bouton = valeur par défaut.  Cmd + glisser = réglage fin.\n"
        "  • Survole un élément pour voir son infobulle (activable dans SETTINGS).\n"
        "  • En temps réel, si tu changes la durée ou le tempo, relance la lecture pour que FL se recale.\n"
        "  • SETTINGS : couleur de l'interface, taille de la fenêtre, infobulles.\n"));

    // Titres en gras et en couleur, texte normal ensuite
    for (auto& line : StringArray::fromLines (help))
    {
        const auto firstWord = line.upToFirstOccurrenceOf (" ", false, false);
        const bool heading = firstWord.length() >= 3 && ! line.startsWith (" ") && firstWord == firstWord.toUpperCase();
        text.setFont (heading ? FontOptions (14.5f, Font::bold) : FontOptions (14.0f));
        text.setColour (TextEditor::textColourId, heading ? SwellColours::accent() : SwellColours::text);
        text.insertTextAtCaret (line + "\n");
    }
    text.moveCaretToTop (false);
    addAndMakeVisible (text);
}

void HelpOverlay::layoutContent (Rectangle<int> area)
{
    text.setBounds (area);
}

SettingsOverlay::SettingsOverlay() : OverlayPanel ("SETTINGS")
{
    setCardHeight (430);
    for (auto& s : SwellColours::swatches())
    {
        auto* b = swatchButtons.add (new TextButton());
        const Colour c (s.argb);
        b->setTooltip (fr (s.name));
        b->setColour (TextButton::buttonColourId, c);
        b->onClick = [this, c] { currentAccent = c; if (onAccent) onAccent (c); repaint(); };
        addAndMakeVisible (b);
    }
    for (float s : { 0.75f, 1.0f, 1.25f, 1.5f })
    {
        auto* b = scaleButtons.add (new TextButton (String (roundToInt (s * 100)) + " %"));
        b->setClickingTogglesState (false);
        b->onClick = [this, s] { currentScale = s; if (onScale) onScale (s); repaint(); };
        addAndMakeVisible (b);
    }
    tooltipToggle.onClick = [this] { if (onTooltips) onTooltips (tooltipToggle.getToggleState()); };
    addAndMakeVisible (tooltipToggle);

    folderButton.onClick = []
    {
        auto f = SwellAudioProcessor::getRenderFolder();
        f.createDirectory();
        f.revealToUser();
    };
    presetFolderButton.onClick = []
    {
        auto f = SwellAudioProcessor::getPresetFolder();
        f.createDirectory();
        f.revealToUser();
    };
    addAndMakeVisible (folderButton);
    addAndMakeVisible (presetFolderButton);
}

void SettingsOverlay::setCurrent (Colour a, float s, bool tips)
{
    currentAccent = a;
    currentScale = s;
    tooltipToggle.setToggleState (tips, dontSendNotification);
    repaint();
}

void SettingsOverlay::layoutContent (Rectangle<int> area)
{
    colourLabel = area.removeFromTop (22);
    auto row = area.removeFromTop (40);
    const int sw = 40;
    for (auto* b : swatchButtons)
    {
        b->setBounds (row.removeFromLeft (sw));
        row.removeFromLeft (10);
    }
    area.removeFromTop (22);

    scaleLabel = area.removeFromTop (22);
    row = area.removeFromTop (34);
    for (auto* b : scaleButtons)
    {
        b->setBounds (row.removeFromLeft (84));
        row.removeFromLeft (10);
    }
    area.removeFromTop (22);

    otherLabel = area.removeFromTop (22);
    tooltipToggle.setBounds (area.removeFromTop (34).removeFromLeft (150));
    area.removeFromTop (12);
    row = area.removeFromTop (34);
    folderButton.setBounds (row.removeFromLeft (250));
    row.removeFromLeft (10);
    presetFolderButton.setBounds (row.removeFromLeft (270));
}

void SettingsOverlay::paint (Graphics& g)
{
    OverlayPanel::paint (g);
    g.setColour (SwellColours::dim);
    g.setFont (FontOptions (12.0f, Font::bold));
    g.drawText (fr ("COULEUR DE L'INTERFACE"), colourLabel, Justification::centredLeft);
    g.drawText (fr ("TAILLE DE LA FENÊTRE"), scaleLabel, Justification::centredLeft);
    g.drawText ("AUTRES", otherLabel, Justification::centredLeft);

    // repère sur la couleur et la taille actives
    for (auto* b : swatchButtons)
        if (b->findColour (TextButton::buttonColourId) == currentAccent)
        {
            g.setColour (SwellColours::text);
            g.drawRoundedRectangle (b->getBounds().toFloat().expanded (3.0f), 8.0f, 2.0f);
        }
    for (int i = 0; i < scaleButtons.size(); ++i)
        if (std::abs (currentScale - std::array<float, 4> { 0.75f, 1.0f, 1.25f, 1.5f }[(size_t) i]) < 0.01f)
        {
            g.setColour (SwellColours::accent());
            g.drawRoundedRectangle (scaleButtons[i]->getBounds().toFloat().expanded (2.0f), 8.0f, 2.0f);
        }

    // signature
    auto sig = getContentArea().removeFromBottom (20);
    g.setFont (FontOptions (12.0f));
    g.setColour (SwellColours::dim);
    g.drawText ("SWELL Reverse Reverb  v" + String (SWELL_VERSION) + "  |  by @sevyondatracks", sig, Justification::centredLeft);
}

// ============================================================ Vue principale
SwellMainView::SwellMainView (SwellAudioProcessor& p, SwellLookAndFeel& l)
    : proc (p), lnf (l), display (p)
{
    logoS = ImageCache::getFromMemory (BinaryData::logo_s_png, BinaryData::logo_s_pngSize);

    addAndMakeVisible (display);

    setupKnob (length,     ParamID::length,     "LENGTH",      fr ("Durée libre de la montée (quand SYNC est coupé)."));
    setupKnob (shape,      ParamID::shape,      "SHAPE",       fr ("Forme de la montée : progressive (bas) ou tardive qui aspire (haut)."));
    setupKnob (tone,       ParamID::tone,       "TONE",        fr ("Couleur de la reverb : sombre ↔ brillante."));
    setupKnob (lowCut,     ParamID::lowCut,     "LOW CUT",     fr ("Retire le grave de la reverb pour garder le mix propre."));
    setupKnob (width,      ParamID::width,      "WIDTH",       fr ("Largeur stéréo de la reverb."));
    setupKnob (gap,        ParamID::gap,        "GAP",         fr ("Silence entre la fin de la montée et le transient."));
    setupKnob (gateDepth,  ParamID::gateDepth,  "GATE DEPTH",  fr ("Intensité du gate (nécessite GATE activé)."));
    setupKnob (gateSmooth, ParamID::gateSmooth, "GATE SMOOTH", fr ("0 % = découpe sèche, 100 % = vague douce « wou-wou »."));
    setupKnob (mix,        ParamID::mix,        "MIX",         fr ("0 % = son seul, 50 % = son + reverb, 100 % = reverb seule."));
    setupKnob (output,     ParamID::output,     "OUTPUT",      fr ("Volume de sortie (s'applique aussi aux clips rendus)."));

    // Presets
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.setTooltip (fr ("Presets d'usine et tes presets."));
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id <= 0) return;
        if (id < 1000) proc.loadFactoryPreset (id - 1);
        else           proc.loadUserPreset (presetBox.getText());
        lastPresetName = proc.getCurrentPresetName();
    };
    addAndMakeVisible (presetBox);
    prevPreset.onClick = [this] { stepPreset (-1); };
    nextPreset.onClick = [this] { stepPreset (+1); };
    prevPreset.setTooltip (fr ("Preset précédent"));
    nextPreset.setTooltip ("Preset suivant");
    addAndMakeVisible (prevPreset);
    addAndMakeVisible (nextPreset);
    saveButton.onClick = [this] { savePresetDialog(); };
    saveButton.setTooltip ("Sauvegarder les réglages actuels comme preset.");
    addAndMakeVisible (saveButton);
    refreshPresetList();

    helpButton.onClick = [this] { showHelp(); };
    helpButton.setTooltip (fr ("Aide : à quoi sert chaque élément."));
    addAndMakeVisible (helpButton);
    settingsButton.onClick = [this] { showSettings(); };
    settingsButton.setTooltip (fr ("Couleur, taille de la fenêtre, infobulles."));
    addAndMakeVisible (settingsButton);

    // Gate
    gateLabel.setText ("GATE", dontSendNotification);
    gateLabel.setFont (FontOptions (12.0f, Font::bold));
    gateLabel.setJustificationType (Justification::centredRight);
    addAndMakeVisible (gateLabel);
    if (auto* choice = dynamic_cast<AudioParameterChoice*> (p.apvts.getParameter (ParamID::gate)))
        gateBox.addItemList (choice->choices, 1);
    gateBox.setJustificationType (Justification::centred);
    gateBox.setTooltip (fr ("Gate rythmique sur la montée (Off = montée continue)."));
    addAndMakeVisible (gateBox);
    gateAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, ParamID::gate, gateBox);
    gateBox.onChange = [this] { updateEnablement(); display.repaint(); };

    // Bypass
    bypassButton.getProperties().set ("danger", true);
    bypassButton.setTooltip (fr ("Coupe l'effet (la latence reste identique)."));
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ParamID::bypass, bypassButton);

    // Sync
    syncButton.setTooltip (fr ("Durée de la montée calée sur le tempo de FL."));
    addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ParamID::sync, syncButton);
    syncButton.onStateChange = [this] { updateEnablement(); };

    if (auto* choice = dynamic_cast<AudioParameterChoice*> (p.apvts.getParameter (ParamID::division)))
        divisionBox.addItemList (choice->choices, 1);
    divisionBox.setJustificationType (Justification::centred);
    divisionBox.setTooltip (fr ("Durée de la montée en valeur rythmique."));
    addAndMakeVisible (divisionBox);
    divisionAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, ParamID::division, divisionBox);

    infoLabel.setJustificationType (Justification::centredRight);
    infoLabel.setFont (FontOptions (12.0f));
    addAndMakeVisible (infoLabel);

    // Render
    captureButton.onClick = [this] { proc.toggleCapture(); };
    captureButton.setTooltip (fr ("Arme la capture puis lance la lecture : enregistre le son qui entre dans Swell."));
    loadButton.onClick = [this] { chooseFile(); };
    loadButton.setTooltip (fr ("Charger un sample (ou glisse-le directement sur le plugin)."));
    playButton.onClick = [this] { proc.togglePreview(); };
    playButton.setTooltip (fr ("Pré-écouter le clip rendu."));
    folderButton.onClick = []
    {
        auto f = SwellAudioProcessor::getRenderFolder();
        f.createDirectory();
        f.revealToUser();
    };
    folderButton.setTooltip (fr ("Ouvre le dossier où les clips sont enregistrés."));
    withSourceButton.setToggleState (proc.getPreviewWithSource(), dontSendNotification);
    withSourceButton.onClick = [this] { proc.setPreviewWithSource (withSourceButton.getToggleState()); };
    withSourceButton.setTooltip (fr ("Pré-écoute : enchaîne le son original après la montée."));
    for (auto* c : std::initializer_list<Component*> { &captureButton, &loadButton, &playButton, &folderButton, &withSourceButton, &tile })
        addAndMakeVisible (c);
    tile.setTooltip (fr ("Glisse cette tuile dans la playlist de FL Studio."));
    renderStatus.setFont (FontOptions (12.0f));
    renderStatus.setJustificationType (Justification::centredLeft);
    addAndMakeVisible (renderStatus);

    updateEnablement();
    display.refresh();
    setSize (designW, designH);
    startTimerHz (30);
}

SwellMainView::~SwellMainView()
{
    stopTimer();
}

void SwellMainView::setupKnob (Knob& k, const String& paramId, const String& name, const String& tip)
{
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 18);
    k.slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    k.slider.setColour (Slider::textBoxTextColourId, SwellColours::text);
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    k.slider.setTooltip (tip);
    addAndMakeVisible (k.slider);

    k.label.setText (name, dontSendNotification);
    k.label.setJustificationType (Justification::centred);
    k.label.setFont (FontOptions (11.5f, Font::bold));
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
    if (auto* param = proc.apvts.getParameter (paramId))
        k.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    k.slider.onValueChange = [this] { display.repaint(); };
}

void SwellMainView::updateEnablement()
{
    const bool sync = syncButton.getToggleState();
    length.slider.setEnabled (! sync);
    length.label.setAlpha (sync ? 0.4f : 1.0f);
    divisionBox.setEnabled (sync);

    const bool gateOn = gateBox.getSelectedItemIndex() > 0;
    for (auto* k : { &gateDepth, &gateSmooth })
    {
        k->slider.setEnabled (gateOn);
        k->label.setAlpha (gateOn ? 1.0f : 0.4f);
    }
}

void SwellMainView::refreshPresetList()
{
    presetBox.clear (dontSendNotification);
    presetBox.addSectionHeading ("USINE");
    auto factory = proc.getFactoryPresetNames();
    for (int i = 0; i < factory.size(); ++i)
        presetBox.addItem (factory[i], i + 1);

    auto user = proc.getUserPresetNames();
    if (! user.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("MES PRESETS");
        for (int i = 0; i < user.size(); ++i)
            presetBox.addItem (user[i], 1000 + i);
    }
    lastPresetName = proc.getCurrentPresetName();
    presetBox.setText (lastPresetName, dontSendNotification);
}

void SwellMainView::stepPreset (int delta)
{
    const int n = presetBox.getNumItems();
    if (n == 0) return;
    int idx = presetBox.getSelectedItemIndex();
    idx = idx < 0 ? 0 : (idx + delta + n) % n;
    presetBox.setSelectedItemIndex (idx, sendNotificationSync);
}

void SwellMainView::savePresetDialog()
{
    auto* w = new AlertWindow (fr ("Sauvegarder le preset"), fr ("Nom du preset :"), MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf);
    w->addTextEditor ("name", proc.getCurrentPresetName() == "Init" ? String() : proc.getCurrentPresetName());
    w->addButton ("SAUVER", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("ANNULER", 0, KeyPress (KeyPress::escapeKey));

    Component::SafePointer<SwellMainView> safe (this);
    w->enterModalState (true, ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result != 1 || safe == nullptr)
            return;
        if (safe->proc.saveUserPreset (w->getTextEditorContents ("name")))
            safe->refreshPresetList();
    }), true);
}

void SwellMainView::chooseFile()
{
    chooser = std::make_unique<FileChooser> (fr ("Choisis un sample"), File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.m4a;*.ogg");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [this] (const FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f.existsAsFile())
                                  proc.renderFromFile (f);
                          });
}

void SwellMainView::showHelp()
{
    help = std::make_unique<HelpOverlay>();
    help->onClose = [this] { MessageManager::callAsync ([sp = Component::SafePointer<SwellMainView> (this)] { if (sp) sp->help.reset(); }); };
    addAndMakeVisible (*help);
    help->setBounds (getLocalBounds());
}

void SwellMainView::showSettings()
{
    settings = std::make_unique<SettingsOverlay>();
    auto& pf = SwellSettings::file();
    settings->setCurrent (SwellColours::accent(), (float) pf.getDoubleValue ("uiScale", 1.0), pf.getBoolValue ("tooltips", true));
    settings->onAccent   = [this] (Colour c) { applyAccent (c); };
    settings->onScale    = [this] (float s) { if (onScaleChange) onScaleChange (s); };
    settings->onTooltips = [this] (bool b) { if (onTooltipsChange) onTooltipsChange (b); };
    settings->onClose = [this] { MessageManager::callAsync ([sp = Component::SafePointer<SwellMainView> (this)] { if (sp) sp->settings.reset(); }); };
    addAndMakeVisible (*settings);
    settings->setBounds (getLocalBounds());
}

void SwellMainView::applyAccent (Colour c)
{
    SwellColours::setAccent (c);
    SwellSettings::file().setValue ("accent", c.toString());
    lnf.refreshColours();
    // force la mise à jour de tous les composants
    std::function<void (Component&)> refresh = [&] (Component& comp)
    {
        comp.sendLookAndFeelChange();
        comp.repaint();
    };
    if (auto* top = getTopLevelComponent())
        refresh (*top);
    else
        refresh (*this);
}

bool SwellMainView::isInterestedInFileDrag (const StringArray& files)
{
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;m4a;ogg"))
            return true;
    return false;
}

void SwellMainView::filesDropped (const StringArray& files, int, int)
{
    tile.setHighlight (false);
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;m4a;ogg"))
        {
            proc.renderFromFile (File (f));
            break;
        }
}

void SwellMainView::timerCallback()
{
    if (const int v = proc.getDisplayVersion(); v != lastDisplayVersion)
    {
        lastDisplayVersion = v;
        display.refresh();
    }

    if (const int v = proc.getRenderVersion(); v != lastRenderVersion)
    {
        lastRenderVersion = v;
        auto info = proc.getRenderInfo();
        renderStatus.setText (info.status, dontSendNotification);
        tile.setClip (info.file, info.envelope);
        playButton.setEnabled (info.file.existsAsFile());
    }

    if (proc.getCurrentPresetName() != lastPresetName)
    {
        lastPresetName = proc.getCurrentPresetName();
        presetBox.setText (lastPresetName, dontSendNotification);
    }

    // Vumètre avec retombée douce
    meterL = jmax (proc.getOutputPeak (0), meterL * 0.85f);
    meterR = jmax (proc.getOutputPeak (1), meterR * 0.85f);
    display.setMeter (meterL, meterR);

    // Pré-écoute
    const bool playing = proc.isPreviewPlaying();
    playButton.setButtonText (playing ? "STOP" : "PLAY");
    playButton.setColour (TextButton::buttonColourId, playing ? SwellColours::accent() : SwellColours::panel);
    playButton.setColour (TextButton::textColourOffId, playing ? SwellColours::bg : SwellColours::text);
    tile.setPlayhead (playing ? proc.getPreviewProgress() : -1.0f, proc.getPreviewClipFraction());

    // Capture
    ++blink;
    switch (proc.getCaptureState())
    {
        case SwellAudioProcessor::CaptureState::idle:
            captureButton.setButtonText ("CAPTURE");
            captureButton.setColour (TextButton::buttonColourId, SwellColours::panel);
            break;
        case SwellAudioProcessor::CaptureState::armed:
            captureButton.setButtonText (fr ("ARMÉ…"));
            captureButton.setColour (TextButton::buttonColourId,
                                     (blink / 10) % 2 ? SwellColours::rec.withAlpha (0.6f) : SwellColours::panel);
            break;
        case SwellAudioProcessor::CaptureState::recording:
            captureButton.setButtonText ("STOP  " + String (proc.getCaptureSeconds(), 1) + " s");
            captureButton.setColour (TextButton::buttonColourId, SwellColours::rec);
            break;
    }

    if (blink % 6 == 0)
    {
        String info;
        info << fr ("Montée ") << String (proc.getLengthSeconds(), 2) << " s";
        if (syncButton.getToggleState() || gateBox.getSelectedItemIndex() > 0)
            info << "  |  " << String (proc.getHostBpm(), 1) << " BPM";
        info << "  |  Latence " << String (roundToInt (proc.getLatencyMs())) << " ms";
        infoLabel.setText (info, dontSendNotification);
    }
}

void SwellMainView::paint (Graphics& g)
{
    g.fillAll (SwellColours::bg);

    // En-tête : logo S + nom du plugin
    auto header = getLocalBounds().removeFromTop (64).reduced (22, 0);
    if (logoS.isValid())
    {
        g.setOpacity (1.0f);
        g.drawImage (logoS, header.removeFromLeft (44).withSizeKeepingCentre (40, 36).toFloat(), RectanglePlacement::centred);
        header.removeFromLeft (10);
    }
    auto titleArea = header.removeFromLeft (150).withSizeKeepingCentre (150, 48);
    g.setColour (SwellColours::text);
    g.setFont (FontOptions (28.0f, Font::bold));
    g.drawText ("SWELL", titleArea.removeFromTop (30), Justification::bottomLeft);
    g.setColour (SwellColours::accent());
    g.setFont (FontOptions (10.5f, Font::bold));
    g.drawText ("R E V E R S E   R E V E R B", titleArea, Justification::centredLeft);

    // Panneau Render
    if (! renderPanelArea.isEmpty())
    {
        g.setColour (SwellColours::panel);
        g.fillRoundedRectangle (renderPanelArea.toFloat(), 10.0f);
        g.setColour (SwellColours::accent());
        g.setFont (FontOptions (12.0f, Font::bold));
        g.drawText (fr ("RENDER  →  PLAYLIST"), renderPanelArea.reduced (14, 8).removeFromTop (16), Justification::centredLeft);
    }

    // Pied de page : version + by @sevyondatracks + logo (en bas à droite)
    if (! footerArea.isEmpty())
    {
        auto f = footerArea;
        if (logoS.isValid())
        {
            g.setOpacity (0.9f);
            g.drawImage (logoS, f.removeFromRight (22).withSizeKeepingCentre (18, 16).toFloat(), RectanglePlacement::centred);
            f.removeFromRight (6);
        }
        g.setColour (SwellColours::dim);
        g.setFont (FontOptions (12.0f));
        g.drawText ("v" + String (SWELL_VERSION) + "   |   by @sevyondatracks", f, Justification::centredRight);
    }
}

void SwellMainView::resized()
{
    auto area = getLocalBounds().reduced (20, 0);

    // En-tête : presets + aide + settings à droite
    auto header = area.removeFromTop (64);
    auto pr = header.removeFromRight (420).withSizeKeepingCentre (420, 30);
    settingsButton.setBounds (pr.removeFromRight (82));
    pr.removeFromRight (6);
    helpButton.setBounds (pr.removeFromRight (30));
    pr.removeFromRight (10);
    saveButton.setBounds (pr.removeFromRight (56));
    pr.removeFromRight (6);
    nextPreset.setBounds (pr.removeFromRight (26));
    pr.removeFromRight (4);
    prevPreset.setBounds (pr.removeFromLeft (26));
    pr.removeFromLeft (4);
    presetBox.setBounds (pr);

    // Barre : gate à gauche, bypass + sync à droite
    auto bar = area.removeFromTop (32);
    gateLabel.setBounds (bar.removeFromLeft (44));
    bar.removeFromLeft (8);
    gateBox.setBounds (bar.removeFromLeft (110));
    auto right = bar.removeFromRight (300);
    bypassButton.setBounds (right.removeFromLeft (80));
    right.removeFromLeft (10);
    syncButton.setBounds (right.removeFromLeft (76));
    right.removeFromLeft (10);
    divisionBox.setBounds (right);

    area.removeFromTop (10);
    display.setBounds (area.removeFromTop (118));
    infoLabel.setBounds (area.removeFromTop (22).withTrimmedRight (4));

    // Pied de page et panneau Render (depuis le bas)
    footerArea = area.removeFromBottom (30).withTrimmedRight (16);
    renderPanelArea = area.removeFromBottom (176);
    area.removeFromBottom (10);

    {
        auto rp = renderPanelArea.reduced (14, 10);
        rp.removeFromTop (22);
        renderStatus.setBounds (rp.removeFromBottom (20));
        rp.removeFromBottom (4);
        auto col = rp.removeFromLeft (220);
        const int bh = (col.getHeight() - 12) / 3;
        auto r1 = col.removeFromTop (bh); col.removeFromTop (6);
        auto r2 = col.removeFromTop (bh); col.removeFromTop (6);
        auto r3 = col.removeFromTop (bh);
        captureButton.setBounds (r1.removeFromLeft (107)); r1.removeFromLeft (6); loadButton.setBounds (r1);
        playButton.setBounds (r2.removeFromLeft (107));    r2.removeFromLeft (6); folderButton.setBounds (r2);
        withSourceButton.setBounds (r3);
        rp.removeFromLeft (12);
        tile.setBounds (rp);
    }

    // 2 rangées de 5 boutons
    const int rowH = (area.getHeight() - 8) / 2;
    auto row1 = area.removeFromTop (rowH);
    area.removeFromTop (8);
    auto row2 = area.removeFromTop (rowH);
    const int cw = row1.getWidth() / 5;

    auto place = [] (Knob& k, Rectangle<int> cell)
    {
        k.label.setBounds (cell.removeFromTop (16));
        k.slider.setBounds (cell.reduced (10, 0));
    };
    for (int i = 0; i < 5; ++i)
    {
        place (*knobs[(size_t) i],     row1.removeFromLeft (cw));
        place (*knobs[(size_t) i + 5], row2.removeFromLeft (cw));
    }

    if (help != nullptr)     help->setBounds (getLocalBounds());
    if (settings != nullptr) settings->setBounds (getLocalBounds());
}

// ============================================================ Éditeur
SwellAudioProcessorEditor::SwellAudioProcessorEditor (SwellAudioProcessor& p)
    : AudioProcessorEditor (&p), view ([&]() -> SwellAudioProcessor&
      {
          // applique la couleur sauvegardée avant de construire la vue
          auto saved = SwellSettings::file().getValue ("accent");
          if (saved.isNotEmpty())
              SwellColours::setAccent (Colour::fromString (saved));
          lnf.refreshColours();
          return p;
      }(), lnf)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (view);

    view.onScaleChange    = [this] (float s) { setUIScale (s); };
    view.onTooltipsChange = [this] (bool b)
    {
        SwellSettings::file().setValue ("tooltips", b);
        setTooltipsEnabled (b);
    };
    setTooltipsEnabled (SwellSettings::file().getBoolValue ("tooltips", true));

    setResizable (true, true);
    if (auto* c = getConstrainer())
    {
        c->setFixedAspectRatio ((double) SwellMainView::designW / SwellMainView::designH);
        c->setSizeLimits (SwellMainView::designW * 3 / 4, SwellMainView::designH * 3 / 4,
                          SwellMainView::designW * 2,     SwellMainView::designH * 2);
    }
    const float s = (float) SwellSettings::file().getDoubleValue ("uiScale", 1.0);
    setSize (roundToInt (SwellMainView::designW * s), roundToInt (SwellMainView::designH * s));
}

SwellAudioProcessorEditor::~SwellAudioProcessorEditor()
{
    tooltipWindow.reset();
    setLookAndFeel (nullptr);
}

void SwellAudioProcessorEditor::setTooltipsEnabled (bool b)
{
    if (b && tooltipWindow == nullptr)
    {
        tooltipWindow = std::make_unique<TooltipWindow> (this, 650);
        tooltipWindow->setLookAndFeel (&lnf);
    }
    else if (! b)
    {
        tooltipWindow.reset();
    }
}

void SwellAudioProcessorEditor::setUIScale (float s)
{
    setSize (roundToInt (SwellMainView::designW * s), roundToInt (SwellMainView::designH * s));
}

void SwellAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / (float) SwellMainView::designW;
    view.setBounds (0, 0, SwellMainView::designW, SwellMainView::designH);
    view.setTransform (AffineTransform::scale (s));
    SwellSettings::file().setValue ("uiScale", s);
}
