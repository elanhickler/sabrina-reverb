Sabrina Reverb
Source: https://github.com/elanhickler/sabrina-reverb

Original Sabrina reverb by Walter Hackett.

Licenses
- DSP (the reverb): MIT. Copyright (c) Walter Hackett. See License.txt.
- UI (the plugin window / JUCE wrapper): GNU AGPLv3 from JUCE. That GPL/AGPL
  notice is for the UI and for this binary because it links JUCE. It does not
  re-license the DSP. You can reuse the DSP under MIT.

You've downloaded a Windows, macOS, or Linux package containing CLAP and VST3
(macOS also has AU). Copy the plugin into the folder for your OS.

Windows
- CLAP → `%LOCALAPPDATA%\Programs\Common\CLAP\`
- VST3 → `C:\Program Files\Common Files\VST3\`

macOS
- CLAP → `~/Library/Audio/Plug-Ins/CLAP/`
- VST3 → `~/Library/Audio/Plug-Ins/VST3/`
- AU (.component) → `~/Library/Audio/Plug-Ins/Components/`

These Mac builds are not Apple-notarized. After a download, Gatekeeper will
say it cannot tell if they contain malware. That is expected. You are not
doing anything wrong.

In Terminal, after copying the plugins:

  xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Sabrina Reverb.vst3"
  xattr -cr ~/Library/Audio/Plug-Ins/Components/"Sabrina Reverb.component"
  xattr -cr ~/Library/Audio/Plug-Ins/CLAP/"Sabrina Reverb.clap"

Then System Settings → Privacy & Security → Open Anyway if macOS still
blocks them. Restart the DAW and rescan plugins.

Linux
- CLAP → `~/.clap/` or `/usr/lib/clap/`
- VST3 → `~/.vst3/` or `/usr/lib/vst3/`

If you have feedback ideas etc open a github issue on elanhickler/sabrina-reverb.
