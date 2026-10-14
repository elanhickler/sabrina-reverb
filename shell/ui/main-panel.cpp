#include "main-panel.h"

namespace soundemote::sabrina::ui
{

MainPanel::MainPanel(PluginEditor &e) : editor(e)
{
    face = se::product::makeFace(e);
    addAndMakeVisible(*face);
}

void MainPanel::resized()
{
    if (face)
        face->setBounds(getLocalBounds());
}

void MainPanel::pushVizData(const float *values, size_t count)
{
    if (face)
        face->pushVizData(values, count);
}

} // namespace soundemote::sabrina::ui
