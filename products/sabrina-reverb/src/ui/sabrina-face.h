#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#ifdef small
#undef small
#endif
#include "ui/patch-data-bindings.h"
#include "ui/plugin-editor.h"
#include "product/product-face.h"

namespace soundemote::sabrina::ui
{
// soemdsp-sandbox module chrome at 100% (grid 28). Scale with PluginEditor::px.
static constexpr int kGrid{28};
static constexpr int kTitleBarHeight{28};
static constexpr int kSliderHeight{30};
static constexpr int kBodyGap{2};
static constexpr int kInset{6};
static constexpr int kBorder{2};
static constexpr int kSliderPadX{7};
static constexpr int kHandle{16};
static constexpr int kUnitMin{35};
static constexpr int kLabelFontPx{13};
static constexpr int kValueFontPx{14};
static constexpr int kUnitFontPx{13};
static constexpr int kTitleFontPx{20};

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
    juce::String faceLabel;

    SabrinaSlider(PluginEditor &e, uint32_t paramId, juce::String unitText,
                  juce::String faceLabelText = {});
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
    int choiceCount() const;

    bool dragging{false};
    bool gestureOpen{false};
    int dragStartX{0};
    int dragStartY{0};
    float dragStartValue{0.f};
    float dragFineScale{1.f};
    std::unique_ptr<juce::TextEditor> typeIn;
};

struct SabrinaWaterfall : juce::Component, private juce::OpenGLRenderer
{
    PluginEditor *editor{nullptr};
    SabrinaWaterfall();
    ~SabrinaWaterfall() override;
    void pushDelayTimes(const float times[14]);
    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent &e) override;
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

  private:
    juce::OpenGLContext glContext;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> colourUniform;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> halfUniform;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> positionAttrib;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> distAttrib;
    std::array<std::atomic<float>, 14> stemX{};
    std::atomic<int> viewW{1};
    std::atomic<int> viewH{1};
    unsigned int vao{0};
    unsigned int vbo{0};
    std::atomic<bool> glOk{false};
    void drawLine(float x01, float halfPx, int viewW, float r, float g, float b);
    void paintLines(juce::Graphics &g);
};

struct SabrinaTitleBar : juce::Component, juce::TextEditor::Listener
{
    PluginEditor *editor{nullptr};
    void paint(juce::Graphics &g) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDoubleClick(const juce::MouseEvent &e) override;
    void textEditorReturnKeyPressed(juce::TextEditor &) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &) override;
    void textEditorFocusLost(juce::TextEditor &) override;
    void textEditorTextChanged(juce::TextEditor &) override;

  private:
    std::unique_ptr<juce::TextEditor> typeIn;
    bool committing{false};
    void beginEdit();
    void commitEdit();
    void cancelEdit();
    void fitEditFont();
};

struct SabrinaFace : se::product::Face
{
    PluginEditor &editor;
    SabrinaFace(PluginEditor &e);

    void resized() override;
    void paint(juce::Graphics &g) override;
    void pushDelayTimes(const float times[14]);
    void pushVizData(const float *values, size_t count) override;
    void patchNameChanged() override;

    SabrinaTitleBar titleBar;
    SabrinaWaterfall waterfall;
    std::vector<std::unique_ptr<SabrinaSlider>> sliders;
};

} // namespace soundemote::sabrina::ui
