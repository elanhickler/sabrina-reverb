#include "main-panel.h"

namespace baconpaul::sidequest_ns::ui
{

MainPanel::MainPanel(PluginEditor &e) : editor(e)
{
    face = std::make_unique<SabrinaFace>(e);
    addAndMakeVisible(*face);
}

void MainPanel::resized()
{
    if (face)
        face->setBounds(getLocalBounds());
}

void MainPanel::pushPeaks(float left, float right)
{
    if (face)
        face->pushPeaks(left, right);
}

} // namespace baconpaul::sidequest_ns::ui
