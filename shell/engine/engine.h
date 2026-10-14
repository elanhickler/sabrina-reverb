/*
 * Sabrina Reverb — Soundemote
 * DSP MIT (Walter Hackett). UI links JUCE (AGPLv3). See LICENSE.md.
 */

#ifndef SOUNDEMOTE_SABRINA_ENGINE_ENGINE_H
#define SOUNDEMOTE_SABRINA_ENGINE_ENGINE_H

#include <memory>
#include <array>
#include <atomic>
#include <cstring>

#include <clap/clap.h>
#include "sst/basic-blocks/dsp/Lag.h"
#include "sst/cpputils/ring_buffer.h"

#include "filesystem/import.h"

#include "configuration.h"
#include "engine/patch.h"
#include "sst/basic-blocks/dsp/LagCollection.h"
#include "product/product-api.h"
#include "engine/soem-params.h"

namespace soundemote::sabrina
{
struct Engine
{
    float output alignas(16)[2][blockSize];

    Patch patch;

    std::unique_ptr<se::product::Kernel> kernel;
    SoemParams soem;
    void syncParamsToKernel();

    Engine();
    ~Engine();

    bool audioRunning{true};
    int beginEndParamGestureCount{0};

    double sampleRate{1}, sampleRateInv{1};
    void setSampleRate(double sampleRate);

    void process(const clap_output_events_t *);
    void process(const clap_output_events_t *, const float *const *inputChannels,
                 uint32_t inputChannelCount, uint32_t inputFrameCount);
    void processUIQueue(const clap_output_events_t *);

    // SilenceDetector: ~1s of input+tail below Planck. Delay network stops.
    // LFOs keep running only while the editor is open.
    bool isIdle() const;

    void handleParamValue(Param *p, uint32_t pid, float value);

    // UI Communication
    struct AudioToUIMsg
    {
        enum Action : uint32_t
        {
            UPDATE_PARAM,
            SET_PATCH_NAME,
            SET_PATCH_DIRTY_STATE,
            DO_PARAM_RESCAN,
            SEND_SAMPLE_RATE,
            VIZ_DATA
        } action;
        uint32_t paramId{0};
        float value{0}, value2{0};
        const char *patchNamePointer{0};
        float vizData[se::product::kMaxVizValues]{};
        uint32_t vizCount{0};
    };
    struct MainToAudioMsg
    {
        enum Action : uint32_t
        {
            REQUEST_REFRESH,
            SET_PARAM,
            SET_PARAM_WITHOUT_NOTIFYING,
            BEGIN_EDIT,
            END_EDIT,
            STOP_AUDIO,
            START_AUDIO,
            SEND_PATCH_NAME,
            SEND_PATCH_IS_CLEAN,
            SEND_POST_LOAD,
            SEND_REQUEST_RESCAN,
            EDITOR_ATTACH_DETATCH, // paramid is true for attach and false for detach
            SEND_PREP_FOR_STREAM
        } action;
        uint32_t paramId{0};
        float value{0};
        const char *uiManagedPointer{nullptr};
    };
    using audioToUIQueue_t = sst::cpputils::SimpleRingBuffer<AudioToUIMsg, 1024 * 16>;
    using mainToAudioQueue_T = sst::cpputils::SimpleRingBuffer<MainToAudioMsg, 1024 * 64>;
    audioToUIQueue_t audioToUi;
    mainToAudioQueue_T mainToAudio;
    std::atomic<bool> doFullRefresh{false};
    std::atomic<bool> isEditorAttached{false};
    sst::basic_blocks::dsp::UIComponentLagHandler lagHandler;

    std::atomic<bool> readyForStream{false};
    void prepForStream()
    {
        SQLOG("Ready for Stream");
        if (lagHandler.active)
            lagHandler.instantlySnap();

        for (auto &p : paramLagSet)
        {
            p.lag.snapToTarget();
            p.value = p.lag.v;
        }
        paramLagSet.removeAll();

        patch.dirty = false;
        doFullRefresh = true;
        readyForStream = true;
    }

    void pushFullUIRefresh();
    void postLoad()
    {
        doFullRefresh = true;

        for (auto &[i, p] : patch.paramMap)
        {
            p->lag.snapTo(p->value);
        }
    }

    std::atomic<bool> onMainRescanParams{false};
    std::atomic<bool> onMainRescanInfo{false};
    void onMainThread();

    sst::cpputils::active_set_overlay<Param> paramLagSet;

    int32_t lastScopeUpdate{0};

    const clap_host_t *clapHost{nullptr};
};
} // namespace soundemote::sabrina
#endif // SYNTH_H
