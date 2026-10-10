#pragma once

#include <soemdsp/delay/ModulatedDelay.hpp>
#include <soemdsp/dynamics/SilenceDetector.hpp>
#include <soemdsp/dynamics/SoftClipper.hpp>
#include <soemdsp/filter/MultiStageFilter.hpp>
#include <soemdsp/filter/OnePoleFilter.hpp>
#include <soemdsp/random/Random.hpp>
#include <soemdsp/timer/Counter.hpp>

namespace soemdsp::delay {
struct Reverb {
    Reverb() {
        init();
    }

    enum class EchoMode {
        PostDelay,
        PreDelay,
        Slapback
    };

    // io
    void connect(); // populate Wires by linking to the needed parameter which the rest of the plugin will derive values from, then run this to connect parameters to the other dsp objects
    void init();
    void reset();
    void runWithIdleDetection(double inL, double inR);
    double inL_{}, inR_{};   // input signal
    double fbL_{}, fbR_{};   // feedback signal
    double outL_{}, outR_{}; // output signal
    double dryL_{}, dryR_{}; // dry only signal with dry/wet adjustment
    double wetL_{}, wetR_{}; // wet only signal with dry/wet adjustment

    // settings
    const double MAX_DELAY_SECONDS{ 4.0 };

    // parameters
    void sampleRateChanged();        // bufferSize
    void numDelaysChanged();         // numDelays_
    void echoTimeChanged();          // echoTime_
    void diffusionSizeChanged();     // diffusionSize_
    void diffusionSeedChanged();     // call this for global random seed change
    void lfoFrequencyChanged();      // lfoFrequency_, lfoVariation_
    void lfoVariationChanged();      // lfoVariation_
    void doModulateEchoChanged();    // doModulateEcho_
    void clippingThresholdChanged(); // clippingThreshold_

    /* GAIN */
    Wire mix_{ 0.43 };              // 0.0 to 1.0
    Wire clippingThreshold_{ 1.0 }; // 0.0 to 2.0 controls clipping threshold TODO: find a better name
    Wire volume_;                   // 0.0 to 2.0 amplitude for entire output
    /* LFO */
    Wire lfoAmp_;
    Wire lfoFrequency_{ 0.83 };  // 0.0 to half samplerate
    Wire lfoVariation_{ 0.001 }; // 0.0 to 1.0 controls randomized lfo phase range
    /* ECHO */
    Wire echoTime_{ 1.0 }; // 0.0 to MAX_DELAY_SECONDS controls echos' delay times
    Wire recycle_{ 0.5 };  // 0.0 to 2.0 feedback for echo amplitude after clipping
    Wire echoMode_;        // uses EchoMode enum
    Wire doModulateEcho_;  // 0 or 1
    /* DIFFUSION */
    Wire numDelays_{ 10 };         // 0 to 50, number of delays to use for diffusion
    Wire diffusionSize_{ 0.35 };   // 0.0 to 1.0 controls diffusion delay times
    Wire diffusionAmount_{ 0.70 }; // 0.0 to 1.0 controls diffusion feedback gain

    // coefficients
    Wire bufferSize_{ SampleRate::freq_ * MAX_DELAY_SECONDS };
    double feedbackCompensation_{};

    // objects
    std::vector<delay::ModulatedDelay> delaysL_{}; // diffusion delays DO NOT INCREASE SIZE VIA RESIZE, construct from scratch!
    std::vector<delay::ModulatedDelay> delaysR_{}; // diffusion delays DO NOT INCREASE SIZE VIA RESIZE, construct from scratch!
    delay::ModulatedDelay echoDelayL_, echoDelayR_;

    filter::StereoMultiStageFilter lpf_, hpf_, peak_;
    filter::OnePoleHP hpfL_, hpfR_;
    dynamics::SoftClipper clipperL_;
    dynamics::SoftClipper clipperR_;
    dynamics::SilenceDetector silenceDetector_;
};
} // namespace soemdsp::delay
