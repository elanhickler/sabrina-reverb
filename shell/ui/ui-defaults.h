/*
 * Sabrina Reverb — Soundemote
 * DSP MIT (Walter Hackett). UI links JUCE (AGPLv3). See LICENSE.md.
 */

#ifndef SOUNDEMOTE_SABRINA_UI_UI_DEFAULTS_H
#define SOUNDEMOTE_SABRINA_UI_UI_DEFAULTS_H

#include "configuration.h"
#include <sst/plugininfra/userdefaults.h>

namespace soundemote::sabrina::ui
{
enum Defaults
{
    useLightSkin,
    zoomLevel,
    screenSize,          // display height in title-bar units, 1..5, default 2
    useSoftwareRenderer, // only used on windows
    numDefaults
};

inline std::string defaultName(Defaults d)
{
    switch (d)
    {
    case useLightSkin:
        return "useLightSkin";
    case zoomLevel:
        return "zoomLevel";
    case screenSize:
        return "screenSize";
    case useSoftwareRenderer:
        return "useSoftwareRenderer";
    case numDefaults:
    {
        SQLOG("Software Error - defaults found");
        return "";
    }
    }
    return "";
}

using defaultsProvder_t = sst::plugininfra::defaults::Provider<Defaults, Defaults::numDefaults>;
} // namespace soundemote::sabrina::ui

#endif // UI_DEFAULTS_H
