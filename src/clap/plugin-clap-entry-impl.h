/*
 * Sabrina Reverb — Soundemote
 * DSP MIT (Walter Hackett). UI links JUCE (AGPLv3). See LICENSE.md.
 */

#ifndef SOUNDEMOTE_SABRINA_CLAP_PLUGIN_CLAP_ENTRY_IMPL_H
#define SOUNDEMOTE_SABRINA_CLAP_PLUGIN_CLAP_ENTRY_IMPL_H

namespace soundemote::sabrina
{
const void *get_factory(const char *factory_id);
bool clap_init(const char *p);
void clap_deinit();
} // namespace soundemote::sabrina

#endif
