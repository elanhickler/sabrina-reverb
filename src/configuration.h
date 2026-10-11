/*
 * Sabrina Reverb — Soundemote
 * DSP MIT (Walter Hackett). UI links JUCE (AGPLv3). See LICENSE.md.
 */

#ifndef SOUNDEMOTE_SABRINA_CONFIGURATION_H
#define SOUNDEMOTE_SABRINA_CONFIGURATION_H

#include <stddef.h>
#include <cstdint>
#include <string>
#include <cstring>
#include <iostream>
#include "sst/plugininfra/version_information.h"
#include <fmt/core.h>

namespace soundemote::sabrina
{

static constexpr size_t blockSize{8};

extern int debugLevel;

} // namespace soundemote::sabrina

inline std::string fileTrunc(const std::string &f)
{
    auto p = f.find(sst::plugininfra::VersionInformation::cmake_source_dir);
    if (p != std::string::npos)
    {
        return f.substr(p + strlen(sst::plugininfra::VersionInformation::cmake_source_dir) + 1);
    }
    return f;
}

#define SQLOG(...)                                                                                 \
    std::cout << fileTrunc(__FILE__) << ":" << __LINE__ << " " << __VA_ARGS__ << std::endl;

#define SQLOGFMT(...)                                                                              \
    std::cout << fileTrunc(__FILE__) << ":" << __LINE__ << " " << fmt::format(__VA_ARGS__)         \
              << std::endl;

#define SQLOG_ERR(...)                                                                             \
    std::cout << fileTrunc(__FILE__) << ":" << __LINE__ << " [ERROR] " << __VA_ARGS__ << std::endl;

#define SQLOG_UNIMPL                                                                               \
    std::cout << fileTrunc(__FILE__) << ":" << __LINE__ << " Unimplemented " << __func__           \
              << std::endl;

#define SQLOG_ONCE(...)                                                                            \
    {                                                                                              \
        static bool x842132{false};                                                                \
        if (!x842132)                                                                              \
        {                                                                                          \
            SQLOG(__VA_ARGS__);                                                                    \
        }                                                                                          \
        x842132 = true;                                                                            \
    }
#define SQD(x) " " << #x << "=" << x

#endif // CONFIGURATION_H
