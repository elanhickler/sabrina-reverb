/*
 * Sabrina Reverb — Soundemote
 * DSP MIT (Walter Hackett). UI links JUCE (AGPLv3). See LICENSE.md.
 */

#include "engine/engine.h"
#include <algorithm>
#include <cmath>
namespace soundemote::sabrina
{

int debugLevel{0};

Engine::Engine()
{
    kernel = se::product::makeKernel();
    syncParamsToKernel();
}

Engine::~Engine() = default;

void Engine::syncParamsToKernel()
{
    if (!kernel)
        return;

    // Host/CLAP knobs carry real DSP numbers. soemdsp Parameter stores
    // the same mapped value and a linear copy for 0..1 modulation.
    double dsp[se::product::numParams];
    for (size_t i = 0; i < se::product::numParams; ++i)
    {
        soem[i].setMapped(patch.param(i).value);
        dsp[i] = soem[i].getDspDouble();
    }
    kernel->setParams(dsp, se::product::numParams);
}

void Engine::setSampleRate(double sr)
{
    sampleRate = sr;
    sampleRateInv = 1.0 / sr;
    for (auto &[i, p] : patch.paramMap)
    {
        p->lag.setRateInMilliseconds(1000.0 * 64.0 / 48000.0, sampleRate, 1.0 / blockSize);
        p->lag.snapTo(p->value);
    }
    paramLagSet.removeAll();

    if (kernel)
        kernel->prepare(sampleRate);
    syncParamsToKernel();

    audioToUi.push({AudioToUIMsg::SEND_SAMPLE_RATE, 0, (float)sampleRate});
}

void Engine::process(const clap_output_events_t *outq)
{
    process(outq, nullptr, 0, 0);
}

bool Engine::isIdle() const
{
    return kernel && kernel->isIdle();
}

void Engine::process(const clap_output_events_t *outq, const float *const *inputChannels,
                     uint32_t inputChannelCount, uint32_t inputFrameCount)
{
    processUIQueue(outq);

    if (!audioRunning)
    {
        memset(output, 0, sizeof(output));
        return;
    }

    for (auto it = paramLagSet.begin(); it != paramLagSet.end();)
    {
        it->lag.process();
        it->value = it->lag.v;
        if (!it->lag.isActive())
        {
            it = paramLagSet.erase(it);
        }
        else
        {
            ++it;
        }
    }

    lagHandler.process();

    syncParamsToKernel();

    const bool hasInput = inputChannels != nullptr && inputChannelCount > 0 &&
                          inputChannels[0] != nullptr && inputFrameCount > 0;

    // Planck 1e-7, same as soemdsp SilenceDetector.
    constexpr double kPlanck = 1.0e-7;
    bool inputAwake = false;
    if (hasInput)
    {
        const uint32_t frames = std::min<uint32_t>(blockSize, inputFrameCount);
        for (uint32_t i = 0; i < frames; ++i)
        {
            const double inL = inputChannels[0][i];
            const double inR = inputChannelCount > 1 && inputChannels[1] != nullptr
                                   ? inputChannels[1][i]
                                   : inL;
            if (std::abs(inL) >= kPlanck || std::abs(inR) >= kPlanck)
            {
                inputAwake = true;
                break;
            }
        }
    }

    if (isIdle() && !inputAwake)
    {
        memset(output, 0, sizeof(output));
        if (!isEditorAttached.load(std::memory_order_relaxed) || !kernel)
            return;
        kernel->advanceIdle((int)blockSize);
    }
    else
    {
        float in[2][blockSize];
        for (int i = 0; i < blockSize; ++i)
        {
            const bool validFrame = hasInput && static_cast<uint32_t>(i) < inputFrameCount;
            in[0][i] = validFrame ? inputChannels[0][i] : 0.f;
            in[1][i] = validFrame && inputChannelCount > 1 && inputChannels[1] != nullptr
                           ? inputChannels[1][i]
                           : in[0][i];
        }
        const float *ins[2] = {in[0], in[1]};
        float *outs[2] = {output[0], output[1]};
        if (kernel)
        {
            kernel->process(ins, outs, blockSize);
        }
        else
        {
            memcpy(output[0], in[0], sizeof(in[0]));
            memcpy(output[1], in[1], sizeof(in[1]));
        }
    }

    if (isEditorAttached.load(std::memory_order_relaxed))
    {
        if (++lastScopeUpdate >= 100)
        {
            AudioToUIMsg msg{AudioToUIMsg::VIZ_DATA};
            if (kernel)
                msg.vizCount =
                    (uint32_t)kernel->vizData(msg.vizData, se::product::kMaxVizValues);
            audioToUi.push(msg);
            lastScopeUpdate = 0;
        }
    }
}

void Engine::processUIQueue(const clap_output_events_t *outq)
{
    bool didRefresh{false};
    if (doFullRefresh)
    {
        pushFullUIRefresh();
        doFullRefresh = false;
        didRefresh = true;
    }
    auto uiM = mainToAudio.pop();
    while (uiM.has_value())
    {
        switch (uiM->action)
        {
        case MainToAudioMsg::REQUEST_REFRESH:
        {
            if (!didRefresh)
            {
                // don't do it twice in one process obvs
                pushFullUIRefresh();
            }
        }
        break;
        case MainToAudioMsg::SET_PARAM_WITHOUT_NOTIFYING:
        case MainToAudioMsg::SET_PARAM:
        {
            bool notify = uiM->action == MainToAudioMsg::SET_PARAM;

            auto dest = patch.paramMap.at(uiM->paramId);
            if (notify)
            {
                if (beginEndParamGestureCount == 0)
                {
                    SQLOG("Non-begin/end bound param edit for '" << dest->meta.name << "'");
                }
                if (dest->meta.type == md_t::FLOAT)
                    lagHandler.setNewDestination(&(dest->value), uiM->value);
                else
                    dest->value = uiM->value;

                clap_event_param_value_t p;
                p.header.size = sizeof(clap_event_param_value_t);
                p.header.time = 0;
                p.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                p.header.type = CLAP_EVENT_PARAM_VALUE;
                p.header.flags = 0;
                p.param_id = uiM->paramId;
                p.cookie = dest;

                p.note_id = -1;
                p.port_index = -1;
                p.channel = -1;
                p.key = -1;

                p.value = uiM->value;

                outq->try_push(outq, &p.header);
            }
            else
            {
                dest->value = uiM->value;
            }

            // Side Effects and Ad Hoc Features go here

            auto d = patch.dirty;
            if (!d)
            {
                patch.dirty = true;
                audioToUi.push({AudioToUIMsg::SET_PATCH_DIRTY_STATE, patch.dirty});
            }
        }
        break;
        case MainToAudioMsg::BEGIN_EDIT:
        case MainToAudioMsg::END_EDIT:
        {
            if (uiM->action == MainToAudioMsg::BEGIN_EDIT)
            {
                beginEndParamGestureCount++;
            }
            else
            {
                beginEndParamGestureCount--;
            }
            clap_event_param_gesture_t p;
            p.header.size = sizeof(clap_event_param_gesture_t);
            p.header.time = 0;
            p.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            p.header.type = uiM->action == MainToAudioMsg::BEGIN_EDIT
                                ? CLAP_EVENT_PARAM_GESTURE_BEGIN
                                : CLAP_EVENT_PARAM_GESTURE_END;
            p.header.flags = 0;
            p.param_id = uiM->paramId;

            outq->try_push(outq, &p.header);
        }
        break;
        case MainToAudioMsg::STOP_AUDIO:
        {
            if (lagHandler.active)
                lagHandler.instantlySnap();
            audioRunning = false;
        }
        break;
        case MainToAudioMsg::START_AUDIO:
        {
            audioRunning = true;
        }
        break;
        case MainToAudioMsg::SEND_PATCH_NAME:
        {
            memset(patch.name, 0, sizeof(patch.name));
            strncpy(patch.name, uiM->uiManagedPointer, 255);
            audioToUi.push({AudioToUIMsg::SET_PATCH_NAME, 0, 0, 0, patch.name});
        }
        break;
        case MainToAudioMsg::SEND_PATCH_IS_CLEAN:
        {
            patch.dirty = false;
            audioToUi.push({AudioToUIMsg::SET_PATCH_DIRTY_STATE, patch.dirty});
        }
        break;
        case MainToAudioMsg::SEND_POST_LOAD:
        {
            postLoad();
        }
        break;
        case MainToAudioMsg::SEND_PREP_FOR_STREAM:
        {
            prepForStream();
        }
        break;
        case MainToAudioMsg::SEND_REQUEST_RESCAN:
        {
            onMainRescanParams = true;
            audioToUi.push({AudioToUIMsg::DO_PARAM_RESCAN});
            clapHost->request_callback(clapHost);
        }
        break;
        case MainToAudioMsg::EDITOR_ATTACH_DETATCH:
        {
            const bool attached = uiM->paramId != 0;
            isEditorAttached.store(attached, std::memory_order_relaxed);
            if (attached)
            {
                onMainRescanInfo = true;
                clapHost->request_callback(clapHost);
            }
        }
        break;
        }
        uiM = mainToAudio.pop();
    }
}

void Engine::handleParamValue(Param *p, uint32_t pid, float value)
{
    if (!p)
    {
        p = patch.paramMap.at(pid);
    }

    // p->value = value;
    p->lag.setTarget(value);
    paramLagSet.addToActive(p);

    AudioToUIMsg au = {AudioToUIMsg::UPDATE_PARAM, pid, value};
    audioToUi.push(au);
}

void Engine::pushFullUIRefresh()
{
    for (const auto *p : patch.params)
    {
        AudioToUIMsg au = {AudioToUIMsg::UPDATE_PARAM, p->meta.id, p->value};
        audioToUi.push(au);
    }
    audioToUi.push({AudioToUIMsg::SET_PATCH_NAME, 0, 0, 0, patch.name});
    audioToUi.push({AudioToUIMsg::SET_PATCH_DIRTY_STATE, patch.dirty});
    audioToUi.push({AudioToUIMsg::SEND_SAMPLE_RATE, 0, (float)sampleRate});
}

void Engine::onMainThread()
{
    bool ex{true}, re{false};
    if (onMainRescanParams.compare_exchange_strong(ex, re))
    {
        auto pe = static_cast<const clap_host_params_t *>(
            clapHost->get_extension(clapHost, CLAP_EXT_PARAMS));
        if (pe)
        {
            pe->rescan(clapHost, CLAP_PARAM_RESCAN_VALUES | CLAP_PARAM_RESCAN_TEXT);
        }
    }
    ex = true;
    re = false;
    if (onMainRescanInfo.compare_exchange_strong(ex, re))
    {
        auto pe = static_cast<const clap_host_params_t *>(
            clapHost->get_extension(clapHost, CLAP_EXT_PARAMS));
        if (pe)
            pe->rescan(clapHost, CLAP_PARAM_RESCAN_INFO | CLAP_PARAM_RESCAN_VALUES);
    }
}

} // namespace soundemote::sabrina
