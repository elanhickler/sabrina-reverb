/*
 * Face half of the product interface (see product-api.h).
 */

#ifndef SOUNDEMOTE_SHELL_PRODUCT_FACE_H
#define SOUNDEMOTE_SHELL_PRODUCT_FACE_H

#include <cstddef>
#include <memory>
#include <juce_gui_basics/juce_gui_basics.h>

namespace soundemote::sabrina::ui
{
struct PluginEditor;
}

namespace se::product
{
struct Face : juce::Component
{
    // Values from Kernel::vizData.
    virtual void pushVizData(const float * /*values*/, size_t /*count*/) {}
    // The patch name / module title changed.
    virtual void patchNameChanged() {}
};

std::unique_ptr<Face> makeFace(soundemote::sabrina::ui::PluginEditor &editor);
} // namespace se::product

#endif
