#include "sabrina-face.h"

namespace baconpaul::sidequest_ns::ui
{

namespace
{
const juce::Colour kModuleFill{0xff171a1f}; // sandbox --node-module-fill
const juce::Colour kFaceBg{0xff080b0d};
const juce::Colour kFaceLine{0xff343a40};
const juce::Colour kAmount{0x8c1b3545};     // hsl(200 31% 15% / 0.55)
const juce::Colour kPosition{0x5e44a4ff};   // rgb(68 164 255 / 0.37)
const juce::Colour kLabel{0xebcfdde5};
const juce::Colour kValue{0xffffffff};
const juce::Colour kUnit{0x947fc7d9};

juce::Font monoFont(float height)
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), height,
                                        juce::Font::plain));
}
} // namespace

SabrinaSlider::SabrinaSlider(PluginEditor &e, uint32_t paramId, juce::String unitText,
                             juce::String faceLabelText)
    : editor(e), unit(std::move(unitText)), faceLabel(std::move(faceLabelText))
{
    source = std::make_unique<PatchContinuous>(e, paramId);
    e.componentRefreshByID[paramId] = [this]() { repaint(); };
}

float SabrinaSlider::value01() const
{
    auto min = source->getMin();
    auto max = source->getMax();
    auto span = max - min;
    if (span <= 0.f)
        return 0.f;
    return juce::jlimit(0.f, 1.f, (source->getValue() - min) / span);
}

float SabrinaSlider::fineScale(const juce::ModifierKeys &m) const
{
    const bool shift = m.isShiftDown();
    const bool cmd = m.isCtrlDown() || m.isCommandDown();
    if (shift && cmd)
        return 0.01f;
    if (shift || cmd)
        return 0.1f;
    return 1.f;
}

void SabrinaSlider::applyValue(float v)
{
    v = juce::jlimit(source->getMin(), source->getMax(), v);
    if (source->p && source->p->meta.type == sst::basic_blocks::params::ParamMetaData::INT)
        v = std::round(v);
    source->setValueFromGUI(v);
    editor.requestParamsFlush();
    repaint();
}

void SabrinaSlider::applyFromX(int x)
{
    auto w = (float)getWidth();
    if (w <= 1.f)
        return;
    auto t = juce::jlimit(0.f, 1.f, x / w);
    applyValue(source->getMin() + t * (source->getMax() - source->getMin()));
}

void SabrinaSlider::beginGesture()
{
    if (gestureOpen)
        return;
    editor.mainToAudio.push({Engine::MainToAudioMsg::Action::BEGIN_EDIT, source->pid});
    gestureOpen = true;
}

void SabrinaSlider::endGesture()
{
    if (!gestureOpen)
        return;
    editor.mainToAudio.push({Engine::MainToAudioMsg::Action::END_EDIT, source->pid});
    gestureOpen = false;
}

void SabrinaSlider::resized()
{
    if (typeIn)
        typeIn->setBounds(getLocalBounds().reduced(2));
}

void SabrinaSlider::startTypeIn()
{
    cancelTypeIn();
    typeIn = std::make_unique<juce::TextEditor>();
    typeIn->setFont(monoFont(editor.px((float)kValueFontPx)));
    typeIn->setJustification(juce::Justification::centredLeft);
    typeIn->setColour(juce::TextEditor::backgroundColourId, kFaceBg);
    typeIn->setColour(juce::TextEditor::textColourId, kValue);
    typeIn->setColour(juce::TextEditor::highlightColourId, kPosition);
    typeIn->setColour(juce::TextEditor::outlineColourId, kFaceLine);
    typeIn->setColour(juce::TextEditor::focusedOutlineColourId, kPosition);
    typeIn->setText(source->getValueAsString(), juce::dontSendNotification);
    typeIn->addListener(this);
    addAndMakeVisible(*typeIn);
    typeIn->setBounds(getLocalBounds().reduced(2));
    typeIn->grabKeyboardFocus();
    typeIn->selectAll();
}

void SabrinaSlider::commitTypeIn()
{
    if (!typeIn)
        return;
    auto text = typeIn->getText().toStdString();
    beginGesture();
    if (text.empty())
        applyValue(source->getDefaultValue());
    else
        source->setValueAsString(text);
    editor.requestParamsFlush();
    endGesture();
    cancelTypeIn();
    repaint();
}

void SabrinaSlider::cancelTypeIn()
{
    if (!typeIn)
        return;
    typeIn->removeListener(this);
    removeChildComponent(typeIn.get());
    typeIn.reset();
}

void SabrinaSlider::textEditorReturnKeyPressed(juce::TextEditor &) { commitTypeIn(); }
void SabrinaSlider::textEditorEscapeKeyPressed(juce::TextEditor &) { cancelTypeIn(); }
void SabrinaSlider::textEditorFocusLost(juce::TextEditor &) { commitTypeIn(); }

void SabrinaSlider::paint(juce::Graphics &g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour(kFaceBg);
    g.fillRoundedRectangle(r, editor.px(3.f));

    auto t = value01();
    auto fill = r.withWidth(r.getWidth() * t);
    g.setColour(kAmount);
    g.fillRect(fill);
    g.setColour(kPosition);
    const float bar = editor.px(2.f);
    g.fillRect(fill.withX(fill.getRight() - bar).withWidth(bar));

    auto text = r.withTrimmedLeft(editor.px(kTextPadPx)).withTrimmedRight(editor.px(kTextPadPx));

    juce::String valueText(source->getValueAsString());
    if (unit.isNotEmpty())
        valueText += " " + unit;

    juce::String label = faceLabel.isNotEmpty() ? faceLabel : juce::String(source->getLabel());
    g.setColour(kLabel);
    g.setFont(monoFont(editor.px((float)kLabelFontPx)));
    if (label.containsChar('\n'))
    {
        auto line1 = label.upToFirstOccurrenceOf("\n", false, false);
        auto line2 = label.fromFirstOccurrenceOf("\n", false, false);
        auto top = text.withHeight(text.getHeight() * 0.5f);
        auto bottom = text.withY(top.getBottom()).withHeight(text.getHeight() * 0.5f);
        g.drawText(line1, top, juce::Justification::centredLeft, false);
        g.drawText(line2, bottom, juce::Justification::centredLeft, false);
    }
    else
    {
        g.drawText(label, text, juce::Justification::centredLeft, false);
    }

    g.setColour(kValue);
    g.setFont(monoFont(editor.px((float)kValueFontPx)));
    g.drawText(valueText, text, juce::Justification::centredRight, false);
}

void SabrinaSlider::mouseDown(const juce::MouseEvent &e)
{
    if (typeIn)
        commitTypeIn();

    const auto mods = e.mods;
    if (e.getNumberOfClicks() >= 2 && !mods.isAltDown())
    {
        startTypeIn();
        return;
    }

    // ctrl/cmd+click (no alt/shift) = reset default
    if ((mods.isCtrlDown() || mods.isCommandDown()) && !mods.isAltDown() && !mods.isShiftDown())
    {
        beginGesture();
        applyValue(source->getDefaultValue());
        endGesture();
        return;
    }

    beginGesture();
    if (mods.isAltDown() && !(mods.isShiftDown() && (mods.isCtrlDown() || mods.isCommandDown())))
        applyFromX(e.x);

    dragging = true;
    dragStartX = e.x;
    dragStartY = e.y;
    dragStartValue = source->getValue();
    dragFineScale = fineScale(mods);
}

void SabrinaSlider::mouseDrag(const juce::MouseEvent &e)
{
    if (!dragging)
        return;

    const auto scale = fineScale(e.mods);
    if (scale != dragFineScale)
    {
        dragStartX = e.x;
        dragStartY = e.y;
        dragStartValue = source->getValue();
        dragFineScale = scale;
    }

    const float w = (float)juce::jmax(1, getWidth());
    // Sandbox: right and up increase; combined = dx + (-dy).
    const float travel = ((float)(e.x - dragStartX) + (float)(dragStartY - e.y)) / w;
    const float span = source->getMax() - source->getMin();
    applyValue(dragStartValue + travel * span * scale);
}

void SabrinaSlider::mouseUp(const juce::MouseEvent &)
{
    dragging = false;
    endGesture();
}

void SabrinaSlider::mouseDoubleClick(const juce::MouseEvent &e)
{
    if (e.mods.isAltDown())
        return;
    startTypeIn();
}

void SabrinaSlider::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w)
{
    const float span = source->getMax() - source->getMin();
    const float delta = (w.deltaY > 0 ? 1.f : -1.f) * span * 0.01f * fineScale(e.mods);
    beginGesture();
    applyValue(source->getValue() + delta);
    endGesture();
}

void SabrinaWaterfall::ensurePlate()
{
    auto w = juce::jmax(8, getWidth());
    auto h = juce::jmax(8, getHeight());
    if (plate.getWidth() != w || plate.getHeight() != h)
    {
        plate = juce::Image(juce::Image::RGB, w, h, true);
        plate.clear(plate.getBounds(), juce::Colours::black);
    }
}

void SabrinaWaterfall::resized() { ensurePlate(); }

void SabrinaWaterfall::pushPeaks(float left, float right)
{
    ensurePlate();
    if (plate.isNull())
        return;

    plate.moveImageSection(0, 0, 1, 0, plate.getWidth() - 1, plate.getHeight());
    auto g = juce::Graphics(plate);
    auto x = plate.getWidth() - 1;
    auto mid = plate.getHeight() / 2;
    auto l = juce::jlimit(0.f, 1.f, std::abs(left));
    auto r = juce::jlimit(0.f, 1.f, std::abs(right));
    g.setColour(juce::Colours::black);
    g.fillRect(x, 0, 1, plate.getHeight());
    g.setColour(juce::Colour::fromFloatRGBA(0.15f + 0.85f * l, 0.55f * l, 0.12f, 1.f));
    auto lh = juce::jmax(1, (int)std::round(l * (float)mid));
    g.fillRect(x, mid - lh, 1, lh);
    g.setColour(juce::Colour::fromFloatRGBA(0.12f, 0.45f * r, 0.15f + 0.85f * r, 1.f));
    auto rh = juce::jmax(1, (int)std::round(r * (float)(plate.getHeight() - mid)));
    g.fillRect(x, mid, 1, rh);
}

void SabrinaWaterfall::paint(juce::Graphics &g)
{
    ensurePlate();
    g.fillAll(juce::Colours::black);
    if (!plate.isNull())
        g.drawImage(plate, getLocalBounds().toFloat());
    g.setColour(juce::Colour(0xff1c2228));
    g.drawRect(getLocalBounds(), 1);
    auto mid = getHeight() / 2;
    g.setColour(juce::Colour(0x3322aaff));
    g.drawHorizontalLine(mid, 0.f, (float)getWidth());
}

void SabrinaTitleBar::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient grad(juce::Colour(0xff020407), 0.f, 0.f, juce::Colour(0xeb080a0d), 0.f,
                              bounds.getHeight(), false);
    g.setGradientFill(grad);
    g.fillRect(bounds);

    const juce::String title{PRODUCT_NAME};
    const float titlePx = editor ? editor->px((float)kTitleFontPx) : (float)kTitleFontPx;
    g.setColour(juce::Colour(0xc2f3f1ec));
    g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), titlePx,
                                           juce::Font::plain)));
    g.drawText(title, getLocalBounds(), juce::Justification::centred, false);
}

void SabrinaTitleBar::mouseUp(const juce::MouseEvent &e)
{
    if (e.mods.isPopupMenu() && editor)
        editor->showUiSizeMenu();
}

void SabrinaWaterfall::mouseUp(const juce::MouseEvent &e)
{
    if (e.mods.isPopupMenu() && editor)
        editor->showUiSizeMenu();
}

SabrinaFace::SabrinaFace(PluginEditor &e) : editor(e)
{
    titleBar.editor = &e;
    waterfall.editor = &e;
    addAndMakeVisible(titleBar);
    addAndMakeVisible(waterfall);

    struct Row
    {
        uint32_t id;
        const char *unit;
        const char *faceLabel;
    };
    const Row rows[] = {
        {e.patchCopy.sabrina.mix.meta.id, "", nullptr},
        {e.patchCopy.sabrina.diffusionSize.meta.id, "", "Diffusion\nSize"},
        {e.patchCopy.sabrina.diffusionAmount.meta.id, "", "Diffusion\nAmount"},
        {e.patchCopy.sabrina.delaySize.meta.id, "", "Delay\nTime"},
        {e.patchCopy.sabrina.recycle.meta.id, "", nullptr},
        {e.patchCopy.sabrina.lfoAmplitude.meta.id, "", nullptr},
        {e.patchCopy.sabrina.lfoBaseSpeed.meta.id, "", nullptr},
        {e.patchCopy.sabrina.lfoVariation.meta.id, "", nullptr},
        {e.patchCopy.sabrina.lfoStyle.meta.id, "", nullptr},
        {e.patchCopy.sabrina.seed.meta.id, "", nullptr},
        {e.patchCopy.sabrina.amplitude.meta.id, "", nullptr},
    };
    for (auto &row : rows)
    {
        sliders.push_back(std::make_unique<SabrinaSlider>(
            e, row.id, row.unit, row.faceLabel ? juce::String(row.faceLabel) : juce::String()));
        addAndMakeVisible(*sliders.back());
    }
}

void SabrinaFace::paint(juce::Graphics &g) { g.fillAll(kModuleFill); }

void SabrinaFace::pushPeaks(float left, float right)
{
    waterfall.pushPeaks(left, right);
    waterfall.repaint();
}

void SabrinaFace::resized()
{
    auto r = getLocalBounds();
    const int titleH = editor.px(kTitleBarHeight);
    titleBar.setBounds(r.removeFromTop(titleH));
    const int displayH = titleH * juce::jlimit(1, 5, editor.screenSize);
    waterfall.setBounds(r.removeFromTop(juce::jmin(displayH, r.getHeight())));
    if (sliders.empty())
        return;
    auto rowH = r.getHeight() / (int)sliders.size();
    for (auto &s : sliders)
        s->setBounds(r.removeFromTop(rowH));
}

} // namespace baconpaul::sidequest_ns::ui
