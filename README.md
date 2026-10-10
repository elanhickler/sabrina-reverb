# Sabrina Reverb CLAP

CLAP-first (VST3/AUv2 via clap-wrapper) stereo reverb plugin for Soundemote.

Built from [`baconpaul/sidequest-startingpoint`](https://github.com/baconpaul/sidequest-startingpoint).

DSP is the sandbox native module (same as `reverbEffect` / `sabrina_reverb.wasm`).
Loose, readable copies of Walter's original VST and the soemdsp delay/reverb
library live in [`reference/`](reference/) — they are **not** compiled into the
plugin.

```text
reference/
  README.md
  walter-original-vst/     Sabrina.cpp / Sabrina.h  (Walter's original VST)
  soemdsp-delay/           ModulatedDelay + Reverb   (soemdsp library)

soemdsp-sandbox/native_modules/sabrina_reverb/
  sabrina_reverb.cpp       <- plugin links this
  sabrina_reverb.wasm
  originalcode/            same VST sources as reference/walter-original-vst
```

Host parameters match the sandbox `reverbEffect` control scheme, including
**Mod Style: Parabol / FBM**.

```text
sabrina-reverb-clap   -> DAW plugin shell (this repo)
soemdsp-sandbox       -> Sabrina DSP + control surface (required sibling)
```

## Readable DSP

Want the algorithm as C++ you can sit down and read? Start here:

| Path | What it is |
| --- | --- |
| [`reference/walter-original-vst/`](reference/walter-original-vst/) | Walter's original Sabrina VST (`Sabrina.cpp`, `Sabrina.h`) |
| [`reference/soemdsp-delay/`](reference/soemdsp-delay/) | soemdsp `ModulatedDelay` + `Reverb` (library, not the plugin kernel) |
| [`reference/README.md`](reference/README.md) | How these relate to the shipping DSP |

The plugin **does not compile** those files. The live kernel is the sibling
sandbox module `sabrina_reverb.cpp`.

## Layout on disk

Clone this repo next to `soemdsp-sandbox` (required, or `-DSOEMDSP_SANDBOX=`).
`soemdsp` is optional (Parameter stack + delay/reverb library).

## Build (Windows)

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Artifacts land under `build/sabrina-reverb-clap_assets/`.

## Controls (sandbox `reverbEffect`)

| Param | Label | Notes |
| --- | --- | --- |
| mix | Mix | Dry/wet on Mix outs |
| diffusionSize | Size | Diffusion network size |
| diffusionAmount | Diffusion | Early diffusion strength |
| delaySize | Tape Memory | Main reverb delay length |
| recycle | Rec | Feedback / tail |
| lfoAmplitude | Mod Amp | Delay-mod depth |
| lfoBaseSpeed | Mod Speed | Delay-mod base speed |
| lfoVariation | Mod Vary | Randomized mod variation |
| lfoStyle | Mod Style | **0 Parabol / 1 FBM** |
| seed | Seed | Delay-line / mod pattern |
| amplitude | Amplitude | Output scale |

## Status

Scaffold: sidequest template renamed, stereo audio I/O, sandbox
`sabrina_reverb.cpp` linked, sandbox control surface including FBM.
UI is still the generic knob grid. Voice/synth scaffolding remains
in-tree but is unused on the audio path.

## License

**DSP is MIT.** Copyright (c) Walter Hackett — original Sabrina reverb.
Take the algorithm / native kernel / `reference/` DSP files under MIT.

**UI is GPL / AGPL** because the editor links [JUCE](https://juce.com) (AGPLv3
unless you have a JUCE commercial licence). That applies to the UI and to
plugin binaries that contain JUCE. It does not re-license the DSP.

Details: [`LICENSE.md`](LICENSE.md).
