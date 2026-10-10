# Readable DSP reference (not compiled)

These DSP files are **MIT**, Copyright (c) Walter Hackett. The plugin UI is
JUCE (AGPLv3); that does not apply to this folder. See `/LICENSE.md`.


These files are **loose copies for reading**. They are not part of the plugin
build. The shipping kernel is `soemdsp-sandbox/native_modules/sabrina_reverb/sabrina_reverb.cpp`.

## `walter-original-vst/`

Walter's original Sabrina VST sources (`Sabrina.cpp`, `Sabrina.h`).

Copied from `soemdsp-sandbox/native_modules/sabrina_reverb/originalcode/`.
Includes the VST GUI/editor and the original delay/reverb processing.

They still `#include` VST2 / Windows / lunasvg headers. Open them as a
narrative of the algorithm, not as a drop-in compile unit.

## `soemdsp-delay/`

soemdsp library delay + reverb (`ModulatedDelay`, `Reverb`).

Copied from `soemdsp/include/soemdsp/delay/`. This is the OOP / Wire-parameter
version of a related reverb (echo mode, num delays, clip, Parabol-only LFO).
It is **not** the plugin DSP. Use it to compare structure with Walter's original
and with the sandbox native module.

They still `#include` other soemdsp headers (`Wire`, filters, SilenceDetector,
…). Follow those includes in a local `soemdsp` checkout if you want the full
call graph.
