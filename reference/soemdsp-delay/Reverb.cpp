#include <soemdsp/delay/Reverb.hpp>

using std::clamp;
using std::function;
using std::min;
using std::vector;

namespace soemdsp::delay {

void Reverb::connect() {
    clipperL_.width_.pointTo(clippingThreshold_);
    clipperR_.slave(clipperL_);

    for (size_t i = 0; i < delaysL_.size(); ++i) {
        delaysL_[i].bufferSize_.pointTo(bufferSize_);
        delaysR_[i].bufferSize_.pointTo(bufferSize_);

        delaysL_[i].lfoAmp_.pointTo(lfoAmp_);
        delaysR_[i].lfoAmp_.pointTo(lfoAmp_);

        delaysL_[i].lfoVariation_.pointTo(lfoVariation_);
        delaysR_[i].lfoVariation_.pointTo(lfoVariation_);

        delaysL_[i].lfoFrequency_.pointTo(lfoFrequency_);
        delaysR_[i].lfoFrequency_.pointTo(lfoFrequency_);

        delaysL_[i].feedback_.pointTo(diffusionAmount_);
        delaysR_[i].feedback_.pointTo(diffusionAmount_);

        delaysL_[i].delayTime_.pointTo(diffusionSize_);
        delaysR_[i].delayTime_.pointTo(diffusionSize_);
    }

    echoDelayL_.bufferSize_.pointTo(bufferSize_);
    echoDelayR_.bufferSize_.pointTo(bufferSize_);

    echoDelayL_.lfoVariation_.pointTo(lfoVariation_);
    echoDelayR_.lfoVariation_.pointTo(lfoVariation_);

    echoDelayL_.lfoAmp_.pointTo(lfoAmp_);
    echoDelayR_.lfoAmp_.pointTo(lfoAmp_);

    echoDelayL_.lfoFrequency_.pointTo(lfoFrequency_);
    echoDelayR_.lfoFrequency_.pointTo(lfoFrequency_);

    // lpfR_.slave(lpfL_);
    hpfR_.slave(hpfL_);
}

void Reverb::init() {
    numDelaysChanged();
    sampleRateChanged();
    reset();
    diffusionSeedChanged();
    lfoFrequencyChanged();
    doModulateEchoChanged();

    hpf_.mode_      = filter::MultiStageFilter::Mode::HIGHPASS;
    hpf_.frequency_ = 1.0;
    hpf_.modeChanged();
    hpf_.frequencyChanged();

    lpf_.mode_      = filter::MultiStageFilter::Mode::LOWPASS;
    lpf_.frequency_ = 20000.0;
    lpf_.modeChanged();
    lpf_.frequencyChanged();

    peak_.mode_   = filter::MultiStageFilter::Mode::PEAK;
    peak_.stages_ = 2;
    peak_.modeChanged();
    peak_.stagesChanged();

    hpfL_.frequency_ = 20.0;
    hpfR_.frequency_ = 20.0;
    hpfL_.sampleRateChanged();
    hpfR_.sampleRateChanged();
}

void Reverb::numDelaysChanged() {
    if (delaysL_.size() == numDelays_) {
        return;
    }
    delaysL_ = vector<ModulatedDelay>(static_cast<size_t>(numDelays_));
    delaysR_ = vector<ModulatedDelay>(static_cast<size_t>(numDelays_));

    connect();
    init();
}

void Reverb::reset() {
    fbL_ = 0.0;
    fbR_ = 0.0;
    for (size_t i = 0; i < delaysL_.size(); i++) {
        delaysL_[i].reset();
        delaysR_[i].reset();
    }
    echoDelayL_.reset();
    echoDelayR_.reset();
    hpf_.reset();
    lpf_.reset();
    peak_.reset();
    hpfL_.reset();
    hpfR_.reset();
}

void Reverb::sampleRateChanged() {
    bufferSize_ = static_cast<int>(SampleRate::freq_ * MAX_DELAY_SECONDS);

    for (size_t i = 0; i < delaysL_.size(); i++) {
        delaysL_[i].init();
        delaysR_[i].init();
    }

    echoDelayL_.init();
    echoDelayR_.init();

    echoTimeChanged();

    hpf_.sampleRateChanged();
    lpf_.sampleRateChanged();
    peak_.sampleRateChanged();
    hpfL_.sampleRateChanged();
    hpfR_.sampleRateChanged();
}

void Reverb::diffusionSizeChanged() {
    for (size_t i = 0; i < delaysL_.size(); i++) {
        delaysL_[i].delayTimeChanged();
        delaysR_[i].delayTimeChanged();
    }
}

void Reverb::diffusionSeedChanged() {
    global::randomengine.reset();
    for (size_t i = 0; i < delaysL_.size(); i++) {
        delaysL_[i].diffusionSeedChanged();
        delaysR_[i].diffusionSeedChanged();
    }
}

void Reverb::echoTimeChanged() {
    echoDelayL_.delaySamples_ = clamp(SampleRate::timeToSamples(echoTime_), 1.0, static_cast<double>(bufferSize_ - 1));
    echoDelayR_.delaySamples_ = echoDelayL_.delaySamples_;
}

void Reverb::lfoFrequencyChanged() {
    for (size_t i = 0; i < delaysL_.size(); i++) {
        delaysL_[i].lfoChanged();
        delaysR_[i].lfoChanged();
    }
    echoDelayL_.lfoChanged();
    echoDelayR_.lfoChanged();
}

void Reverb::lfoVariationChanged() {
    for (size_t i = 0; i < delaysL_.size(); i++) {
        delaysL_[i].lfoChanged();
        delaysR_[i].lfoChanged();
    }
    echoDelayL_.lfoChanged();
    echoDelayR_.lfoChanged();
}

void Reverb::doModulateEchoChanged() {
    if (doModulateEcho_) {
        echoDelayL_.lfoAmp_.pointTo(lfoAmp_);
        echoDelayR_.lfoAmp_.pointTo(lfoAmp_);
    } else {
        echoDelayL_.lfoAmp_.disconnect();
        echoDelayR_.lfoAmp_.disconnect();
    }
}

void Reverb::clippingThresholdChanged() {
    clipperL_.updateCoeffs();
    feedbackCompensation_ = convert::db_to_amp(abs(min(convert::amp_to_db(clippingThreshold_), 0.0)));
}

void Reverb::runWithIdleDetection(double inL, double inR) {
    inL_ = inL;
    inR_ = inR;

    if (silenceDetector_.run(inL + inR + fbL_ + fbR_ + wetL_ + wetR_ + dryL_ + dryR_)) {
        return;
    }

    // wetL_ = clipperL_.run(inL_) * feedbackCompensation_;
    // wetR_ = clipperR_.run(inR_) * feedbackCompensation_;
    // return;

    std::function feedbackfilter = [&](bool reverseStereo) {
        fbL_ = hpfL_.run(fbL_);
        fbR_ = hpfR_.run(fbR_);
        if (reverseStereo) {
            lpf_.run(fbL_, fbR_, &fbR_, &fbL_);
        } else {
            lpf_.run(fbL_, fbR_, &fbL_, &fbR_);
        }
    };

    std::function drywet = [&]() {
        dryL_ = inL * (1.0 - mix_) * volume_;
        dryR_ = inR * (1.0 - mix_) * volume_;
        wetL_ = fbL_ * mix_ * volume_;
        wetR_ = fbR_ * mix_ * volume_;
        peak_.run(wetL_, wetR_, &wetL_, &wetR_);
        outL_ = dryL_ + wetL_;
        outR_ = dryR_ + wetR_;
    };

    switch (convert::double_to_enum<EchoMode>(echoMode_)) {
    case EchoMode::PostDelay: {

        // echo
        fbL_ = inL_ + echoDelayL_.runDelay(fbL_);
        fbR_ = inR_ + echoDelayR_.runDelay(fbR_);

        // reverbertation
        for (size_t i = 0; i < delaysL_.size(); ++i) {
            fbL_ = delaysL_[i].runDiffuse(fbL_);
            fbR_ = delaysR_[i].runDiffuse(fbR_);
        }

        drywet();
        feedbackfilter(false);

        // feedback clipper
        fbL_ = clipperL_.run(fbL_) * feedbackCompensation_;
        fbR_ = clipperR_.run(fbR_) * feedbackCompensation_;
        break;
    }
    case EchoMode::PreDelay: {
        feedbackfilter(false);

        // feedback clipper
        fbL_ = clipperL_.run(inL + fbL_) * feedbackCompensation_;
        fbR_ = clipperR_.run(inR + fbR_) * feedbackCompensation_;

        // reverbertation
        for (size_t i = 0; i < delaysL_.size(); ++i) {
            fbL_ = delaysL_[i].runDiffuse(fbL_);
            fbR_ = delaysR_[i].runDiffuse(fbR_);
        }

        // echo
        fbL_ = echoDelayL_.runDelay(fbL_);
        fbR_ = echoDelayR_.runDelay(fbR_);

        drywet();
        break;
    }
    case EchoMode::Slapback: {
        // echo
        fbL_ = echoDelayL_.runDelay(fbL_);
        fbR_ = echoDelayR_.runDelay(fbR_);

        // reverbertation
        double L{ inL_ / static_cast<double>(delaysL_.size()) };
        double R{ inR_ / static_cast<double>(delaysL_.size()) };
        for (size_t i = 0; i < delaysL_.size(); ++i) {
            fbL_ = delaysL_[i].runDiffuse(L + fbL_);
            fbR_ = delaysR_[i].runDiffuse(R + fbR_);
        }

        drywet();
        feedbackfilter(true);

        // feedback clipper
        fbL_ = clipperL_.run(fbL_) * feedbackCompensation_;
        fbR_ = clipperR_.run(fbR_) * feedbackCompensation_;

        break;
    }
    default: {
        debug::FAIL();
    }
    }

    // feedback amplifier
    fbL_ *= recycle_;
    fbR_ *= recycle_;
}
} // namespace soemdsp::delay
