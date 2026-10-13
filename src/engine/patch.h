/*
 * Sabrina Reverb — based on SideQuest Starting Point
 *
 * Parameter surface matches soemdsp-sandbox reverbEffect
 * (public/node-graph-module-definitions.js).
 *
 * Copyright 2024-2025, Paul Walker and Various authors, as described in the github
 * transaction log. Soundemote adaptations for Sabrina Reverb.
 *
 * This source repo is released under the MIT license, but has
 * GPL3 dependencies, as such the combined work will be
 * released under GPL3.
 *
 * The source code and license are at https://github.com/baconpaul/sidequest-startingpoint
 */

#ifndef SOUNDEMOTE_SABRINA_ENGINE_PATCH_H
#define SOUNDEMOTE_SABRINA_ENGINE_PATCH_H

#include <vector>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <clap/clap.h>
#include "configuration.h"
#include "sst/cpputils/constructors.h"
#include "sst/cpputils/active_set_overlay.h"
#include "sst/basic-blocks/params/ParamMetadata.h"
#include "sst/basic-blocks/dsp/Lag.h"
#include "sst/plugininfra/patch-support/patch_base.h"

namespace soundemote::sabrina
{
namespace scpu = sst::cpputils;
namespace pats = sst::plugininfra::patch_support;
using md_t = sst::basic_blocks::params::ParamMetaData;
struct Param : pats::ParamBase, sst::cpputils::active_set_overlay<Param>::participant
{
    Param(const md_t &m) : pats::ParamBase(m) {}

    Param &operator=(const float &val)
    {
        value = val;
        return *this;
    }

    uint64_t adhocFeatures{0};
    enum AdHocFeatureValues : uint64_t
    {
    };

    bool isTemposynced() const
    {
        if (tempoSyncPartner)
            return tempoSyncPartner->value;

        return false;
    }

    Param *tempoSyncPartner{nullptr};

    sst::basic_blocks::dsp::LinearLag<float, false> lag;
};

struct Patch : pats::PatchBase<Patch, Param>
{
    static constexpr uint32_t patchVersion{2};
    static constexpr const char *id{"org.soundemote.sabrina-reverb"};

    static constexpr uint32_t floatFlags{CLAP_PARAM_IS_AUTOMATABLE};
    static constexpr uint32_t boolFlags{CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED};

    static md_t floatMd() { return md_t().asFloat().withFlags(floatFlags); }
    static md_t boolMd() { return md_t().asBool().withFlags(boolFlags); }
    static md_t intMd() { return md_t().asInt().withFlags(boolFlags); }

    // Host reports the real DSP number (0.43, not 43%). Modulation linear
    // 0..1 lives on soemdsp::plugin::Parameter::getLinear().
    static md_t unit01(float def)
    {
        return floatMd()
            .withRange(0.f, 1.f)
            .withDefault(def)
            .withDimensionlessFormatting()
            .withDecimalPlaces(3);
    }
    static md_t unitRange(float def, float lo, float hi)
    {
        return floatMd()
            .withRange(lo, hi)
            .withDefault(def)
            .withDimensionlessFormatting()
            .withDecimalPlaces(3);
    }

    Patch() : pats::PatchBase<Patch, Param>()

    {
        auto pushParams = [this](auto &from) { this->pushMultipleParams(from.params()); };

        pushParams(sabrina);

        std::sort(params.begin(), params.end(),
                  [](const Param *a, const Param *b)
                  {
                      auto ga = a->meta.groupName;
                      auto gb = b->meta.groupName;
                      if (ga != gb)
                      {
                          if (ga == "Main")
                              return true;
                          if (gb == "Main")
                              return false;

                          return ga < gb;
                      }

                      return a->meta.id < b->meta.id;
                  });
    }

    // Host-facing params = soemdsp-sandbox reverbEffect keys.
    struct SabrinaNode
    {
        static constexpr uint32_t idBase{500};

        SabrinaNode()
            : mix(floatMd()
                      .asPercent()
                      .withDefault(0.43f)
                      .withName("Mix")
                      .withGroupName("Main")
                      .withID(id(0))),
              diffusionSize(unit01(0.35f)
                                .withName("Diffusion Size")
                                .withGroupName("Diffusion")
                                .withID(id(1))),
              diffusionAmount(unit01(0.70f)
                                  .withName("Diffusion Amount")
                                  .withGroupName("Diffusion")
                                  .withID(id(2))),
              delaySize(unit01(0.02f)
                            .withName("Delay Time")
                            .withGroupName("Echo")
                            .withID(id(3))),
              recycle(unit01(0.70f)
                          .withName("Recycle")
                          .withGroupName("Echo")
                          .withID(id(4))),
              lfoAmplitude(unit01(0.07f)
                               .withName("Mod(~)Amp")
                               .withGroupName("Mod")
                               .withID(id(5))),
              lfoBaseSpeed(unit01(0.83f)
                               .withName("Mod(~)Speed")
                               .withGroupName("Mod")
                               .withID(id(6))),
              lfoVariation(unit01(0.001f)
                               .withName("Mod(~)Vary")
                               .withGroupName("Mod")
                               .withID(id(7))),
              lfoStyle(intMd()
                           .withRange(0, 1)
                           .withDefault(0)
                           .withUnorderedMapFormatting({{0, "Parabol"}, {1, "FBM"}})
                           .withName("Mod(~)Style")
                           .withGroupName("Mod")
                           .withID(id(8))),
              seed(intMd()
                       .withRange(0, 16777215)
                       .withDefault(0)
                       .withName("Seed")
                       .withGroupName("Mod")
                       .withID(id(9))),
              amplitude(unit01(1.f)
                            .withName("Amplitude")
                            .withGroupName("Main")
                            .withID(id(10)))
        {
        }

        std::string name() const { return "Sabrina"; }
        uint32_t id(int f) const { return idBase + f; }

        Param mix, diffusionSize, diffusionAmount, delaySize, recycle, lfoAmplitude, lfoBaseSpeed,
            lfoVariation, lfoStyle, seed, amplitude;

        std::vector<Param *> params()
        {
            return {&mix,          &diffusionSize, &diffusionAmount, &delaySize, &recycle,
                    &lfoAmplitude, &lfoBaseSpeed,  &lfoVariation,    &lfoStyle,  &seed,
                    &amplitude};
        }
    };

    SabrinaNode sabrina;

    char name[256]{"Init"};

    float migrateParamValueFromVersion(Param *p, float value, uint32_t version);
    void migratePatchFromVersion(uint32_t version);
};
} // namespace soundemote::sabrina
#endif // PATCH_H
