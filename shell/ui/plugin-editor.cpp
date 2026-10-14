/*
 * Sabrina Reverb — Soundemote
 * DSP MIT (Walter Hackett). UI links JUCE (AGPLv3). See LICENSE.md.
 */

#include "plugin-editor.h"

#include <clap/ext/state.h>
#include <cmath>
#include <cstring>

#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef small
#undef small
#endif
#ifdef large
#undef large
#endif
#endif

#include "sst/clap_juce_shim/menu_helper.h"
#include "sst/plugininfra/misc_platform.h"
#include "sst/plugininfra/paths.h"

#include "patch-data-bindings.h"
#include "main-panel.h"

namespace soundemote::sabrina::ui
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
    screenSize = juce::jlimit(1, 5, defaultsProvider->getUserDefaultValue(Defaults::screenSize, 2));
    setTransform({});
    setSize(juce::roundToInt((float)edWidth * zoomFactor),
            juce::roundToInt((float)designHeight() * zoomFactor));
#define DEBUG_FOCUS 0
#if DEBUG_FOCUS
    focusDebugger = std::make_unique<sst::jucegui::accessibility::FocusDebugger>(*this);
    focusDebugger->setDoFocusDebug(true);
#endif
}
PluginEditor::~PluginEditor()
{
    // The title menu is a desktop window, so closing the plugin UI does not
    // destroy it. Drop it here.
    juce::PopupMenu::dismissAllActiveMenus();
    if (idleTimer)
        idleTimer->stopTimer();
    idleTimer.reset();
    onZoomChanged = nullptr;
    fileChooser.reset();
    componentRefreshByID.clear();
    componentByID.clear();
    mainToAudio.push({Engine::MainToAudioMsg::EDITOR_ATTACH_DETATCH, false});
    mainPanel.reset();
    setLookAndFeel(nullptr);
}

void PluginEditor::idle()
{
    float delayTimes[14]{};
    bool haveDelayMod = false;
    auto aum = audioToUI.pop();
    while (aum.has_value())
    {
        if (aum->action == Engine::AudioToUIMsg::UPDATE_PARAM)
        {
            setAndSendParamValue(aum->paramId, aum->value, false);
        }
        else if (aum->action == Engine::AudioToUIMsg::DELAY_MOD)
        {
            std::memcpy(delayTimes, aum->delayTimes, sizeof(delayTimes));
            haveDelayMod = true;
        }
        else if (aum->action == Engine::AudioToUIMsg::SET_PATCH_NAME)
        {
            memset(patchCopy.name, 0, sizeof(patchCopy.name));
            strncpy(patchCopy.name, aum->patchNamePointer, 255);
            if (mainPanel && mainPanel->face)
                mainPanel->face->titleBar.repaint();
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
    if (haveDelayMod && mainPanel && isShowing())
        mainPanel->pushDelayTimes(delayTimes);
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

void PluginEditor::visibilityChanged()
{
    if (!isShowing())
        juce::PopupMenu::dismissAllActiveMenus();
}

void PluginEditor::parentHierarchyChanged()
{
#if JUCE_WINDOWS
    if (!isShowing() || defaultsProvider == nullptr)
        return;
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
    setTransform({});
    setSize(juce::roundToInt((float)edWidth * zoomFactor),
            juce::roundToInt((float)designHeight() * zoomFactor));
    defaultsProvider->updateUserDefaultValue(Defaults::zoomLevel,
                                             juce::roundToInt(zoomFactor * 100.f));
    if (onZoomChanged)
        onZoomChanged(zoomFactor);
}

void PluginEditor::setScreenSize(int titleHeights)
{
    screenSize = juce::jlimit(1, 5, titleHeights);
    defaultsProvider->updateUserDefaultValue(Defaults::screenSize, screenSize);
    if (mainPanel && mainPanel->face)
        mainPanel->face->resized();
}

namespace
{
constexpr const char *kPresetKind = "module-settings";
constexpr const char *kPresetType = se::product::sandboxModule;

float jsonParamNumber(const juce::var &v, float fallback, const se::product::ParamDesc &d)
{
    if (v.isVoid() || v.isUndefined())
        return fallback;
    if (v.isString())
    {
        auto s = v.toString().trim().toLowerCase();
        for (int i = 0; i < d.numChoices; ++i)
            if (s == juce::String(d.choices[i].label).toLowerCase())
                return (float)d.choices[i].value;
        return s.getFloatValue();
    }
    return (float)(double)v;
}

bool isIntegerParam(const se::product::ParamDesc &d)
{
    return d.type == se::product::ParamType::Integer || d.type == se::product::ParamType::Choice;
}
} // namespace

juce::String PluginEditor::jsonPresetFromPatch() const
{
    auto *params = new juce::DynamicObject();
    for (size_t i = 0; i < se::product::numParams; ++i)
    {
        const auto &d = se::product::params[i];
        const auto &p = patchCopy.param(i);
        if (isIntegerParam(d))
            params->setProperty(d.key, (int)std::lround(p.value));
        else
            params->setProperty(d.key, p.value);
    }

    auto *root = new juce::DynamicObject();
    root->setProperty("soundemote", kPresetKind);
    root->setProperty("type", kPresetType);
    root->setProperty("version", 1);
    root->setProperty("params", juce::var(params));
    const auto title = displayTitle();
    if (title != PRODUCT_NAME)
        root->setProperty("alias", title);
    return juce::JSON::toString(juce::var(root), true);
}

bool PluginEditor::applyJsonPreset(const juce::String &text)
{
    auto parsed = juce::JSON::parse(text);
    if (!parsed.isObject())
        return false;
    if (parsed["soundemote"].toString() != kPresetKind)
        return false;
    const auto type = parsed["type"].toString();
    if (type != kPresetType && type != PRODUCT_NAME)
        return false;
    auto params = parsed["params"];
    auto *obj = params.getDynamicObject();
    if (!obj)
        return false;

    for (size_t i = 0; i < se::product::numParams; ++i)
    {
        const auto &d = se::product::params[i];
        auto &p = patchCopy.param(i);
        if (obj->hasProperty(d.key))
            setAndSendParamValue(p.meta.id, jsonParamNumber(obj->getProperty(d.key), p.value, d));
    }
    auto *root = parsed.getDynamicObject();
    if (root && root->hasProperty("alias"))
        setModuleTitle(root->getProperty("alias").toString());
    return true;
}

juce::String PluginEditor::displayTitle() const
{
    const juce::String name{patchCopy.name};
    if (name.isEmpty() || name == "Init")
        return PRODUCT_NAME;
    return name;
}

void PluginEditor::setModuleTitle(const juce::String &raw)
{
    auto trimmed = raw.trim();
    const std::string store =
        (trimmed.isEmpty() || trimmed == PRODUCT_NAME) ? std::string("Init") : trimmed.toStdString();
    if (store == patchCopy.name)
        return;

    static char slots[8][256];
    static int slot = 0;
    char *buf = slots[slot];
    slot = (slot + 1) % 8;
    std::memset(buf, 0, sizeof(slots[0]));
    std::strncpy(buf, store.c_str(), 255);

    std::memset(patchCopy.name, 0, sizeof(patchCopy.name));
    std::strncpy(patchCopy.name, buf, 255);
    mainToAudio.push({Engine::MainToAudioMsg::SEND_PATCH_NAME, 0, 0.f, buf});
    if (mainPanel && mainPanel->face)
        mainPanel->face->titleBar.repaint();
    if (clapHost)
    {
        if (auto *state = static_cast<const clap_host_state_t *>(
                clapHost->get_extension(clapHost, CLAP_EXT_STATE)))
            state->mark_dirty(clapHost);
    }
}

void PluginEditor::copyJsonPreset()
{
    juce::SystemClipboard::copyTextToClipboard(jsonPresetFromPatch());
}

void PluginEditor::pasteJsonPreset()
{
    applyJsonPreset(juce::SystemClipboard::getTextFromClipboard());
}

void PluginEditor::saveJsonPreset()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save preset", juce::File(),
                                                      "*.json");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode |
                                 juce::FileBrowserComponent::canSelectFiles |
                                 juce::FileBrowserComponent::warnAboutOverwriting,
                             [safe = juce::Component::SafePointer(this)](const juce::FileChooser &chooser)
                             {
                                 if (!safe)
                                     return;
                                 auto f = chooser.getResult();
                                 if (f == juce::File{})
                                     return;
                                 if (f.getFileExtension().isEmpty())
                                     f = f.withFileExtension("json");
                                 f.replaceWithText(safe->jsonPresetFromPatch());
                             });
}

void PluginEditor::loadJsonPreset()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load preset", juce::File(),
                                                      "*.json");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
                                 juce::FileBrowserComponent::canSelectFiles,
                             [safe = juce::Component::SafePointer(this)](const juce::FileChooser &chooser)
                             {
                                 if (!safe)
                                     return;
                                 auto f = chooser.getResult();
                                 if (f == juce::File{})
                                     return;
                                 safe->applyJsonPreset(f.loadFileAsString());
                             });
}

bool PluginEditor::isStandaloneHost() const
{
    return clapHost != nullptr && clapHost->name != nullptr &&
           std::strcmp(clapHost->name, "CLAP-Wrapper-As-Standalone") == 0;
}

void PluginEditor::openStandaloneAudioSettings()
{
#if JUCE_WINDOWS
    if (auto *peer = getPeer())
    {
        auto hwnd = static_cast<HWND>(peer->getNativeHandle());
        auto root = ::GetAncestor(hwnd, GA_ROOT);
        if (root == nullptr)
            root = hwnd;
        // clap-wrapper standalone: first system-menu command is Audio/MIDI Settings.
        ::PostMessageW(root, WM_SYSCOMMAND, 0, 0);
    }
#endif
}

void PluginEditor::showUiSizeMenu()
{
    auto safe = juce::Component::SafePointer(this);
    juce::PopupMenu menu;
    if (isStandaloneHost())
    {
        menu.addItem("Audio Settings", [safe]() {
            if (safe)
                safe->openStandaloneAudioSettings();
        });
        menu.addSeparator();
    }
    menu.addSectionHeader("Preset");
    menu.addItem("Copy preset", [safe]() { if (safe) safe->copyJsonPreset(); });
    menu.addItem("Paste preset", [safe]() { if (safe) safe->pasteJsonPreset(); });
    menu.addItem("Load preset...", [safe]() { if (safe) safe->loadJsonPreset(); });
    menu.addItem("Save preset...", [safe]() { if (safe) safe->saveJsonPreset(); });
    menu.addSeparator();
    menu.addSectionHeader("UI Size");
    menu.addItem("Reset to 100%  (" + std::to_string((int)edWidth) + " x " +
                     std::to_string((int)edHeight) +
                     ")",
                 true, std::fabs(zoomFactor - 1.f) < 0.02f,
                 [safe]() { if (safe) safe->setZoomFactor(1.f); });
    menu.addSeparator();
    for (int scale : {75, 90, 100, 110, 125, 150, 200})
    {
        const int w = (int)std::lround((float)edWidth * scale / 100.f);
        const int h = (int)std::lround((float)edHeight * scale / 100.f);
        const bool ticked = std::fabs(zoomFactor * 100.f - (float)scale) < 2.f;
        menu.addItem(std::to_string(scale) + "%  (" + std::to_string(w) + " x " + std::to_string(h) +
                         ")",
                     true, ticked,
                     [safe, scale]() { if (safe) safe->setZoomFactor(scale * 0.01f); });
    }

    menu.addSeparator();
    menu.addSectionHeader("Display Size");
    for (int n = 1; n <= 5; ++n)
    {
        const auto label = std::to_string(n) + (n == 1 ? " title height" : " title heights") +
                           (n == 2 ? "  (default)" : "");
        menu.addItem(label, true, screenSize == n,
                     [safe, n]() { if (safe) safe->setScreenSize(n); });
    }
    menu.addSeparator();
    menu.addItem("soundemote.com/sabrinareverb", [] {
        juce::URL("https://soundemote.com/sabrinareverb").launchInDefaultBrowser();
    });
#if JUCE_DEBUG
    const char *buildConfig = "DEBUG";
#else
    const char *buildConfig = "RELEASE";
#endif
    menu.addItem(juce::String(PRODUCT_VERSION) + " " + buildConfig, false, false, [] {});
    // Desktop popup — not parented to the editor, so UI size does not scale the menu.
    // Closed in the destructor and when the editor stops showing.
    menu.showMenuAsync(juce::PopupMenu::Options());
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

} // namespace soundemote::sabrina::ui