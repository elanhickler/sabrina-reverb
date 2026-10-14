/*
 * Sabrina Reverb — Soundemote
 * DSP MIT (Walter Hackett). UI links JUCE (AGPLv3). See LICENSE.md.
 */

#ifndef SOUNDEMOTE_SABRINA_PRESETS_PRESET_MANAGER_H
#define SOUNDEMOTE_SABRINA_PRESETS_PRESET_MANAGER_H

#include <clap/clap.h>
#include "filesystem/import.h"
#include "sst/jucegui/data/Discrete.h"
#include "engine/patch.h"
#include "engine/engine.h"
#include <map>
#include <unordered_map>
#include <functional>
#include <set>
#include <string>

namespace soundemote::sabrina::presets
{
struct PresetManager
{
    fs::path userPath;
    fs::path userPatchesPath;
    const clap_host_t *clapHost{nullptr};

    // Call with a null host to be read-only
    PresetManager(const clap_host_t *host);
    ~PresetManager();

    void rescanUserPresets();

    void loadInit(Patch &p, Engine::mainToAudioQueue_T &);
    void loadUserPresetDirect(Patch &, Engine::mainToAudioQueue_T &, const fs::path &p);
    void loadFactoryPreset(Patch &, Engine::mainToAudioQueue_T &, const std::string &cat,
                           const std::string &pat);

#if USE_WCHAR_PRESET
    void saveUserPresetDirect(Patch &, const wchar_t *utf8path);
#else
    void saveUserPresetDirect(Patch &, const fs::path &p);
#endif

    std::function<void(const std::string &)> onPresetLoaded{nullptr};

    static constexpr const char *factoryPath{"resources/factory_patches"};
    std::map<std::string, std::vector<std::string>> factoryPatchNames;
    std::vector<std::pair<std::string, std::string>> factoryPatchVector;
    std::vector<fs::path> userPatches;

    const clap_host_params_t *clapHostParams{nullptr};
    void sendEntirePatchToAudio(Patch &, Engine::mainToAudioQueue_T &, const std::string &name);
    static void sendEntirePatchToAudio(Patch &, Engine::mainToAudioQueue_T &,
                                       const std::string &name, const clap_host_t *,
                                       const clap_host_params_t *p = nullptr);
};
} // namespace soundemote::sabrina::presets
#endif // PRESET_MANAGER_H
