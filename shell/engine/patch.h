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
#include <memory>
#include <string>
#include "product_generated.h"

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
    static constexpr uint32_t patchVersion{se::product::patchVersion};
    static constexpr const char *id{se::product::clapId};

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
        productParams.reserve(se::product::numParams);
        for (const auto &d : se::product::params)
            productParams.push_back(std::make_unique<Param>(mdFromDesc(d)));
        for (auto &p : productParams)
            this->pushSingleParam(p.get());

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

    // Host-facing params come from the product's generated descriptor table
    // (products/<slug>/product.json). Ids are frozen by params.lock.
    static md_t mdFromDesc(const se::product::ParamDesc &d)
    {
        using T = se::product::ParamType;
        md_t m;
        switch (d.type)
        {
        case T::Percent:
            m = floatMd().asPercent().withDefault(d.defFloat);
            break;
        case T::Unit:
            m = unitRange(d.defFloat, (float)d.min, (float)d.max);
            break;
        case T::Integer:
            m = intMd().withRange((float)d.min, (float)d.max).withDefault(d.defFloat);
            break;
        case T::Choice:
        {
            std::unordered_map<int, std::string> labels;
            for (int i = 0; i < d.numChoices; ++i)
                labels[d.choices[i].value] = d.choices[i].label;
            m = intMd()
                    .withRange((float)d.min, (float)d.max)
                    .withDefault(d.defFloat)
                    .withUnorderedMapFormatting(labels);
            break;
        }
        }
        return m.withName(d.name).withGroupName(d.group).withID(d.id);
    }

    // One Param per descriptor row, in table order (index = param_index::<key>).
    std::vector<std::unique_ptr<Param>> productParams;

    Param &param(size_t index) { return *productParams[index]; }
    const Param &param(size_t index) const { return *productParams[index]; }

    char name[256]{"Init"};

    float migrateParamValueFromVersion(Param *p, float value, uint32_t version);
    void migratePatchFromVersion(uint32_t version);
};
} // namespace soundemote::sabrina
#endif // PATCH_H
