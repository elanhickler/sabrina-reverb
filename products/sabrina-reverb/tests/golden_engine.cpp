/*
 * Golden render (engine side, kept apart from catch2 because its macros
 * clash with soemdsp/sehelper.hpp): fixed params + impulse/noise input through the real Engine.
 * The FNV-1a hash of every output float must stay bit-identical across the
 * shell restructure. Record a new golden only on purpose:
 *   set SE_GOLDEN_RECORD=1 and run the tests once.
 */

#include "engine/engine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#ifndef SE_GOLDEN_DIR
#error SE_GOLDEN_DIR must be defined
#endif

namespace
{
struct Fnv
{
    uint64_t h{1469598103934665603ULL};
    void add(const void *p, size_t n)
    {
        auto b = static_cast<const unsigned char *>(p);
        for (size_t i = 0; i < n; ++i)
        {
            h ^= b[i];
            h *= 1099511628211ULL;
        }
    }
};

template <typename E> uint64_t renderGolden(E &engine, float &peak)
{
    constexpr double sr = 48000.0;
    constexpr int seconds = 4;
    constexpr int bs = (int)soundemote::sabrina::blockSize;
    const int total = (int)sr * seconds;

    engine.setSampleRate(sr);

    // Fixed, non-default settings (ids 500..510).
    auto set = [&](uint32_t id, float v) {
        auto p = engine.patch.paramMap.at(id);
        p->value = v;
        p->lag.snapTo(v);
    };
    set(500, 0.6f);   // mix
    set(501, 0.5f);   // diffusion size
    set(502, 0.65f);  // diffusion amount
    set(503, 0.1f);   // delay time
    set(504, 0.8f);   // recycle
    set(505, 0.2f);   // mod amp
    set(506, 0.5f);   // mod speed
    set(507, 0.05f);  // mod vary
    set(508, 1.0f);   // style FBM
    set(509, 1234.f); // seed
    set(510, 0.9f);   // amplitude

    uint32_t rng = 0x12345678u;
    Fnv fnv;
    float inL[bs], inR[bs];
    const float *ins[2] = {inL, inR};
    for (int f = 0; f < total; f += bs)
    {
        for (int i = 0; i < bs; ++i)
        {
            const int n = f + i;
            rng = rng * 1664525u + 1013904223u;
            const float noise = ((rng >> 8) * (1.0f / 16777216.0f) - 0.5f) * 0.5f;
            const bool burst = n < (int)(sr * 0.25);
            inL[i] = (n == 0 ? 1.f : 0.f) + (burst ? noise : 0.f);
            inR[i] = burst ? -noise * 0.7f : 0.f;
        }
        if (f == (int)sr * 2) // a host automation step half-way through
            engine.handleParamValue(nullptr, 506, 0.9f);
        engine.process(nullptr, ins, 2, bs);
        for (int i = 0; i < bs; ++i)
            peak = std::max(peak, std::max(std::abs(engine.output[0][i]), std::abs(engine.output[1][i])));
        fnv.add(engine.output[0], sizeof(float) * bs);
        fnv.add(engine.output[1], sizeof(float) * bs);
        while (engine.audioToUi.pop().has_value())
        {
        }
    }
    return fnv.h;
}
} // namespace

// Shipped identity of Sabrina Reverb. Must never change (DAW sessions and
// presets store these).
bool seIdentityOk(std::string &why)
{
    using soundemote::sabrina::Patch;
    if (std::string(Patch::id) != "org.soundemote.sabrina-reverb")
        return why = "clap/patch id changed", false;
    if (Patch::patchVersion != 2)
        return why = "patchVersion changed", false;
    if (std::string(se::product::patchExtension) != ".sabrina")
        return why = "preset extension changed", false;
    Patch p;
    if (p.params.size() != 11)
        return why = "param count changed", false;
    for (uint32_t id = 500; id <= 510; ++id)
        if (p.paramMap.find(id) == p.paramMap.end())
            return why = "missing param id " + std::to_string(id), false;
    return true;
}

uint64_t seGoldenRenderHash(float &peak)
{
    peak = 0.f;
    auto engine = std::make_unique<soundemote::sabrina::Engine>();
    return renderGolden(*engine, peak);
}

