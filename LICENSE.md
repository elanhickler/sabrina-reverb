# Licenses

Sabrina Reverb is **two licenses**, split by what the code is.

## DSP — MIT

The reverb itself is MIT. Copyright (c) Walter Hackett.

That covers:

- the original Sabrina algorithm (`reference/walter-original-vst/`)
- the shipping native kernel (`soemdsp-sandbox` `sabrina_reverb.cpp`, linked by this plugin)
- soemdsp delay / reverb library copies (`reference/soemdsp-delay/`)

You may take those DSP sources under MIT without taking the UI.

Full MIT text is at the bottom of this file. Keep the copyright notice with copies
of that Software.

## UI — GPL / AGPL (JUCE)

The plugin editor, windowing, and CLAP/VST wrapper use
[JUCE](https://juce.com), which is licensed under the
[GNU Affero General Public License v3](https://www.gnu.org/licenses/agpl-3.0.en.html)
unless you have a JUCE commercial licence.

That GPL/AGPL obligation is **for the UI and for any binary that links JUCE**.
It does **not** re-license the DSP sources. If you reuse only the MIT DSP
files, you are not taking JUCE and you are not under AGPL for that reuse.

The full AGPLv3 text as shipped with JUCE is in `libs/JUCE/LICENSE.md`.

---

## MIT License (DSP)

Copyright (c) Walter Hackett

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
