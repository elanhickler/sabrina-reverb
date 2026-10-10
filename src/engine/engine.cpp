/*
 * SideQuest Starting Point
 *
 * Basically lets paul bootstrap his projects.
 *
 * Copyright 2024-2025, Paul Walker and Various authors, as described in the github
 * transaction log.
 *
 * This source repo is released under the MIT license, but has
 * GPL3 dependencies, as such the combined work will be
 * released under GPL3.
 *
 * The source code and license are at https://github.com/baconpaul/sidequest-startingpoint
 */

#include "engine/engine.h"
#include <algorithm>
#include <cmath>
#include "sst/cpputils/constructors.h"
#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/PanLaws.h"

#include "libMTSClient.h"

namespace baconpaul::sidequest_ns
{

int debugLevel{0};

namespace mech = sst::basic_blocks::mechanics;
namespace sdsp = sst::basic_blocks::dsp;

Engine::Engine()
    : responder(*this), monoResponder(*this),
      voices(sst::cpputils::make_array<Voice, VMConfig::maxVoiceCount>(patch))
{
    voiceManager = std::make_unique<voiceManager_t>(responder, monoResponder);
    sabrinaHandle = soemdsp_sabrina_reverb_create(48000.0);
    syncSabrinaParamsFromPatch();
}

Engine::~Engine()
{
    if (sabrinaHandle > 0)
    {
        soemdsp_sabrina_reverb_destroy(sabrinaHandle);
        sabrinaHandle = 0;
    }
}

void Engine::syncSabrinaParamsFromPatch()
{
    if (sabrinaHandle <= 0)
        return;

    auto &p = patch.sabrina;
    // Host/CLAP knobs carry real DSP numbers. soemdsp Parameter stores
    // the same mapped value and a linear copy for 0..1 modulation.
    soem.mix.setMapped(p.mix.value);
    soem.diffusionSize.setMapped(p.diffusionSize.value);
    soem.diffusionAmount.setMapped(p.diffusionAmount.value);
    soem.delaySize.setMapped(p.delaySize.value);
    soem.recycle.setMapped(p.recycle.value);
    soem.lfoAmplitude.setMapped(p.lfoAmplitude.value);
    soem.lfoBaseSpeed.setMapped(p.lfoBaseSpeed.value);
    soem.lfoVariation.setMapped(p.lfoVariation.value);
    soem.lfoStyle.setMapped(p.lfoStyle.value);
    soem.seed.setMapped(p.seed.value);
    soem.amplitude.setMapped(p.amplitude.value);

    soemdsp_sabrina_reverb_set_params(
        sabrinaHandle, soem.mix.getDspDouble(), soem.diffusionSize.getDspDouble(),
        soem.diffusionAmount.getDspDouble(), soem.delaySize.getDspDouble(),
        soem.recycle.getDspDouble(), soem.lfoAmplitude.getDspDouble(),
        soem.lfoBaseSpeed.getDspDouble(), soem.lfoVariation.getDspDouble(),
        soem.seed.getDspDouble(), soem.lfoStyle.getDspDouble());
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

    // midi is a bit less frequent than param automation so a slightly slower smooth
    midiCCLagCollection.setRateInMilliseconds(1000.0 * 128.0 / 48000.0, sampleRate,
                                              1.0 / blockSize);
    midiCCLagCollection.snapAllActiveToTarget();

    if (sabrinaHandle > 0)
        soemdsp_sabrina_reverb_reset(sabrinaHandle, sampleRate);
    else
        sabrinaHandle = soemdsp_sabrina_reverb_create(sampleRate);
    syncSabrinaParamsFromPatch();

    audioToUi.push({AudioToUIMsg::SEND_SAMPLE_RATE, 0, (float)sampleRate});
}

void Engine::process(const clap_output_events_t *outq)
{
    process(outq, nullptr, 0, 0);
}

bool Engine::isIdle() const
{
    return sabrinaHandle > 0 && soemdsp_sabrina_reverb_is_idle(sabrinaHandle) != 0;
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

    midiCCLagCollection.processAll();

    lagHandler.process();

    syncSabrinaParamsFromPatch();

    const bool hasInput = inputChannels != nullptr && inputChannelCount > 0 &&
                          inputChannels[0] != nullptr && inputFrameCount > 0;
    const float amp = static_cast<float>(soem.amplitude.getDspDouble());

    // Planck 1e-7 — same as sabrina_reverb.cpp / soemdsp SilenceDetector.
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
        return;
    }

    for (int i = 0; i < blockSize; ++i)
    {
        const bool validFrame = hasInput && static_cast<uint32_t>(i) < inputFrameCount;
        const double inL = validFrame ? inputChannels[0][i] : 0.0;
        const double inR = validFrame && inputChannelCount > 1 && inputChannels[1] != nullptr
                               ? inputChannels[1][i]
                               : inL;

        if (sabrinaHandle > 0)
        {
            soemdsp_sabrina_reverb_process(sabrinaHandle, inL, inR);
            output[0][i] = static_cast<float>(soemdsp_sabrina_reverb_left(sabrinaHandle) * amp);
            output[1][i] = static_cast<float>(soemdsp_sabrina_reverb_right(sabrinaHandle) * amp);
        }
        else
        {
            output[0][i] = static_cast<float>(inL * amp);
            output[1][i] = static_cast<float>(inR * amp);
        }
    }

    if (isEditorAttached)
    {
        float peakL = 0.f, peakR = 0.f;
        for (int i = 0; i < blockSize; ++i)
        {
            peakL = std::max(peakL, std::abs(output[0][i]));
            peakR = std::max(peakR, std::abs(output[1][i]));
        }
        // blockSize is 8 (~6k blocks/sec). Only post ~60 scope columns/sec.
        if (++lastScopeUpdate >= 100)
        {
            audioToUi.push({AudioToUIMsg::SCOPE_PEAK, 0, peakL, peakR});
            lastScopeUpdate = 0;
        }
    }
}

void Engine::addToVoiceList(Voice *v)
{
    v->prior = nullptr;
    v->next = head;
    if (v->next)
    {
        v->next->prior = v;
    }
    head = v;
    voiceCount++;
}

Voice *Engine::removeFromVoiceList(Voice *cvoice)
{
    if (cvoice->prior)
    {
        cvoice->prior->next = cvoice->next;
    }
    if (cvoice->next)
    {
        cvoice->next->prior = cvoice->prior;
    }
    if (cvoice == head)
    {
        head = cvoice->next;
    }
    auto nv = cvoice->next;
    cvoice->cleanup();
    cvoice->next = nullptr;
    cvoice->prior = nullptr;
    voiceCount--;

    return nv;
}

void Engine::dumpVoiceList()
{
    SQLOG("DUMP VOICE LIST : head=" << std::hex << head << std::dec);
    auto c = head;
    while (c)
    {
        // SQLOG("   c=" << std::hex << c << std::dec << " key=" << c->voiceValues.key
        //                 << " u=" << c->used);
        c = c->next;
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
            voiceManager->allSoundsOff();
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
            isEditorAttached = uiM->paramId;
        }
        break;
        case MainToAudioMsg::PANIC_STOP_VOICES:
        {
            voiceManager->allSoundsOff();
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
}

} // namespace baconpaul::sidequest_ns
