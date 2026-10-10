/*
 * Sabrina Reverb face host — no NamedPanel chrome.
 */

#ifndef BACONPAUL_SIDEQUEST_UI_MAIN_PANEL_H
#define BACONPAUL_SIDEQUEST_UI_MAIN_PANEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "plugin-editor.h"
#include "sabrina-face.h"

namespace baconpaul::sidequest_ns::ui
{
struct MainPanel : juce::Component
{
    MainPanel(PluginEditor &editor);
    void resized() override;
    void pushPeaks(float left, float right);

    std::unique_ptr<SabrinaFace> face;
    PluginEditor &editor;
};
} // namespace baconpaul::sidequest_ns::ui
#endif
