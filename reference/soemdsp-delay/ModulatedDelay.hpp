#pragma once

#include <soemdsp/SampleRate.hpp>
#include <soemdsp/modulator/Parabol.hpp>
#include <soemdsp/random/FlexibleRandomWalk.hpp>
#include <soemdsp/random/FractalBrownianMotion.hpp>
#include <soemdsp/random/Random.hpp>
#include <soemdsp/sehelper.hpp>
#include <soemdsp/semath.hpp>
#include <algorithm>

namespace soemdsp::delay {
// class WindowedSincInterpolator {
//   public:
//     WindowedSincInterpolator(double windowSize, size_t maxBufferSize)
//       : windowSize_(windowSize)
//       , maxBufferSize_(maxBufferSize) {
//         buffer_.reserve(maxBufferSize_);
//     }
//
//     // Process one input sample and return the interpolated output sample
//     double processSample(double inputSample) {
//         buffer_.push_back(inputSample);
//         if (buffer_.size() > maxBufferSize_) {
//             buffer_.erase(buffer_.begin()); // Remove the oldest sample
//         }
//
//         // Perform windowed sinc interpolation
//         double interpolatedSample = 0.0;
//         for (size_t i = 0; i < buffer_.size(); ++i) {
//             double sincValue = buffer_[i] * windowFunction(i - buffer_.size() + 1, windowSize_);
//             interpolatedSample += sincValue;
//         }
//
//         return interpolatedSample;
//     }
//
//   private:
//     // Define a window function (e.g., Hamming window)
//     double windowFunction(double x, double windowSize) {
//         if (x < -windowSize || x > windowSize) {
//             return 0.0;
//         } else {
//             return 0.54 - 0.46 * std::cos(2.0 * kPI * x / (2.0 * windowSize));
//         }
//     }
//
//     std::vector<double> buffer_;
//     double windowSize_;
//     size_t maxBufferSize_;
// };

struct ModulatedDelay {
    ModulatedDelay() {
        init();
    };

    // io
    void init();
    void reset();
    bool isidle() {
        return false;
    }
    double runDiffuse(double in);
    double runDelay(double in);
    double runLfo();
    double in_{};
    double out_{};
    double lfo_{};

    // parameters
    void sampleRateChanged();    // bufferSize_
    void delayTimeChanged();     // delayTime_
    void diffusionSeedChanged(); // diffusionSize_
    void lfoChanged();           // lfoFrequency_ lfoVariation_
    Wire feedback_;
    Wire delayTime_{ 0.06 };
    double diffusionSize_{};
    Wire lfoAmp_;
    Wire bufferSize_{ 44100 * 4 };
    Wire lfoFrequency_{ 0.01 };
    Wire lfoVariation_{ 0.001 };

    // coefficients
    double bufferOffset_{};
    double lfoInc_{};
    double lfoPhase_{};
    int rndNext_{};
    double rndAcc_{};
    int bufferPos_{};
    Wire delaySamples_{ 1.0 };

    // objects
    std::vector<double> buffer_;
    modulator::FlexibleRandomWalk randomWalk;

  private:
    double interpolateLinear(const std::vector<double>& buffer, double where);
    int tempseed{};
};

} // namespace soemdsp::delay
