# Sabrina Reverb

<img width="250" height="500" alt="cUJXUGU65P" src="https://github.com/user-attachments/assets/13b05894-5eeb-4f73-a724-4f812b8fd167" />

CLAP-first (VST3/AUv2 via clap-wrapper) stereo reverb plugin for Soundemote.

KVR thread: https://www.kvraudio.com/forum/viewtopic.php?p=9309297

Built from [`baconpaul/sidequest-startingpoint`](https://github.com/baconpaul/sidequest-startingpoint).

DSP is vendored in [`dsp/sabrina_reverb.cpp`](dsp/sabrina_reverb.cpp) (same kernel
as sandbox `reverbEffect`). Walter's original VST is in
[`reference/walter-original-vst/`](reference/walter-original-vst/) for reading.
The object-oriented port lives in [soemdsp](https://github.com/soundemote/soemdsp/tree/main/include/soemdsp/delay) — not duplicated here.

```text
dsp/sabrina_reverb.cpp             <- plugin compiles this
dsp/include/soemdsp/               math headers the kernel needs
reference/walter-original-vst/     Sabrina.cpp / Sabrina.h  (Walter's original VST)
```

## Readable DSP

Start here:

| Path | What it is |
| --- | --- |
| [`reference/walter-original-vst/`](reference/walter-original-vst/) | Walter's original Sabrina VST (`Sabrina.cpp`, `Sabrina.h`) |
| [soemdsp `delay/`](https://github.com/soundemote/soemdsp/tree/main/include/soemdsp/delay) | Soundemote OOP port (`ModulatedDelay`, `Reverb`) |
| [`reference/README.md`](reference/README.md) | How these relate to the shipping DSP |

The plugin **does not compile** those reference files. The live kernel is
[`dsp/sabrina_reverb.cpp`](dsp/sabrina_reverb.cpp).

## Layout on disk

`soemdsp-sandbox` is **not** required. Clone `soemdsp` next to this repo
(or pass `-DSOEMDSP_ROOT=`) for the Parameter / plugin headers.

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

Stereo effect plugin. DSP is `dsp/sabrina_reverb.cpp` (Parabol / FBM).
UI is the sandbox-style module face (title, waterfall, full-width sliders),
not the SideQuest knob grid. Synth `Voice` files are still in the tree and
unused on the audio path.

## License

**DSP is MIT.** Copyright (c) Walter Hackett — original Sabrina reverb.
Take the algorithm / native kernel / `reference/` DSP files under MIT.

**UI is GPL / AGPL** because the editor links [JUCE](https://juce.com) (AGPLv3
unless you have a JUCE commercial licence). That applies to the UI and to
plugin binaries that contain JUCE. It does not re-license the DSP.

Details: [`LICENSE.md`](LICENSE.md).
