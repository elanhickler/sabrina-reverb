#pragma once

#include <soemdsp/plugin/Parameter.hpp>
#include <soemdsp/plugin/ParameterPrototype.hpp>

#include <string>
#include <unordered_map>
#include <vector>

#include "product_generated.h"

namespace soundemote::sabrina
{
namespace proto = soemdsp::plugin::parameterprototype;

// soemdsp plugin Parameter stack, one per descriptor row (same order as
// se::product::params). Linear (set / getLinear) is 0..1 for host
// modulation. Mapped (setMapped / getDspDouble) is the real DSP number the
// kernel and CLAP host report (Mix 0.43, not a percent).
struct SoemParams
{
    SoemParams()
    {
        using T = se::product::ParamType;
        values.reserve(se::product::numParams);
        for (const auto &d : se::product::params)
        {
            switch (d.type)
            {
            case T::Percent:
            case T::Unit:
                values.emplace_back(proto::Decimal{d.def, d.min, d.max, 0.0});
                break;
            case T::Integer:
                values.emplace_back(proto::Integer{(int)d.def, (int)d.min, (int)d.max});
                break;
            case T::Choice:
            {
                std::unordered_map<int, std::string> labels;
                for (int i = 0; i < d.numChoices; ++i)
                    labels[d.choices[i].value] = d.choices[i].label;
                values.emplace_back(proto::Choice{(int)d.def, labels});
                break;
            }
            }
            values.back().setMapped(d.def);
        }
    }

    soemdsp::plugin::Parameter &operator[](size_t index) { return values[index]; }
    const soemdsp::plugin::Parameter &operator[](size_t index) const { return values[index]; }

    std::vector<soemdsp::plugin::Parameter> values;
};
} // namespace soundemote::sabrina
