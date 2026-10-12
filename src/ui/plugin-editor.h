/*
 * Sabrina Reverb — Soundemote
 * DSP MIT (Walter Hackett). UI links JUCE (AGPLv3). See LICENSE.md.
 */

#ifndef SOUNDEMOTE_SABRINA_UI_PLUGIN_EDITOR_H
#define SOUNDEMOTE_SABRINA_UI_PLUGIN_EDITOR_H

#include <functional>
#include <utility>
#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/style/JUCELookAndFeelAdapter.h"

#include <sst/jucegui/components/WindowPanel.h>
#include <sst/jucegui/components/ToolTip.h>
#include <sst/jucegui/components/ToggleButton.h>
#include <sst/jucegui/components/ContinuousParamEditor.h>
#include <sst/jucegui/accessibility/FocusDebugger.h>
#include <sst/jucegui/data/Continuous.h>

#include "engine/engine.h"
#include "engine/patch.h"
#include "ui-defaults.h"

namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

namespace soundemote::sabrina::ui
{

struct MainPanel;

struct PluginEditor : jcmp::WindowPanel
{
    Patch patchCopy;

    Engine::audioToUIQueue_t &audioToUI;
    Engine::mainToAudioQueue_T &mainToAudio;
    const clap_host_t *clapHost{nullptr};

    PluginEditor(Engine::audioToUIQueue_t &atou, Engine::mainToAudioQueue_T &utoa,
                 const clap_host_t *ch);
    virtual ~PluginEditor();

    std::unique_ptr<sst::jucegui::style::LookAndFeelManager> lnf;
    void onStyleChanged() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    void idle();
    std::unique_ptr<juce::Timer> idleTimer;

    std::unique_ptr<MainPanel> mainPanel;

    void postPatchChange(const std::string &displayName);

    std::unique_ptr<defaultsProvder_t> defaultsProvider;

    void setSkinFromDefaults();

    std::unique_ptr<jcmp::ToolTip> toolTip;
    void showTooltipOn(juce::Component *c);
    void updateTooltip(jdat::Continuous *c);
    void updateTooltip(jdat::Discrete *d);
    void hideTooltip();

    void popupMenuForContinuous(jcmp::ContinuousParamEditor *e);

    std::unordered_map<uint32_t, juce::Component::SafePointer<juce::Component>> componentByID;
    std::unordered_map<uint32_t, std::function<void()>> componentRefreshByID;

    void setAndSendParamValue(uint32_t id, float value, bool notifyAudio = true,
                              bool includeBeginEnd = true);
    void setAndSendParamValue(const Param &p, float value, bool notifyAudio = true,
                              bool includeBeginEnd = true)
    {
        setAndSendParamValue(p.meta.id, value, notifyAudio, includeBeginEnd);
    }

    bool keyPressed(const juce::KeyPress &key) override;

    void setZoomFactor(float zf);
    void setScreenSize(int titleHeights);
    void showUiSizeMenu();
    void copyJsonPreset();
    void pasteJsonPreset();
    void saveJsonPreset();
    void loadJsonPreset();
    juce::String jsonPresetFromPatch() const;
    bool applyJsonPreset(const juce::String &text);
    std::unique_ptr<juce::FileChooser> fileChooser;
    float zoomFactor{1.0f};
    int screenSize{2}; // display height in title-bar heights, 1..5
    int designHeight() const { return (int)edHeight; }
    int px(int native) const { return juce::roundToInt((float)native * zoomFactor); }
    float px(float native) const { return native * zoomFactor; }
    std::function<void(float)> onZoomChanged{nullptr};
    bool toggleDebug();

    static constexpr uint32_t edWidth{234}, edHeight{516};

    void visibilityChanged() override;
    void parentHierarchyChanged() override;
    std::unique_ptr<sst::jucegui::accessibility::FocusDebugger> focusDebugger;

    float sampleRate{0};

    void requestParamsFlush();
    const clap_host_params_t *clapParamsExtension{nullptr};

    // the name tells you about the intent. It just makes startup faster
    void sneakyStartupGrabFrom(Patch &other);
};

struct HasEditor
{
    PluginEditor &editor;
    HasEditor(PluginEditor &e) : editor(e) {}
};
} // namespace soundemote::sabrina::ui
#endif
