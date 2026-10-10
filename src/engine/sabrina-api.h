#pragma once

#include <stdint.h>

// C ABI from dsp/sabrina_reverb.cpp (vendored sandbox kernel).

#ifdef __cplusplus
extern "C" {
#endif

int soemdsp_sabrina_reverb_create(double sampleRate);
void soemdsp_sabrina_reverb_destroy(int handle);
void soemdsp_sabrina_reverb_reset(int handle, double sampleRate);
void soemdsp_sabrina_reverb_set_params(int handle, double mix, double diffusionSize,
                                      double diffusionAmount, double delaySize, double recycle,
                                      double lfoAmplitude, double lfoBaseSpeed,
                                      double lfoVariation, double seed, double lfoStyle);
void soemdsp_sabrina_reverb_process(int handle, double leftInput, double rightInput);
double soemdsp_sabrina_reverb_left(int handle);
double soemdsp_sabrina_reverb_right(int handle);
double soemdsp_sabrina_reverb_wet_left(int handle);
double soemdsp_sabrina_reverb_wet_right(int handle);
int soemdsp_sabrina_reverb_is_idle(int handle);
int soemdsp_sabrina_reverb_version();

#ifdef __cplusplus
}
#endif
