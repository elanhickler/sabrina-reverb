/*
 * SideQuest Starting Point
 *
 * Basically lets paul bootstrap his projects.
 *
 * Copyright 2024-2025, Paul Walker and Various authors, as described in the github
 * transaction log.
 *
 * This source repo is released under the MIT license, but has
 * GPL3 dependencies, as such the combined work will be
 * released under GPL3.
 *
 * The source code and license are at https://github.com/baconpaul/sidequest-startingpoint
 */

#include "plugin-editor.h"

#include <cmath>

#include "sst/clap_juce_shim/menu_helper.h"
#include "sst/plugininfra/misc_platform.h"
#include "sst/plugininfra/paths.h"

#include "patch-data-bindings.h"
#include "main-panel.h"

namespace baconpaul::sidequest_ns::ui
{

struct IdleTimer : juce::Timer
{
    PluginEditor &editor;
    IdleTimer(PluginEditor &e) : editor(e) {}
    void timerCallback() override { editor.idle(); }
};

PluginEditor::PluginEditor(Engine::audioToUIQueue_t &atou, Engine::mainToAudioQueue_T &utoa,
                           const clap_host_t *h)
    : jcmp::WindowPanel(true), audioToUI(atou), mainToAudio(utoa), clapHost(h)
{
    setTitle("Sabrina Reverb");
    setAccessible(true);
    sst::jucegui::style::StyleSheet::initializeStyleSheets([]() {});

    setStyle(sst::jucegui::style::StyleSheet::getBuiltInStyleSheet(
        sst::jucegui::style::StyleSheet::DARK));

    mainPanel = std::make_unique<MainPanel>(*this);
    addAndMakeVisible(*mainPanel);

    auto startMsg = Engine::MainToAudioMsg{Engine::MainToAudioMsg::REQUEST_REFRESH};
    mainToAudio.push(startMsg);
    requestParamsFlush();

    idleTimer = std::make_unique<IdleTimer>(*this);
    idleTimer->startTimer(1000. / 60.);

    toolTip = std::make_unique<jcmp::ToolTip>();
    addChildComponent(*toolTip);

    auto userPath =
        sst::plugininfra::paths::bestDocumentsVendorFolderPathFor("Soundemote", "SabrinaReverb");
    defaultsProvider = std::make_unique<defaultsProvder_t>(
        userPath, "SabrinaReverb", defaultName,
        [](auto e, auto b) { SQLOG("[ERROR]" << e << " " << b); });
    setSkinFromDefaults();

    lnf = std::make_unique<sst::jucegui::style::LookAndFeelManager>(this);
    lnf->setStyle(style());

    mainToAudio.push({Engine::MainToAudioMsg::EDITOR_ATTACH_DETATCH, true});
    mainToAudio.push({Engine::MainToAudioMsg::REQUEST_REFRESH, true});
    requestParamsFlush();

    auto pzf = defaultsProvider->getUserDefaultValue(Defaults::zoomLevel, 100);
    zoomFactor = pzf * 0.01f;
    setSize((int)std::lround((float)edWidth * zoomFactor),
            (int)std::lround((float)edHeight * zoomFactor));
#define DEBUG_FOCUS 0
#if DEBUG_FOCUS
    focusDebugger = std::make_unique<sst::jucegui::accessibility::FocusDebugger>(*this);
    focusDebugger->setDoFocusDebug(true);
#endif
}
PluginEditor::~PluginEditor()
{
    mainToAudio.push({Engine::MainToAudioMsg::EDITOR_ATTACH_DETATCH, false});
    idleTimer->stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditor::idle()
{
    float scopeL = 0.f, scopeR = 0.f;
    bool haveScope = false;
    auto aum = audioToUI.pop();
    while (aum.has_value())
    {
        if (aum->action == Engine::AudioToUIMsg::UPDATE_PARAM)
        {
            setAndSendParamValue(aum->paramId, aum->value, false);
        }
        else if (aum->action == Engine::AudioToUIMsg::SCOPE_PEAK)
        {
            scopeL = aum->value;
            scopeR = aum->value2;
            haveScope = true;
        }
        else if (aum->action == Engine::AudioToUIMsg::SET_PATCH_NAME)
        {
            memset(patchCopy.name, 0, sizeof(patchCopy.name));
            strncpy(patchCopy.name, aum->patchNamePointer, 255);
        }
        else if (aum->action == Engine::AudioToUIMsg::SET_PATCH_DIRTY_STATE)
        {
            patchCopy.dirty = (bool)aum->paramId;
        }
        else if (aum->action == Engine::AudioToUIMsg::DO_PARAM_RESCAN)
        {
            if (!clapParamsExtension)
                clapParamsExtension = static_cast<const clap_host_params_t *>(
                    clapHost->get_extension(clapHost, CLAP_EXT_PARAMS));
            if (clapParamsExtension)
            {
                clapParamsExtension->rescan(clapHost,
                                            CLAP_PARAM_RESCAN_VALUES | CLAP_PARAM_RESCAN_TEXT);
                clapParamsExtension->request_flush(clapHost);
            }
        }
        else if (aum->action == Engine::AudioToUIMsg::SEND_SAMPLE_RATE)
        {
            sampleRate = aum->value;
            repaint();
        }
        else
        {
            SQLOG("Ignored patch message " << aum->action);
        }
        aum = audioToUI.pop();
    }
    if (haveScope && mainPanel)
        mainPanel->pushPeaks(scopeL, scopeR);
}

void PluginEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff171a1f));
}

void PluginEditor::resized()
{
    if (mainPanel)
        mainPanel->setBounds(getLocalBounds());
}

void PluginEditor::showTooltipOn(juce::Component *c)
{
    int x = 0;
    int y = 0;
    juce::Component *component = c;
    while (component != this)
    {
        auto bounds = component->getBoundsInParent();
        x += bounds.getX();
        y += bounds.getY();

        component = component->getParentComponent();
    }
    y += c->getHeight();
    toolTip->resetSizeFromData();
    if (y + toolTip->getHeight() > getHeight() - 40)
    {
        y -= c->getHeight() + 3 + toolTip->getHeight();
    }

    if (x + toolTip->getWidth() > getWidth())
    {
        x -= toolTip->getWidth();
        x += c->getWidth() - 3;
    }

    toolTip->setTopLeftPosition(x, y);
    toolTip->setVisible(true);
}
void PluginEditor::updateTooltip(jdat::Continuous *c)
{
    toolTip->setTooltipTitleAndData(c->getLabel(), {c->getValueAsString()});
    toolTip->resetSizeFromData();
}
void PluginEditor::updateTooltip(jdat::Discrete *d)
{
    toolTip->setTooltipTitleAndData(d->getLabel(), {d->getValueAsString()});
    toolTip->resetSizeFromData();
}
void PluginEditor::hideTooltip() { toolTip->setVisible(false); }

struct MenuValueTypein : HasEditor, juce::PopupMenu::CustomComponent, juce::TextEditor::Listener
{
    std::unique_ptr<juce::TextEditor> textEditor;
    juce::Component::SafePointer<jcmp::ContinuousParamEditor> underComp;

    MenuValueTypein(PluginEditor &editor,
                    juce::Component::SafePointer<jcmp::ContinuousParamEditor> under)
        : juce::PopupMenu::CustomComponent(false), HasEditor(editor), underComp(under)
    {
        textEditor = std::make_unique<juce::TextEditor>();
        textEditor->setWantsKeyboardFocus(true);
        textEditor->addListener(this);
        textEditor->setIndents(2, 0);

        addAndMakeVisible(*textEditor);
    }

    void getIdealSize(int &w, int &h) override
    {
        w = 180;
        h = 22;
    }
    void resized() override { textEditor->setBounds(getLocalBounds().reduced(3, 1)); }

    void visibilityChanged() override
    {
        juce::Timer::callAfterDelay(
            2,
            [this]()
            {
                if (textEditor->isVisible())
                {
                    textEditor->setText(getInitialText(),
                                        juce::NotificationType::dontSendNotification);
                    auto valCol = juce::Colour(0xFF, 0x90, 0x00);
                    textEditor->setColour(juce::TextEditor::ColourIds::backgroundColourId,
                                          valCol.withAlpha(0.1f));
                    textEditor->setColour(juce::TextEditor::ColourIds::highlightColourId,
                                          valCol.withAlpha(0.15f));
                    textEditor->setJustification(juce::Justification::centredLeft);
                    textEditor->setColour(juce::TextEditor::ColourIds::outlineColourId,
                                          juce::Colours::black.withAlpha(0.f));
                    textEditor->setColour(juce::TextEditor::ColourIds::focusedOutlineColourId,
                                          juce::Colours::black.withAlpha(0.f));
                    textEditor->setBorder(juce::BorderSize<int>(3));
                    textEditor->applyColourToAllText(valCol, true);
                    textEditor->grabKeyboardFocus();
                    textEditor->selectAll();
                }
            });
    }

    std::string getInitialText() const { return underComp->continuous()->getValueAsString(); }

    void setValueString(const std::string &s)
    {
        if (underComp && underComp->continuous())
        {
            underComp->onBeginEdit();

            if (s.empty())
            {
                underComp->continuous()->setValueFromGUI(
                    underComp->continuous()->getDefaultValue());
            }
            else
            {
                underComp->continuous()->setValueAsString(s);
            }
            underComp->onEndEdit();
            underComp->repaint();
            underComp->grabKeyboardFocus();
            underComp->notifyAccessibleChange();
        }
    }

    void textEditorReturnKeyPressed(juce::TextEditor &ed) override
    {
        auto s = ed.getText().toStdString();
        setValueString(s);
        triggerMenuItem();
    }
    void textEditorEscapeKeyPressed(juce::TextEditor &) override { triggerMenuItem(); }
};

void PluginEditor::popupMenuForContinuous(jcmp::ContinuousParamEditor *e)
{
    auto data = e->continuous();
    if (!data)
    {
        return;
    }

    if (!e->isEnabled())
    {
        return;
    }

    auto p = juce::PopupMenu();
    p.addSectionHeader(data->getLabel());
    p.addSeparator();
    p.addCustomItem(-1, std::make_unique<MenuValueTypein>(*this, juce::Component::SafePointer(e)));
    p.addSeparator();
    p.addItem("Set to Default",
              [w = juce::Component::SafePointer(e)]()
              {
                  if (!w)
                      return;
                  w->continuous()->setValueFromGUI(w->continuous()->getDefaultValue());
                  w->repaint();
              });

    // I could also stick the param id onto the component properties I guess
    auto pid = sst::jucegui::component_adapters::getClapParamId(e);
    if (pid.has_value())
    {
        sst::clap_juce_shim::populateMenuForClapParam(p, *pid, clapHost);
    }

    p.showMenuAsync(juce::PopupMenu::Options().withParentComponent(this));
}

void PluginEditor::setAndSendParamValue(uint32_t paramId, float value, bool notifyAudio,
                                        bool sendBeginEnd)
{
    patchCopy.paramMap[paramId]->value = value;

    auto rit = componentRefreshByID.find(paramId);
    if (rit != componentRefreshByID.end())
    {
        rit->second();
    }

    auto pit = componentByID.find(paramId);
    if (pit != componentByID.end() && pit->second)
        pit->second->repaint();

    if (notifyAudio)
    {
        if (sendBeginEnd)
            mainToAudio.push({Engine::MainToAudioMsg::Action::BEGIN_EDIT, paramId});
        mainToAudio.push({Engine::MainToAudioMsg::Action::SET_PARAM, paramId, value});
        if (sendBeginEnd)
            mainToAudio.push({Engine::MainToAudioMsg::Action::END_EDIT, paramId});
        requestParamsFlush();
    }
}

void PluginEditor::postPatchChange(const std::string &s)
{
    memset(patchCopy.name, 0, sizeof(patchCopy.name));
    strncpy(patchCopy.name, s.c_str(), 255);
    for (auto [id, f] : componentRefreshByID)
        f();

    repaint();
}

bool PluginEditor::keyPressed(const juce::KeyPress &key) { return false; }

void PluginEditor::visibilityChanged() {}

void PluginEditor::parentHierarchyChanged()
{
#if JUCE_WINDOWS
    auto swr = defaultsProvider->getUserDefaultValue(Defaults::useSoftwareRenderer, false);

    if (swr)
    {
        if (auto peer = getPeer())
        {
            SQLOG("Enabling software rendering engine");
            peer->setCurrentRenderingEngine(0); // 0 for software mode, 1 for Direct2D mode
        }
    }
#endif
}

void PluginEditor::setSkinFromDefaults()
{
    auto b = defaultsProvider->getUserDefaultValue(Defaults::useLightSkin, 0);
    setStyle(sst::jucegui::style::StyleSheet::getBuiltInStyleSheet(
        b ? sst::jucegui::style::StyleSheet::LIGHT : sst::jucegui::style::StyleSheet::DARK));
}

void PluginEditor::setZoomFactor(float zf)
{
    zoomFactor = juce::jlimit(0.5f, 3.f, zf);
    setSize((int)std::lround((float)edWidth * zoomFactor),
            (int)std::lround((float)edHeight * zoomFactor));
    defaultsProvider->updateUserDefaultValue(Defaults::zoomLevel, (int)std::lround(zoomFactor * 100.f));
    if (onZoomChanged)
        onZoomChanged(zoomFactor);
}

void PluginEditor::showUiSizeMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader("UI Size");
    menu.addItem("Reset to 100%  (320 x 640)", true, std::fabs(zoomFactor - 1.f) < 0.02f,
                 [this]() { setZoomFactor(1.f); });
    menu.addSeparator();
    for (int scale : {75, 90, 100, 110, 125, 150, 200})
    {
        const int w = (int)std::lround((float)edWidth * scale / 100.f);
        const int h = (int)std::lround((float)edHeight * scale / 100.f);
        const bool ticked = std::fabs(zoomFactor * 100.f - (float)scale) < 2.f;
        menu.addItem(std::to_string(scale) + "%  (" + std::to_string(w) + " x " + std::to_string(h) +
                         ")",
                     true, ticked, [this, scale]() { setZoomFactor(scale * 0.01f); });
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withParentComponent(this));
}

void PluginEditor::requestParamsFlush()
{
    if (!clapParamsExtension)
        clapParamsExtension = static_cast<const clap_host_params_t *>(
            clapHost->get_extension(clapHost, CLAP_EXT_PARAMS));
    if (clapParamsExtension)
    {
        clapParamsExtension->request_flush(clapHost);
    }
}

void PluginEditor::sneakyStartupGrabFrom(Patch &other)
{
    for (auto &p : other.params)
    {
        patchCopy.paramMap.at(p->meta.id)->value = p->value;
    }
    strncpy(patchCopy.name, other.name, 255);
    postPatchChange(other.name);
}

bool PluginEditor::toggleDebug()
{
    if (debugLevel == 0)
    {
        sst::plugininfra::misc_platform::allocateConsole();
    }
    if (debugLevel <= 0)
        debugLevel = 1;
    else
        debugLevel = -1;
    SQLOG("Started debug session");
    SQLOG("If you are on windows and you close this window it may end your entire session");
    return debugLevel > 0;
}

void PluginEditor::onStyleChanged()
{
    jcmp::WindowPanel::onStyleChanged();
    if (lnf)
        lnf->setStyle(style());
}

} // namespace baconpaul::sidequest_ns::ui