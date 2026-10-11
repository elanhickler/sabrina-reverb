/*
 * Sabrina Reverb face host — no NamedPanel chrome.
 */

#ifndef SOUNDEMOTE_SABRINA_UI_MAIN_PANEL_H
#define SOUNDEMOTE_SABRINA_UI_MAIN_PANEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "plugin-editor.h"
#include "sabrina-face.h"

namespace soundemote::sabrina::ui
{
struct MainPanel : juce::Component
{
    MainPanel(PluginEditor &editor);
    void resized() override;
    void pushPeaks(float left, float right);

    std::unique_ptr<SabrinaFace> face;
    PluginEditor &editor;
};
} // namespace soundemote::sabrina::ui
#endif
