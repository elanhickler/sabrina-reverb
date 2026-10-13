#pragma once

#include <soemdsp/plugin/Parameter.hpp>
#include <soemdsp/plugin/ParameterPrototype.hpp>

namespace soundemote::sabrina
{
namespace proto = soemdsp::plugin::parameterprototype;

// soemdsp plugin Parameter stack for Sabrina.
// Linear (set / getLinear) is 0..1 for host modulation.
// Mapped (setMapped / getDspDouble) is the real DSP number the kernel and
// CLAP host report (Mix 0.43, Size 0.35, Rec 0.70 — not percents).
struct SabrinaSoemParams
{
    SabrinaSoemParams()
        : mix(proto::Decimal{0.43, 0.0, 1.0, 0.0}),
          diffusionSize(proto::Decimal{0.35, 0.0, 1.0, 0.0}),
          diffusionAmount(proto::Decimal{0.70, 0.0, 0.98, 0.0}),
          delaySize(proto::Decimal{0.02, 0.0, 1.0, 0.0}),
          recycle(proto::Decimal{0.70, 0.0, 2.0, 0.0}),
          lfoAmplitude(proto::Decimal{0.07, 0.0, 1.0, 0.0}),
          lfoBaseSpeed(proto::Decimal{0.83, 0.0, 1.0, 0.0}),
          lfoVariation(proto::Decimal{0.001, 0.0, 1.0, 0.0}),
          lfoStyle(proto::Choice{0, {{0, "Parabol"}, {1, "FBM"}}}),
          seed(proto::Integer{0, 0, 16777215}),
          amplitude(proto::Decimal{1.0, 0.0, 1.0, 0.0})
    {
        mix.setMapped(0.43);
        diffusionSize.setMapped(0.35);
        diffusionAmount.setMapped(0.70);
        delaySize.setMapped(0.02);
        recycle.setMapped(0.70);
        lfoAmplitude.setMapped(0.07);
        lfoBaseSpeed.setMapped(0.83);
        lfoVariation.setMapped(0.001);
        lfoStyle.setMapped(0.0);
        seed.setMapped(0.0);
        amplitude.setMapped(1.0);
    }

    soemdsp::plugin::Parameter mix;
    soemdsp::plugin::Parameter diffusionSize;
    soemdsp::plugin::Parameter diffusionAmount;
    soemdsp::plugin::Parameter delaySize;
    soemdsp::plugin::Parameter recycle;
    soemdsp::plugin::Parameter lfoAmplitude;
    soemdsp::plugin::Parameter lfoBaseSpeed;
    soemdsp::plugin::Parameter lfoVariation;
    soemdsp::plugin::Parameter lfoStyle;
    soemdsp::plugin::Parameter seed;
    soemdsp::plugin::Parameter amplitude;

    soemdsp::plugin::Parameter *byClapId(uint32_t id)
    {
        switch (id)
        {
        case 500:
            return &mix;
        case 501:
            return &diffusionSize;
        case 502:
            return &diffusionAmount;
        case 503:
            return &delaySize;
        case 504:
            return &recycle;
        case 505:
            return &lfoAmplitude;
        case 506:
            return &lfoBaseSpeed;
        case 507:
            return &lfoVariation;
        case 508:
            return &lfoStyle;
        case 509:
            return &seed;
        case 510:
            return &amplitude;
        default:
            return nullptr;
        }
    }
};
} // namespace soundemote::sabrina
