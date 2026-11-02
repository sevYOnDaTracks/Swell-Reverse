#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ============================================================ Thème + réglages globaux
namespace SwellColours
{
    const juce::Colour bg       { 0xff0f0f12 };
    const juce::Colour panel    { 0xff1a1a1f };
    const juce::Colour track    { 0xff2c2c34 };
    const juce::Colour text     { 0xffeeebe6 };
    const juce::Colour dim      { 0xffb4b2ba };   // assez clair pour rester lisible sur le fond noir
    const juce::Colour label    { 0xffc8c6cd };
    const juce::Colour rec      { 0xffe5484d };

    juce::Colour accent();          // couleur choisie dans Settings
    juce::Colour accent2();         // variante claire
    void setAccent (juce::Colour);

    struct Swatch { const char* name; juce::uint32 argb; };
    const std::vector<Swatch>& swatches();
}

// Réglages de l'interface, partagés par toutes les instances (couleur, taille, infobulles)
namespace SwellSettings
{
    juce::PropertiesFile& file();
}

// ============================================================ LookAndFeel
class SwellLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SwellLookAndFeel();
    void refreshColours();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
};

// ============================================================ Courbe + vumètre
class SwellDisplay : public juce::Component
{
public:
    explicit SwellDisplay (SwellAudioProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void refresh();
    void setMeter (float l, float r);
private:
    SwellAudioProcessor& proc;
    std::vector<float> env;
    float meterL = 0.0f, meterR = 0.0f;
};

// ============================================================ Bouton icône + nom en dessous (panneau Render)
class IconButton : public juce::TextButton
{
public:
    enum class Icon { record, load, play, folder };
    IconButton (const juce::String& name, Icon i) : juce::TextButton (name), icon (i) {}
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
private:
    Icon icon;
};

// ============================================================ Tuile du clip (glissable)
class DragTile : public juce::Component, public juce::SettableTooltipClient
{
public:
    void setClip (const juce::File& f, std::vector<float> envelope);
    void setHighlight (bool h) { highlight = h; repaint(); }
    void setPlayhead (float pos, float clipFrac) { playhead = pos; clipFraction = clipFrac; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }
private:
    juce::File file;
    std::vector<float> env;
    bool highlight = false, dragging = false;
    float playhead = -1.0f, clipFraction = 1.0f;
};

// ============================================================ Fenêtres superposées
class OverlayPanel : public juce::Component
{
public:
    explicit OverlayPanel (const juce::String& title);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void()> onClose;
    void setCardHeight (int h) { cardHeight = h; }
protected:
    juce::Rectangle<int> getContentArea() const { return content; }
    virtual void layoutContent (juce::Rectangle<int>) {}
private:
    juce::String title;
    juce::TextButton closeButton { "X" };
    juce::Rectangle<int> card, content;
    int cardHeight = 0;
};

class HelpOverlay : public OverlayPanel
{
public:
    HelpOverlay();
protected:
    void layoutContent (juce::Rectangle<int>) override;
private:
    juce::TextEditor text;
};

class SettingsOverlay : public OverlayPanel
{
public:
    SettingsOverlay();
    void paint (juce::Graphics&) override;
    std::function<void (juce::Colour)> onAccent;
    std::function<void (float)> onScale;
    std::function<void (bool)> onTooltips;
    void setCurrent (juce::Colour accent, float scale, bool tooltips);
protected:
    void layoutContent (juce::Rectangle<int>) override;
private:
    juce::OwnedArray<juce::TextButton> swatchButtons, scaleButtons;
    juce::ToggleButton tooltipToggle { "TOOLTIPS" };
    juce::TextButton folderButton { "OPEN CLIPS FOLDER" };
    juce::TextButton presetFolderButton { "OPEN PRESETS FOLDER" };
    juce::Rectangle<int> colourLabel, scaleLabel, otherLabel;
    juce::Colour currentAccent;
    float currentScale = 1.0f;
};

// ============================================================ Vue principale (taille de référence 720x740)
class SwellMainView : public juce::Component,
                      public juce::DragAndDropContainer,
                      public juce::FileDragAndDropTarget,
                      private juce::Timer
{
public:
    static constexpr int designW = 720, designH = 740;

    SwellMainView (SwellAudioProcessor&, SwellLookAndFeel&);
    ~SwellMainView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { tile.setHighlight (true); }
    void fileDragExit (const juce::StringArray&) override              { tile.setHighlight (false); }
    void filesDropped (const juce::StringArray& files, int, int) override;

    std::function<void (float)> onScaleChange;
    std::function<void (bool)>  onTooltipsChange;

    void showHelp();
    void showSettings();

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name, const juce::String& tip);
    void updateEnablement();
    void refreshPresetList();
    void stepPreset (int delta);
    void savePresetDialog();
    void chooseFile();
    void applyAccent (juce::Colour c);
    void styleBigKnobs();

    SwellAudioProcessor& proc;
    SwellLookAndFeel& lnf;
    SwellDisplay display;
    juce::Image logoS;

    Knob length, shape, tone, lowCut, width, gap, gateDepth, gateSmooth, mix, output;
    // Les 3 réglages principaux en grand, les autres en petit en dessous
    std::array<Knob*, 3> bigKnobs { &length, &shape, &mix };
    std::array<Knob*, 7> smallKnobs { &tone, &lowCut, &width, &gap, &gateDepth, &gateSmooth, &output };
    juce::Rectangle<int> knobDivider;

    // En-tête / barre d'outils
    juce::TextButton prevPreset { "<" }, nextPreset { ">" };
    juce::ComboBox presetBox;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton helpButton { "?" };
    juce::TextButton settingsButton { "SETTINGS" };
    juce::Label gateLabel;
    juce::ComboBox gateBox;
    juce::ToggleButton bypassButton { "BYPASS" };
    juce::ToggleButton syncButton { "SYNC" };
    juce::ComboBox divisionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   syncAttachment, bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment, gateAttachment;

    juce::Label infoLabel;

    // Panneau Render
    IconButton captureButton { "CAPTURE", IconButton::Icon::record };
    IconButton loadButton    { "LOAD",    IconButton::Icon::load };
    IconButton playButton    { "PLAY",    IconButton::Icon::play };
    IconButton folderButton  { "FOLDER",  IconButton::Icon::folder };
    juce::ToggleButton withSourceButton { "+ ORIGINAL SOUND" };
    DragTile tile;
    juce::Label renderStatus;
    juce::Rectangle<int> renderPanelArea, footerArea;
    std::unique_ptr<juce::FileChooser> chooser;

    std::unique_ptr<HelpOverlay> help;
    std::unique_ptr<SettingsOverlay> settings;

    int lastDisplayVersion = -1, lastRenderVersion = -1, blink = 0;
    bool snapshotChecked = false;
    juce::String lastPresetName;
    float meterL = 0.0f, meterR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwellMainView)
};

// ============================================================ Éditeur (redimensionnable)
class SwellAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SwellAudioProcessorEditor (SwellAudioProcessor&);
    ~SwellAudioProcessorEditor() override;
    void resized() override;
    void setUIScale (float s);

private:
    SwellLookAndFeel lnf;
    SwellMainView view;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    void setTooltipsEnabled (bool b);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwellAudioProcessorEditor)
};
