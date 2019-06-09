/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef __linux__

#ifndef __AUDIO_ENGINE_ALSA_H__
#define __AUDIO_ENGINE_ALSA_H__

#include <stdlib.h>

typedef struct AudioEngine AudioEngine;

void
engine_alsa_fill_stereo_out_buffs (
  AudioEngine * engine);

/**
 * Copy the cached MIDI events to the MIDI events
 * in the MIDI in port, used at the start of each
 * cycle. */
void
engine_alsa_receive_midi_events (
  AudioEngine * self,
  int           print);

int
alsa_midi_setup (
  AudioEngine * self,
  int           loading);

/**
 * Sets up the audio engine to use alsa.
 */
int
alsa_setup (AudioEngine * self,
            int           loading);

void
alsa_tear_down ();

#endif // header guard
#endif // __linux__
