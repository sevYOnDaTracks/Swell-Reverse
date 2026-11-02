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
        { "Orange",  0xffff7a3d }, { "Red",     0xffef4444 }, { "Pink",    0xffec4899 },
        { "Purple",  0xff8b5cf6 }, { "Blue",    0xff3b82f6 }, { "Cyan",    0xff22d3ee },
        { "Green",   0xff22c55e }, { "Gold",    0xfff5b301 }, { "White",   0xffe8e6e1 },
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
    const bool danger = b.getProperties()["danger"];
    // Allumé : contour + teinte de l'accent (l'orange plein est réservé à l'action principale) ; BYPASS reste plein rouge
    const auto onColour = danger ? SwellColours::rec : SwellColours::accent();
    g.setColour (on ? (danger ? onColour : SwellColours::panel.overlaidWith (onColour.withAlpha (0.16f)))
                    : SwellColours::panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? onColour : (highlighted ? SwellColours::dim : SwellColours::track));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
    g.setColour (on ? (danger ? SwellColours::bg : onColour) : SwellColours::text);
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

// ============================================================ Bouton icône
void IconButton::paintButton (Graphics& g, bool highlighted, bool down)
{
    const auto bg = findColour (TextButton::buttonColourId);
    getLookAndFeel().drawButtonBackground (g, *this, bg, highlighted, down);

    // Fond plein (capture en cours, lecture) : icône et texte en sombre
    const bool filled = bg != SwellColours::panel;
    const float alpha = isEnabled() ? 1.0f : 0.4f;
    const auto ink = (filled ? SwellColours::bg : SwellColours::text).withMultipliedAlpha (alpha);

    auto r = getLocalBounds().toFloat().reduced (4.0f, 6.0f);
    auto labelArea = r.removeFromBottom (13.0f);
    const auto box = r.withSizeKeepingCentre (22.0f, 22.0f);
    const auto toBox = AffineTransform::scale (22.0f / 24.0f).translated (box.getX(), box.getY());
    const PathStrokeType stroke (2.0f, PathStrokeType::curved, PathStrokeType::rounded);

    switch (icon)
    {
        case Icon::record:
            g.setColour (ink);
            g.drawEllipse (box.reduced (2.0f), 2.0f);
            g.setColour ((filled ? SwellColours::bg : SwellColours::accent()).withMultipliedAlpha (alpha));
            g.fillEllipse (box.reduced (7.0f));
            break;
        case Icon::load:
        {
            auto p = Drawable::parseSVGPath ("M12 3v12M7 10l5 5 5-5M4 15v4a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-4");
            p.applyTransform (toBox);
            g.setColour (ink);
            g.strokePath (p, stroke);
            break;
        }
        case Icon::play:
        {
            g.setColour (ink);
            if (getButtonText() == "STOP")
                g.fillRoundedRectangle (box.reduced (5.0f), 2.0f);
            else
            {
                auto p = Drawable::parseSVGPath ("M8 5.5v13a1 1 0 0 0 1.5.86l10.5-6.5a1 1 0 0 0 0-1.72L9.5 4.64A1 1 0 0 0 8 5.5z");
                p.applyTransform (toBox);
                g.fillPath (p);
            }
            break;
        }
        case Icon::folder:
        {
            auto p = Drawable::parseSVGPath ("M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z");
            p.applyTransform (toBox);
            g.setColour (ink);
            g.strokePath (p, stroke);
            break;
        }
    }

    g.setColour (ink);
    g.setFont (FontOptions (10.0f, Font::bold));
    g.drawText (getButtonText(), labelArea, Justification::centred);
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
    g.drawText ("RISE", Rectangle<float> (swellArea.getX(), plot.getBottom() + 1.0f, 80.0f, 13.0f), Justification::left);
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
                g.drawText ("ORIGINAL SOUND >", inner.removeFromRight (130.0f).removeFromTop (14.0f), Justification::centredRight);
            }
        }

        // Poignée orange : l'action principale du panneau, c'est de glisser le clip
        auto pill = label.withWidth (138.0f).expanded (0.0f, 3.0f);
        g.setColour (SwellColours::accent());
        g.fillRoundedRectangle (pill, 5.0f);
        g.setColour (SwellColours::bg);
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 2; ++col)
                g.fillEllipse (pill.getX() + 7.0f + (float) col * 4.5f, pill.getCentreY() - 6.0f + (float) row * 4.5f, 2.6f, 2.6f);
        g.setFont (FontOptions (11.0f, Font::bold));
        g.drawText ("DRAG TO PLAYLIST", pill.withTrimmedLeft (20.0f), Justification::centredLeft);

        g.setColour (SwellColours::dim);
        g.setFont (FontOptions (11.0f));
        g.drawText (file.getFileName(), label.withTrimmedLeft (150.0f), Justification::centredRight);
    }
    else
    {
        g.setColour (SwellColours::dim);
        g.setFont (FontOptions (13.0f));
        g.drawFittedText ("Drop a sample here, or CAPTURE the track,\nthen drag the rendered clip into the playlist",
                          inner.withTrimmedBottom (-16.0f).toNearestInt(), Justification::centred, 2);
    }

    g.setColour (highlight || ready ? SwellColours::accent() : SwellColours::track);
    g.drawRoundedRectangle (r, 8.0f, highlight || hover ? 2.4f : (ready ? 1.8f : 1.2f));
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
    closeButton.setTooltip ("Close");
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

HelpOverlay::HelpOverlay() : OverlayPanel (fr ("HELP  —  How to use SWELL"))
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
        "HOW IT WORKS\n"
        "Swell creates a reverse reverb: the reverb rises BEFORE the sound and stops right on its transient "
        "(the attack). Two ways to use it:\n"
        "  1. In real time on a mixer track (your DAW compensates the latency automatically).\n"
        "  2. In RENDER mode: Swell makes an audio clip that you drag into the playlist.\n\n"

        "RENDER → PLAYLIST MODE (recommended)\n"
        "  1. CAPTURE, then start playback: capture starts on the first sound and stops when you stop "
        "playback (or click STOP).  Or drop a sample on the plugin / use LOAD.\n"
        "  2. Swell detects the transient and builds the clip. Turn the knobs: the clip updates by itself.\n"
        "  3. PLAY to preview. With \"+ ORIGINAL SOUND\" you hear the rise followed by the sound.\n"
        "  4. Drag the tile into the playlist and place the END of the clip right at the start of your sound.\n"
        "     In SYNC mode the clip length lands exactly on the grid.\n"
        "  FOLDER opens the folder where every clip is saved (Music/Swell Reverse).\n\n"

        "RISE LENGTH\n"
        "  SYNC + division: length locked to the tempo (1/16 to 2 bars).\n"
        "  LENGTH: free length in seconds (when SYNC is off).\n\n"

        "SOUND\n"
        "  SHAPE: 0 % = long, gradual rise, 100 % = late rise that \"sucks in\".\n"
        "  TONE: dark ↔ bright.\n"
        "  LOW CUT: removes the low end of the reverb to keep the mix clean.\n"
        "  WIDTH: mono ↔ wide stereo.\n"
        "  GAP: short silence between the end of the rise and the transient.\n\n"

        "GATE (optional)\n"
        "  GATE: chops the rise in rhythm (1/4 to 1/32, triplets).  Off = continuous rise.\n"
        "  GATE DEPTH: how hard it chops.\n"
        "  GATE SMOOTH: 0 % = hard cut (stutter), 100 % = soft \"wou-wou\" wave.\n"
        "  The gate is always open at the transient.\n\n"

        "OUTPUT\n"
        "  MIX: 0 % = dry only, 50 % = dry + full reverb, 100 % = reverb only (great on a send).\n"
        "  OUTPUT: output volume.  BYPASS: turns the effect off without changing the latency.\n\n"

        "PRESETS\n"
        "  Top menu: factory presets and your presets.  < > to step through them.\n"
        "  SAVE stores your settings (Music/Swell Reverse/Presets).\n\n"

        "TIPS\n"
        "  • Double-click a knob = default value.  Cmd + drag = fine adjustment.\n"
        "  • Hover any element to see its tooltip (can be turned off in SETTINGS).\n"
        "  • In real time, if you change the length or the tempo, restart playback so your DAW realigns.\n"
        "  • SETTINGS: interface colour, window size, tooltips.\n"));

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
    g.drawText ("INTERFACE COLOUR", colourLabel, Justification::centredLeft);
    g.drawText ("WINDOW SIZE", scaleLabel, Justification::centredLeft);
    g.drawText ("OTHER", otherLabel, Justification::centredLeft);

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

    setupKnob (length,     ParamID::length,     "LENGTH",      "Free rise length (when SYNC is off).");
    setupKnob (shape,      ParamID::shape,      "SHAPE",       "Rise shape: gradual (low) or a late rise that sucks in (high).");
    setupKnob (tone,       ParamID::tone,       "TONE",        fr ("Reverb colour: dark ↔ bright."));
    setupKnob (lowCut,     ParamID::lowCut,     "LOW CUT",     "Removes the low end of the reverb to keep the mix clean.");
    setupKnob (width,      ParamID::width,      "WIDTH",       "Stereo width of the reverb.");
    setupKnob (gap,        ParamID::gap,        "GAP",         "Silence between the end of the rise and the transient.");
    setupKnob (gateDepth,  ParamID::gateDepth,  "GATE DEPTH",  "Gate intensity (needs GATE on).");
    setupKnob (gateSmooth, ParamID::gateSmooth, "GATE SMOOTH", "0 % = hard cut, 100 % = soft \"wou-wou\" wave.");
    setupKnob (mix,        ParamID::mix,        "MIX",         "0 % = dry only, 50 % = dry + reverb, 100 % = reverb only.");
    setupKnob (output,     ParamID::output,     "OUTPUT",      "Output volume (also applied to rendered clips).");

    // Presets
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.setTooltip ("Factory presets and your presets.");
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
    prevPreset.setTooltip ("Previous preset");
    nextPreset.setTooltip ("Next preset");
    addAndMakeVisible (prevPreset);
    addAndMakeVisible (nextPreset);
    saveButton.onClick = [this] { savePresetDialog(); };
    saveButton.setTooltip ("Save the current settings as a preset.");
    addAndMakeVisible (saveButton);
    refreshPresetList();

    helpButton.onClick = [this] { showHelp(); };
    helpButton.setTooltip ("Help: what every control does.");
    addAndMakeVisible (helpButton);
    settingsButton.onClick = [this] { showSettings(); };
    settingsButton.setTooltip ("Colour, window size, tooltips.");
    addAndMakeVisible (settingsButton);

    // Gate
    gateLabel.setText ("GATE", dontSendNotification);
    gateLabel.setFont (FontOptions (12.0f, Font::bold));
    gateLabel.setJustificationType (Justification::centredRight);
    addAndMakeVisible (gateLabel);
    if (auto* choice = dynamic_cast<AudioParameterChoice*> (p.apvts.getParameter (ParamID::gate)))
        gateBox.addItemList (choice->choices, 1);
    gateBox.setJustificationType (Justification::centred);
    gateBox.setTooltip ("Rhythmic gate on the rise (Off = continuous rise).");
    addAndMakeVisible (gateBox);
    gateAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, ParamID::gate, gateBox);
    gateBox.onChange = [this] { updateEnablement(); display.repaint(); };

    // Bypass
    bypassButton.getProperties().set ("danger", true);
    bypassButton.setTooltip ("Turns the effect off (latency stays the same).");
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ParamID::bypass, bypassButton);

    // Sync
    syncButton.setTooltip ("Rise length locked to your DAW tempo.");
    addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ParamID::sync, syncButton);
    syncButton.onStateChange = [this] { updateEnablement(); };

    if (auto* choice = dynamic_cast<AudioParameterChoice*> (p.apvts.getParameter (ParamID::division)))
        divisionBox.addItemList (choice->choices, 1);
    divisionBox.setJustificationType (Justification::centred);
    divisionBox.setTooltip ("Rise length as a note value.");
    addAndMakeVisible (divisionBox);
    divisionAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, ParamID::division, divisionBox);

    infoLabel.setJustificationType (Justification::centredRight);
    infoLabel.setFont (FontOptions (12.0f));
    addAndMakeVisible (infoLabel);

    // Render
    captureButton.onClick = [this] { proc.toggleCapture(); };
    captureButton.setTooltip ("Arm the capture, then start playback: records the sound coming into Swell.");
    loadButton.onClick = [this] { chooseFile(); };
    loadButton.setTooltip ("Load a sample (or drop it straight onto the plugin).");
    playButton.onClick = [this] { proc.togglePreview(); };
    playButton.setTooltip ("Preview the rendered clip.");
    folderButton.onClick = []
    {
        auto f = SwellAudioProcessor::getRenderFolder();
        f.createDirectory();
        f.revealToUser();
    };
    folderButton.setTooltip ("Opens the folder where clips are saved.");
    withSourceButton.setToggleState (proc.getPreviewWithSource(), dontSendNotification);
    withSourceButton.onClick = [this] { proc.setPreviewWithSource (withSourceButton.getToggleState()); };
    withSourceButton.setTooltip ("Preview: plays the original sound right after the rise.");
    for (auto* c : std::initializer_list<Component*> { &captureButton, &loadButton, &playButton, &folderButton, &withSourceButton, &tile })
        addAndMakeVisible (c);
    tile.setTooltip ("Drag this tile into your DAW playlist.");
    renderStatus.setFont (FontOptions (12.0f));
    renderStatus.setJustificationType (Justification::centredLeft);
    addAndMakeVisible (renderStatus);

    styleBigKnobs();
    infoLabel.setColour (Label::textColourId, SwellColours::label);
    renderStatus.setColour (Label::textColourId, SwellColours::label);
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
    k.label.setFont (FontOptions (10.5f, Font::bold));
    k.label.setColour (Label::textColourId, SwellColours::label);
    k.label.setMinimumHorizontalScale (1.0f);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
    if (auto* param = proc.apvts.getParameter (paramId))
        k.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    k.slider.onValueChange = [this] { display.repaint(); };
}

// Les 3 grands boutons : titre plus gros en blanc, valeur en couleur d'accent
void SwellMainView::styleBigKnobs()
{
    for (auto* k : bigKnobs)
    {
        k->label.setFont (FontOptions (13.0f, Font::bold));
        k->label.setColour (Label::textColourId, SwellColours::text);
        k->slider.setTextBoxStyle (Slider::TextBoxBelow, false, 130, 22);
        k->slider.setColour (Slider::textBoxTextColourId, SwellColours::accent());
    }
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
    presetBox.addSectionHeading ("FACTORY");
    auto factory = proc.getFactoryPresetNames();
    for (int i = 0; i < factory.size(); ++i)
        presetBox.addItem (factory[i], i + 1);

    auto user = proc.getUserPresetNames();
    if (! user.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("MY PRESETS");
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
    auto* w = new AlertWindow ("Save preset", "Preset name:", MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf);
    w->addTextEditor ("name", proc.getCurrentPresetName() == "Init" ? String() : proc.getCurrentPresetName());
    w->addButton ("SAVE", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("CANCEL", 0, KeyPress (KeyPress::escapeKey));

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
    chooser = std::make_unique<FileChooser> ("Choose a sample", File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.m4a;*.ogg");
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
    help->onClose = [this] { Component::SafePointer<SwellMainView> sp (this); MessageManager::callAsync ([sp] { if (sp) sp->help.reset(); }); };
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
    settings->onClose = [this] { Component::SafePointer<SwellMainView> sp (this); MessageManager::callAsync ([sp] { if (sp) sp->settings.reset(); }); };
    addAndMakeVisible (*settings);
    settings->setBounds (getLocalBounds());
}

void SwellMainView::applyAccent (Colour c)
{
    SwellColours::setAccent (c);
    SwellSettings::file().setValue ("accent", c.toString());
    lnf.refreshColours();
    styleBigKnobs();
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
    // Outil de dev pour les captures du README :
    // SWELL_SNAPSHOT=<fichier.png> [SWELL_SNAPSHOT_SAMPLE=<son>] [SWELL_SNAPSHOT_OVERLAY=help|settings]
    if (! snapshotChecked)
    {
        snapshotChecked = true;
        const auto out = SystemStats::getEnvironmentVariable ("SWELL_SNAPSHOT", {});
        if (out.isNotEmpty())
        {
            const auto sample = SystemStats::getEnvironmentVariable ("SWELL_SNAPSHOT_SAMPLE", {});
            if (sample.isNotEmpty())
                proc.renderFromFile (File (sample));
            const auto overlay = SystemStats::getEnvironmentVariable ("SWELL_SNAPSHOT_OVERLAY", {});
            if (overlay == "help")     showHelp();
            if (overlay == "settings") showSettings();
            Timer::callAfterDelay (3000, [sp = Component::SafePointer<SwellMainView> (this), out]
            {
                if (sp == nullptr) return;
                FileOutputStream stream { File (out) };
                if (stream.openedOk())
                {
                    stream.setPosition (0);
                    stream.truncate();
                    PNGImageFormat().writeImageToStream (sp->createComponentSnapshot (sp->getLocalBounds(), true, 2.0f), stream);
                }
                if (auto* app = JUCEApplicationBase::getInstance())
                    app->systemRequestedQuit();
            });
        }
    }

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
            captureButton.setButtonText (fr ("ARMED…"));
            captureButton.setColour (TextButton::buttonColourId,
                                     (blink / 10) % 2 ? SwellColours::rec.withAlpha (0.6f) : SwellColours::panel);
            break;
        case SwellAudioProcessor::CaptureState::recording:
            captureButton.setButtonText ("STOP " + String (proc.getCaptureSeconds(), 1) + "s");
            captureButton.setColour (TextButton::buttonColourId, SwellColours::rec);
            break;
    }

    if (blink % 6 == 0)
    {
        String info;
        info << "Rise " << String (proc.getLengthSeconds(), 2) << " s";
        if (syncButton.getToggleState() || gateBox.getSelectedItemIndex() > 0)
            info << "  |  " << String (proc.getHostBpm(), 1) << " BPM";
        info << "  |  Latency " << String (roundToInt (proc.getLatencyMs())) << " ms";
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

    // Séparation entre les grands boutons et les petits
    if (! knobDivider.isEmpty())
    {
        g.setColour (SwellColours::track.withAlpha (0.7f));
        g.drawHorizontalLine (knobDivider.getCentreY(), (float) knobDivider.getX(), (float) knobDivider.getRight());
    }

    // Pied de page : nom + version à gauche, by @sevyondatracks + logo à droite
    if (! footerArea.isEmpty())
    {
        auto f = footerArea;
        g.setColour (SwellColours::track.withAlpha (0.7f));
        g.drawHorizontalLine (f.getY() + 2, (float) f.getX(), (float) f.getRight());
        f.removeFromTop (4);

        g.setColour (SwellColours::dim);
        g.setFont (FontOptions (12.0f));
        g.drawText (fr ("SWELL Reverse Reverb  ·  v") + String (SWELL_VERSION), f, Justification::centredLeft);

        if (logoS.isValid())
        {
            g.setOpacity (0.95f);
            g.drawImage (logoS, f.removeFromRight (26).withSizeKeepingCentre (22, 20).toFloat(), RectanglePlacement::centred);
            f.removeFromRight (8);
        }
        g.setColour (SwellColours::dim);
        g.drawText ("by @sevyondatracks", f, Justification::centredRight);
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
    footerArea = area.removeFromBottom (32);
    renderPanelArea = area.removeFromBottom (176);
    area.removeFromBottom (10);

    {
        auto rp = renderPanelArea.reduced (14, 10);
        rp.removeFromTop (22);
        renderStatus.setBounds (rp.removeFromBottom (20));
        rp.removeFromBottom (4);
        auto col = rp.removeFromLeft (248);
        // 4 boutons icône sur une ligne, + ORIGINAL SOUND en dessous
        withSourceButton.setBounds (col.removeFromBottom (30));
        col.removeFromBottom (6);
        const int bw = (col.getWidth() - 18) / 4;
        for (auto* b : { &captureButton, &loadButton, &playButton, &folderButton })
        {
            b->setBounds (col.removeFromLeft (bw));
            col.removeFromLeft (6);
        }
        rp.removeFromLeft (12);
        tile.setBounds (rp);
    }

    // Les 3 réglages principaux en grand...
    auto smallRow = area.removeFromBottom (92);
    knobDivider = area.removeFromBottom (10).reduced (10, 0);
    auto bigRow = area.withSizeKeepingCentre (600, area.getHeight());
    const int bw = bigRow.getWidth() / 3;
    for (auto* k : bigKnobs)
    {
        auto cell = bigRow.removeFromLeft (bw);
        k->label.setBounds (cell.removeFromTop (18));
        k->slider.setBounds (cell.withSizeKeepingCentre (jmin (cell.getHeight() - 22, bw), cell.getHeight()).reduced (0, 2));
    }

    // ...et les 7 autres en petit en dessous
    const int sw = smallRow.getWidth() / 7;
    for (auto* k : smallKnobs)
    {
        auto cell = smallRow.removeFromLeft (sw);
        k->label.setBounds (cell.removeFromTop (14));
        k->slider.setBounds (cell.reduced (8, 0));
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
