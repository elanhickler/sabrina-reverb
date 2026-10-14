/*
 * The product interface. A product (products/<slug>/) provides exactly:
 *   1. product.json  -> generated descriptor table (product_generated.h)
 *   2. makeKernel()  -> its DSP behind se::product::Kernel
 *   3. makeFace()    -> its face (product-face.h); every product uses the
 *                       Sabrina look
 * The shell (clap entry, engine, presets, editor) only talks to these and
 * never branches on which product it is.
 */

#ifndef SOUNDEMOTE_SHELL_PRODUCT_API_H
#define SOUNDEMOTE_SHELL_PRODUCT_API_H

#include <cstddef>
#include <cstdint>
#include <memory>

#include "product/param-desc.h"

namespace se::product
{
// Max values a kernel can hand to its face per update (Sabrina: 14 delay times).
inline constexpr size_t kMaxVizValues = 16;

struct Kernel
{
    virtual ~Kernel() = default;

    // Sample rate changed (also called with the host rate before audio starts).
    virtual void prepare(double sampleRate) = 0;

    // Mapped DSP values for every param, in descriptor order. Called once per
    // shell block (blockSize frames), before process().
    virtual void setParams(const double *dspValues, size_t count) = 0;

    // One shell block. in[0..1] hold `frames` samples (missing input is 0,
    // a mono input is copied to both). Write out[0..1].
    virtual void process(const float *const *in, float *const *out, uint32_t frames) = 0;

    // Silence detection: when true and the input is silent the shell skips
    // process() and writes zeros.
    virtual bool isIdle() const { return false; }
    // Called instead of process() while idle and the editor is open (keeps
    // modulation moving for the face).
    virtual void advanceIdle(int /*frames*/) {}

    // Optional values for the face (about every 100 blocks while the editor
    // is open). Return the count written (<= max).
    virtual size_t vizData(float * /*out*/, size_t /*max*/) { return 0; }
};

std::unique_ptr<Kernel> makeKernel();
} // namespace se::product

#endif
