#pragma once

#include <array>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include "patch-data-bindings.h"
#include "plugin-editor.h"

namespace baconpaul::sidequest_ns::ui
{

// Sandbox-style module slider: full width, label / number / unit, amount fill.
// Modifiers match soemdsp-sandbox node-graph-slider-dragging.js:
//   click-drag            relative (no jump)
//   alt+click             jump to pointer
//   shift / ctrl / cmd    fine (0.1x; both 0.01x)
//   ctrl/cmd+click        reset default
//   double-click          type-in
struct SabrinaSlider : juce::Component, juce::TextEditor::Listener
{
    PluginEditor &editor;
    std::unique_ptr<PatchContinuous> source;
    juce::String unit;

    SabrinaSlider(PluginEditor &e, uint32_t paramId, juce::String unitText);
    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDoubleClick(const juce::MouseEvent &e) override;
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w) override;

    void textEditorReturnKeyPressed(juce::TextEditor &) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &) override;
    void textEditorFocusLost(juce::TextEditor &) override;

  private:
    void applyFromX(int x);
    void applyValue(float v);
    void beginGesture();
    void endGesture();
    void startTypeIn();
    void commitTypeIn();
    void cancelTypeIn();
    float value01() const;
    float fineScale(const juce::ModifierKeys &m) const;

    bool dragging{false};
    bool gestureOpen{false};
    int dragStartX{0};
    int dragStartY{0};
    float dragStartValue{0.f};
    float dragFineScale{1.f};
    std::unique_ptr<juce::TextEditor> typeIn;
};

struct SabrinaWaterfall : juce::Component
{
    PluginEditor *editor{nullptr};
    void pushPeaks(float left, float right);
    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent &e) override;

  private:
    juce::Image plate;
    void ensurePlate();
};

struct SabrinaTitleBar : juce::Component
{
    PluginEditor *editor{nullptr};
    void paint(juce::Graphics &g) override;
    void mouseUp(const juce::MouseEvent &e) override;
};

struct SabrinaFace : juce::Component
{
    PluginEditor &editor;
    SabrinaFace(PluginEditor &e);

    void resized() override;
    void paint(juce::Graphics &g) override;
    void pushPeaks(float left, float right);

    SabrinaTitleBar titleBar;
    SabrinaWaterfall waterfall;
    std::vector<std::unique_ptr<SabrinaSlider>> sliders;
};

} // namespace baconpaul::sidequest_ns::ui
