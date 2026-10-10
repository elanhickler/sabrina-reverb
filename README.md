# Sabrina Reverb CLAP

CLAP-first (VST3/AUv2 via clap-wrapper) stereo reverb plugin for Soundemote.

Built from [`baconpaul/sidequest-startingpoint`](https://github.com/baconpaul/sidequest-startingpoint).

DSP is the sandbox native module (same as `reverbEffect` / `sabrina_reverb.wasm`),
with the original VST sources next to it for reference:

```text
soemdsp-sandbox/native_modules/sabrina_reverb/
  sabrina_reverb.cpp      <- plugin links this
  sabrina_reverb.wasm
  originalcode/Sabrina.{cpp,h}
```

Host parameters match the sandbox `reverbEffect` control scheme, including
**Mod Style: Parabol / FBM**.

```text
sabrina-reverb-clap   -> DAW plugin shell (this repo)
soemdsp-sandbox       -> Sabrina DSP + control surface (required sibling)
```

## Layout on disk

```text
Documents/_PROGRAMMING/
  sabrina-reverb-clap/   <- this repo
  soemdsp-sandbox/       <- required sibling (or -DSOEMDSP_SANDBOX=...)
```

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

## Attribution

SideQuest Starting Point © Paul Walker / contributors (MIT; combined work GPL3
via JUCE and other deps). See `LICENSE.md`.
