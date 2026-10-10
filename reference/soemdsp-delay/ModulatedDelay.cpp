#include "ModulatedDelay.hpp"


using std::clamp;
using std::vector;

namespace soemdsp::delay {

void ModulatedDelay::init() {
    buffer_.resize(bufferSize_);
    lfoChanged();
    diffusionSeedChanged();
    reset();
    tempseed = global::randomengine.runBipolar();
}

void ModulatedDelay::reset() {
    bufferPos_ = 0;
    lfoPhase_  = global::randomengine.runBipolar();
    buffer_.assign(buffer_.size(), 0.0);
}

void ModulatedDelay::sampleRateChanged() {
    buffer_.resize(bufferSize_);
    lfoChanged();
    delayTimeChanged();
}

double ModulatedDelay::runLfo() {
    // return map<double>(lfoAmp_, 0.0, 0.5, 0.0, 1.0) * biToUni(randomWalk.run());

    // lfoPhase_ += lfoInc_;
    // return map<double>(lfoAmp_, 0.0, 0.5, 0.0, 1.0) * modulator::FractalBrownianMotion(lfoPhase_, 4, map<double>(lfoVariation_, 0.0, 10.0, 0.0, 1.0), tempseed).runUnipolar();

    lfoPhase_ = math::wrap(lfoPhase_ + lfoInc_);
    return lfoAmp_ * convert::to_unipolar(modulator::Parabol::sample(lfoPhase_));
}

double ModulatedDelay::runDelay(double in) {
    in_           = in;
    lfo_          = runLfo();
    bufferOffset_ = (delaySamples_ - (delaySamples_ * (lfo_ * lfoAmp_))) + 1;
    bufferPos_    = (bufferPos_ + 1) % static_cast<int>(bufferSize_);
    out_          = interpolateLinear(buffer_, fmod(bufferPos_ + bufferSize_ - bufferOffset_, bufferSize_));

    buffer_[bufferPos_] = in_; // + out_ * feedback_;
    return out_;
}

double ModulatedDelay::runDiffuse(double in) {
    in_           = in;
    lfo_          = runLfo();
    bufferOffset_ = (delaySamples_ - (delaySamples_ * (lfo_ * lfoAmp_))) + 1;
    bufferPos_    = (bufferPos_ + 1) % static_cast<int>(bufferSize_);
    out_          = interpolateLinear(buffer_, fmod(bufferPos_ + bufferSize_ - bufferOffset_, bufferSize_));

    buffer_[bufferPos_] = (0.0 - in_) - out_ * feedback_;
    out_                = in_ * feedback_ - out_ * (1.0 - feedback_ * feedback_);
    return out_;
}

void ModulatedDelay::delayTimeChanged() {
    delaySamples_ = std::clamp(SampleRate::timeToSamples(delayTime_ * diffusionSize_), 1.0, bufferSize_ - 1.0);
}

void ModulatedDelay::diffusionSeedChanged() {
    diffusionSize_ = global::randomengine.runUnipolar();
    delayTimeChanged();
}

void ModulatedDelay::lfoChanged() {
    double frequency = lfoFrequency_ + math::map0to1<double>(global::randomengine.runUnipolar(), -lfoFrequency_ * lfoVariation_, lfoFrequency_ * lfoVariation_);
    // lfoInc_          = SampleRate::frequencyToIncrement(abs(frequency));

    randomWalk.frequency_ = math::mapNtoN<double>(lfoFrequency_, 0.1, 90.0, .1, 100);
    randomWalk.jitter_    = math::mapNtoN<double>(lfoVariation_, 0.0, 1.0, 0.0, 500);
    ;
    randomWalk.frequencyChanged();
    randomWalk.jitterChanged();

    lfoInc_ = SampleRate::frequencyToIncrement(math::mapNtoN<double>(lfoFrequency_, 0.1, 90, 0.0, 90));
}

double ModulatedDelay::interpolateLinear(const vector<double>& buffer, double where) {
    size_t Before = static_cast<size_t>(where) % static_cast<int>(bufferSize_);
    size_t After  = static_cast<size_t>(where + 1) % static_cast<int>(bufferSize_);
    double mix    = where - static_cast<int>(Before);
    return buffer[Before] * (1.0 - mix) + buffer[After] * mix;
}
} // namespace soemdsp::delay
