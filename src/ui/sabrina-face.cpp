#include "sabrina-face.h"

#include <cmath>
#include <unordered_map>

namespace soundemote::sabrina::ui
{

namespace
{
const juce::Colour kModuleFill{0xff171a1f}; // sandbox --node-module-fill
const juce::Colour kFaceBg{0xff080b0d};
const juce::Colour kFaceLine{0xff343a40};
const juce::Colour kPosition{0x5e44a4ff}; // rgb(68 164 255 / 0.37)
const juce::Colour kLabel{0xffcfdde5};
const juce::Colour kValue{0xffffffff};
const juce::Colour kUnit{0x947fc7d9}; // #7fc7d9 @ ~58%
const juce::Colour kTitle{0xc2f3f1ec}; // rgba(243,241,236,0.76)
const juce::Colour kChoiceDiv{0x33f3f1ec};

juce::Font sandboxMono(float height)
{
    return juce::Font(juce::FontOptions("Cascadia Mono", height, juce::Font::plain)
                         .withFallbacks({"Cascadia Code", "Consolas", "Courier New"})
                         .withFallbackEnabled());
}

constexpr int kDelayPlateW{512};
constexpr int kDelayPlateH{128};
constexpr float kStemCssPx{2.f};
constexpr float kAaCssPx{1.f};
constexpr float kFaceDesignW{234.f};
constexpr float kTrail{0.3f};
constexpr float kGhost{0.25f};

std::unordered_map<const void *, juce::Image> gDelayPlates;

juce::Image &delayPlateFor(PluginEditor &editor)
{
    auto &img = gDelayPlates[static_cast<const void *>(&editor.audioToUI)];
    if (img.isNull() || img.getWidth() != kDelayPlateW || img.getHeight() != kDelayPlateH)
    {
        img = juce::Image(juce::Image::RGB, kDelayPlateW, kDelayPlateH, true);
        img.clear(img.getBounds(), juce::Colour(0xff000004));
    }
    return img;
}
} // namespace

SabrinaSlider::SabrinaSlider(PluginEditor &e, uint32_t paramId, juce::String unitText,
                             juce::String faceLabelText)
    : editor(e), unit(std::move(unitText)), faceLabel(std::move(faceLabelText))
{
    source = std::make_unique<PatchContinuous>(e, paramId);
    e.componentRefreshByID[paramId] = [this]() { repaint(); };
}

int SabrinaSlider::choiceCount() const
{
    if (!source->p)
        return 0;
    if (source->p->meta.type != sst::basic_blocks::params::ParamMetaData::INT)
        return 0;
    const int n = (int)std::lround(source->getMax() - source->getMin()) + 1;
    if (n < 2 || n > 16)
        return 0;
    return n;
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
    {
        auto r = getLocalBounds();
        r.removeFromTop(r.getHeight() / 2);
        r.removeFromRight(unit.isNotEmpty() ? editor.px(kUnitMin) : 0);
        typeIn->setBounds(r.reduced(editor.px(kSliderPadX), 0));
    }
}

void SabrinaSlider::startTypeIn()
{
    cancelTypeIn();
    typeIn = std::make_unique<juce::TextEditor>();
    typeIn->setFont(sandboxMono(editor.px((float)kValueFontPx)));
    typeIn->setJustification(juce::Justification::centredLeft);
    typeIn->setColour(juce::TextEditor::backgroundColourId, kFaceBg);
    typeIn->setColour(juce::TextEditor::textColourId, kValue);
    typeIn->setColour(juce::TextEditor::highlightColourId, kPosition);
    typeIn->setColour(juce::TextEditor::outlineColourId, kFaceLine);
    typeIn->setColour(juce::TextEditor::focusedOutlineColourId, kPosition);
    typeIn->setText(source->getValueAsString(), juce::dontSendNotification);
    typeIn->addListener(this);
    addAndMakeVisible(*typeIn);
    auto text = getLocalBounds().removeFromBottom(juce::jmax(1, getHeight() / 2));
    text.removeFromRight(unit.isNotEmpty() ? editor.px(kUnitMin) : 0);
    typeIn->setBounds(text.reduced(editor.px(kSliderPadX), 0));
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
    g.fillRect(r);

    const int choices = choiceCount();
    if (choices > 0)
    {
        const int sel = juce::jlimit(
            0, choices - 1,
            (int)std::lround(source->getValue() - source->getMin()));
        const float cell = r.getWidth() / (float)choices;
        g.setColour(kPosition);
        g.fillRect(juce::Rectangle<float>(r.getX() + (float)sel * cell, r.getY(), cell, r.getHeight()));
        g.setColour(kChoiceDiv);
        for (int i = 1; i < choices; ++i)
            g.fillRect(juce::Rectangle<float>(r.getX() + (float)i * cell, r.getY(), 1.f, r.getHeight()));
    }
    else
    {
        const float t = value01();
        const float half = juce::jmin(editor.px((float)kHandle) * 0.5f, r.getWidth() * 0.5f);
        const float travel = juce::jmax(1.f, r.getWidth() - half * 2.f);
        const float center = r.getX() + half + t * travel;
        g.setColour(kPosition);
        g.fillRect(juce::Rectangle<float>(center - half, r.getY(), half * 2.f, r.getHeight()));
    }

    const float padX = editor.px((float)kSliderPadX);
    auto inner = r.reduced(padX, 0.f);
    const float unitW = unit.isNotEmpty() ? editor.px((float)kUnitMin) : 0.f;
    const float fontH = juce::jmax(8.f, r.getHeight() * 0.46f);
    const float pairH = fontH * 2.f;
    const float y0 = inner.getY() + (inner.getHeight() - pairH) * 0.5f;
    auto labelR = juce::Rectangle<float>(inner.getX(), y0, inner.getWidth(), fontH);
    auto valueR = juce::Rectangle<float>(inner.getX(), y0 + fontH, inner.getWidth() - unitW, fontH);
    auto unitR = juce::Rectangle<float>(inner.getRight() - unitW, inner.getY(), unitW, inner.getHeight());

    juce::String label = faceLabel.isNotEmpty() ? faceLabel : juce::String(source->getLabel());
    label = label.replaceCharacter('\n', ' ');
    juce::String valueText(source->getValueAsString());
    if (unit.isNotEmpty())
        valueText = valueText.replace(unit, "").trim();

    auto font = sandboxMono(fontH);
    g.setFont(font);
    g.setColour(kLabel);
    g.drawText(label, labelR, juce::Justification::centredLeft, false);
    g.setColour(kValue);
    g.drawText(valueText, valueR, juce::Justification::centredLeft, false);
    if (unit.isNotEmpty())
    {
        g.setFont(sandboxMono(juce::jmax(8.f, r.getHeight() * 0.5f)));
        g.setColour(kUnit);
        g.drawText(unit, unitR, juce::Justification::centredRight, false);
    }
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

    // Reaper uses Alt+click to close the FX window. Do not start a gesture.
    if (mods.isAltDown())
        return;

    // ctrl/cmd+click (no shift) = reset default
    if ((mods.isCtrlDown() || mods.isCommandDown()) && !mods.isShiftDown())
    {
        beginGesture();
        applyValue(source->getDefaultValue());
        endGesture();
        return;
    }

    beginGesture();
    if (choiceCount() > 0)
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
    if (choiceCount() > 0)
    {
        applyFromX(e.x);
        return;
    }

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

void SabrinaWaterfall::pushDelayTimes(const float times[14])
{
    if (!editor)
        return;

    auto &plate = delayPlateFor(*editor);
    const int w = plate.getWidth();
    const int h = plate.getHeight();
    juce::Image::BitmapData bits(plate, juce::Image::BitmapData::readWrite);

    const float keep = juce::jlimit(0.f, 0.995f, kTrail * 0.97f + kGhost * 0.02f);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const auto c = bits.getPixelColour(x, y);
            bits.setPixelColour(x, y,
                                juce::Colour::fromRGB((juce::uint8)(c.getRed() * keep),
                                                      (juce::uint8)(c.getGreen() * keep),
                                                      (juce::uint8)(c.getBlue() * keep)));
        }
    }

    const float cssToPlate = (float)w / kFaceDesignW;
    const float solidW = juce::jmax(2.f * cssToPlate, kStemCssPx * cssToPlate);
    const float aaW = juce::jmax(0.f, kAaCssPx * cssToPlate);
    auto stampStem = [&](float x01, bool right)
    {
        const float xCenter = juce::jlimit(0.f, 1.f, x01) * (float)w;
        const float left = xCenter - solidW * 0.5f;
        const float rightEdge = left + solidW;
        const int i0 = (int)std::floor(left - aaW);
        const int i1 = (int)std::ceil(rightEdge + aaW) - 1;
        const int rAdd = right ? 110 : 255;
        const int gAdd = 130;
        const int bAdd = right ? 255 : 110;
        for (int ix = i0; ix <= i1; ++ix)
        {
            if (ix < 0 || ix >= w)
                continue;
            const float c = (float)ix + 0.5f;
            float gain = 0.f;
            if (c >= left && c <= rightEdge)
                gain = 1.f;
            else if (aaW > 0.f && c < left)
                gain = juce::jmax(0.f, 1.f - (left - c) / aaW);
            else if (aaW > 0.f)
                gain = juce::jmax(0.f, 1.f - (c - rightEdge) / aaW);
            if (!(gain > 0.f))
                continue;
            const int addR = (int)std::lround((float)rAdd * gain);
            const int addG = (int)std::lround((float)gAdd * gain);
            const int addB = (int)std::lround((float)bAdd * gain);
            for (int y = 0; y < h; ++y)
            {
                const auto c = bits.getPixelColour(ix, y);
                bits.setPixelColour(ix, y,
                                    juce::Colour::fromRGB(
                                        (juce::uint8)juce::jmin(255, c.getRed() + addR),
                                        (juce::uint8)juce::jmin(255, c.getGreen() + addG),
                                        (juce::uint8)juce::jmin(255, c.getBlue() + addB)));
            }
        }
    };

    for (int i = 0; i < 14; ++i)
        stampStem(times[i], (i & 1) != 0);
    repaint();
}

void SabrinaWaterfall::resized()
{
    repaint();
}

void SabrinaWaterfall::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::black);
    if (editor)
    {
        auto &plate = delayPlateFor(*editor);
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.drawImage(plate, getLocalBounds().toFloat());
    }
    g.setColour(juce::Colour(0xff1c2228));
    g.drawRect(getLocalBounds(), 1);
}

void SabrinaTitleBar::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient grad(juce::Colour(0xff020407), 0.f, 0.f, juce::Colour(0xeb080a0d), 0.f,
                              bounds.getHeight(), false);
    g.setGradientFill(grad);
    g.fillRect(bounds);

    const juce::String title{PRODUCT_NAME};
    const float pad = 6.f;
    const float maxH = juce::jmax(8.f, bounds.getHeight() * 0.86f);
    const float maxW = juce::jmax(8.f, bounds.getWidth() - pad * 2.f);
    juce::Font font = sandboxMono(maxH);
    const float textW = juce::jmax(1.f, font.getStringWidthFloat(title));
    if (textW > maxW)
        font = sandboxMono(maxH * maxW / textW);
    g.setColour(kTitle);
    juce::GlyphArrangement ga;
    ga.addLineOfText(font, title, 0.f, 0.f);
    juce::Path outlines;
    ga.createPath(outlines);
    const auto ink = outlines.getBounds();
    if (!ink.isEmpty())
        ga.draw(g, juce::AffineTransform::translation(bounds.getCentreX() - ink.getCentreX(),
                                                      bounds.getCentreY() - ink.getCentreY()));
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
    waterfall.setOpaque(true);
    addAndMakeVisible(titleBar);
    addAndMakeVisible(waterfall);

    struct Row
    {
        uint32_t id;
        const char *unit;
        const char *faceLabel;
    };
    const Row rows[] = {
        {e.patchCopy.sabrina.mix.meta.id, "%", nullptr},
        {e.patchCopy.sabrina.diffusionSize.meta.id, "", nullptr},
        {e.patchCopy.sabrina.diffusionAmount.meta.id, "", nullptr},
        {e.patchCopy.sabrina.delaySize.meta.id, "", nullptr},
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

void SabrinaFace::paint(juce::Graphics &g)
{
    g.fillAll(kModuleFill);
    g.setColour(juce::Colours::white.withAlpha(0.28f));
    g.drawRect(getLocalBounds().toFloat(), 2.f);
}

void SabrinaFace::pushDelayTimes(const float times[14])
{
    waterfall.pushDelayTimes(times);
}

void SabrinaFace::resized()
{
    auto r = getLocalBounds().reduced(editor.px(kInset));
    const int titleH = editor.px(kTitleBarHeight);
    titleBar.setBounds(r.removeFromTop(titleH));
    const int displayH = editor.px(kTitleBarHeight) * juce::jlimit(1, 5, editor.screenSize);
    waterfall.setBounds(r.removeFromTop(juce::jmin(displayH, juce::jmax(0, r.getHeight()))));
    if (sliders.empty())
        return;
    const int gap = editor.px(kBodyGap);
    r.removeFromTop(juce::jmin(gap, r.getHeight()));
    const int n = (int)sliders.size();
    const int totalGap = gap * juce::jmax(0, n - 1);
    const int rowH = juce::jmax(8, (r.getHeight() - totalGap) / n);
    for (int i = 0; i < n; ++i)
    {
        sliders[(size_t)i]->setBounds(r.removeFromTop(rowH));
        if (i + 1 < n)
            r.removeFromTop(gap);
    }
}

} // namespace soundemote::sabrina::ui
