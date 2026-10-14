#include "main-panel.h"

namespace soundemote::sabrina::ui
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

void MainPanel::pushDelayTimes(const float times[14])
{
    if (face)
        face->pushDelayTimes(times);
}

} // namespace soundemote::sabrina::ui
