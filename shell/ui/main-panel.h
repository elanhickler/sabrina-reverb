/*
 * Face host: holds the product's face (no NamedPanel chrome).
 */

#ifndef SOUNDEMOTE_SABRINA_UI_MAIN_PANEL_H
#define SOUNDEMOTE_SABRINA_UI_MAIN_PANEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "plugin-editor.h"
#include "product/product-face.h"

namespace soundemote::sabrina::ui
{
struct MainPanel : juce::Component
{
    MainPanel(PluginEditor &editor);
    void resized() override;
    void pushVizData(const float *values, size_t count);

    std::unique_ptr<se::product::Face> face;
    PluginEditor &editor;
};
} // namespace soundemote::sabrina::ui
#endif
