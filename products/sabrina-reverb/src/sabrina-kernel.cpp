/*
 * Sabrina Reverb kernel adapter: se::product::Kernel over the C ABI of the
 * vendored sandbox native module (dsp/sabrina_reverb.cpp).
 */

#include "product/product-api.h"
#include "engine/sabrina-api.h"
#include "product_generated.h"

namespace
{
namespace pi = se::product::param_index;

struct SabrinaKernel : se::product::Kernel
{
    int handle{0};
    float amp{1.f};

    SabrinaKernel() { handle = soemdsp_sabrina_reverb_create(48000.0); }
    ~SabrinaKernel() override
    {
        if (handle > 0)
            soemdsp_sabrina_reverb_destroy(handle);
        handle = 0;
    }

    void prepare(double sampleRate) override
    {
        if (handle > 0)
            soemdsp_sabrina_reverb_reset(handle, sampleRate);
        else
            handle = soemdsp_sabrina_reverb_create(sampleRate);
    }

    void setParams(const double *v, size_t) override
    {
        amp = static_cast<float>(v[pi::amplitude]);
        if (handle <= 0)
            return;
        soemdsp_sabrina_reverb_set_params(handle, v[pi::mix], v[pi::diffusionSize],
                                          v[pi::diffusionAmount], v[pi::delaySize], v[pi::recycle],
                                          v[pi::lfoAmplitude], v[pi::lfoBaseSpeed],
                                          v[pi::lfoVariation], v[pi::seed], v[pi::lfoStyle]);
    }

    void process(const float *const *in, float *const *out, uint32_t frames) override
    {
        for (uint32_t i = 0; i < frames; ++i)
        {
            const double inL = in[0][i];
            const double inR = in[1][i];
            if (handle > 0)
            {
                soemdsp_sabrina_reverb_process(handle, inL, inR);
                out[0][i] = static_cast<float>(soemdsp_sabrina_reverb_left(handle) * amp);
                out[1][i] = static_cast<float>(soemdsp_sabrina_reverb_right(handle) * amp);
            }
            else
            {
                out[0][i] = static_cast<float>(inL * amp);
                out[1][i] = static_cast<float>(inR * amp);
            }
        }
    }

    bool isIdle() const override
    {
        return handle > 0 && soemdsp_sabrina_reverb_is_idle(handle) != 0;
    }

    void advanceIdle(int frames) override
    {
        if (handle > 0)
            soemdsp_sabrina_reverb_advance_mod(handle, frames);
    }

    size_t vizData(float *out, size_t max) override
    {
        if (handle <= 0 || max < 14)
            return 0;
        soemdsp_sabrina_reverb_delay_times(handle, out);
        return 14;
    }
};
} // namespace

std::unique_ptr<se::product::Kernel> se::product::makeKernel()
{
    return std::make_unique<SabrinaKernel>();
}
