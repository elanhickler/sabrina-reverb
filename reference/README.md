# Readable DSP reference (not compiled)

These DSP files are **MIT**, Copyright (c) Walter Hackett. The plugin UI is
JUCE (AGPLv3); that does not apply to this folder. See `/LICENSE.md`.

These files are **loose copies for reading**. They are not part of the plugin
build. The shipping kernel is `dsp/sabrina_reverb.cpp`.

## `walter-original-vst/`

Walter's original Sabrina VST sources (`Sabrina.cpp`, `Sabrina.h`).

Copied from Walter's original Sabrina VST sources.
Includes the VST GUI/editor and the original delay/reverb processing.

They still `#include` VST2 / Windows / lunasvg headers. Open them as a
narrative of the algorithm, not as a drop-in compile unit.

## Object-oriented port (not in this repo)

Soundemote's OOP / Wire-parameter port lives in soemdsp:

https://github.com/soundemote/soemdsp/tree/main/include/soemdsp/delay

(`ModulatedDelay`, `Reverb`). That is the source of truth; it is not duplicated
here.
