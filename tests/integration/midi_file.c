/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-test-config.h"

#include "audio/engine_dummy.h"
#include "audio/midi_event.h"
#include "audio/midi_track.h"
#include "audio/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

#define BUFFER_SIZE 20
#define LARGE_BUFFER_SIZE 2000
/* around 100 causes OOM in various CIs */
#define MAX_FILES 12

static void
test_midi_file_playback ()
{
  test_helper_zrythm_init ();

  /* create a track for testing */
  int track_pos = TRACKLIST->num_tracks;
  GError * err = NULL;
  tracklist_selections_action_perform_create_midi (
    track_pos, 1, &err);
  g_assert_null (err);

  MidiEvents * events = midi_events_new ();

  char ** midi_files =
    io_get_files_in_dir_ending_in (
      MIDILIB_TEST_MIDI_FILES_PATH,
      F_RECURSIVE, ".MID", false);
  g_assert_nonnull (midi_files);

  /* shuffle array */
  int count = 0;
  char * midi_file;
  while ((midi_file = midi_files[count++]));
  srandom ((unsigned int) time (NULL));
  array_shuffle (
    midi_files, (size_t) (count - 1),
    sizeof (char *));

  int iter = 0;
  Position init_pos;
  position_set_to_bar (&init_pos, 1);
  while ((midi_file = midi_files[iter++]))
    {
      /*if (!g_str_has_suffix (midi_file, "M23.MID"))*/
        /*continue;*/
      g_message ("testing %s", midi_file);

      SupportedFile * file =
        supported_file_new_from_path (midi_file);
      track_create_with_action (
        TRACK_TYPE_MIDI, NULL, file, PLAYHEAD,
        TRACKLIST->num_tracks, 1, NULL);
      supported_file_free (file);
      g_message ("testing %s", midi_file);

      Track * track =
        tracklist_get_last_track (
          TRACKLIST, TRACKLIST_PIN_OPTION_BOTH,
          true);

      ZRegion * region =
        track->lanes[0]->regions[0];

      /* set start pos to a bit before the first
       * note, send end pos a few cycles later */
      Position start_pos, stop_pos;
      position_set_to_pos (
        &start_pos,
        &((ArrangerObject *)
        region->midi_notes[0])->pos);
      position_set_to_pos (&stop_pos, &start_pos);
      position_add_frames (
        &start_pos, - BUFFER_SIZE * 2);
      position_add_frames (
        /* run 80 cycles */
        &stop_pos, BUFFER_SIZE * 80);

      /* set region end somewhere within the
       * covered positions to check for warnings */
      Position tmp;
      position_set_to_pos (&tmp, &start_pos);
      position_add_frames (
        &tmp, BUFFER_SIZE * 32 + BUFFER_SIZE / 3);
      arranger_object_end_pos_setter (
        (ArrangerObject *) region,
        &tmp);

      /* start filling events to see if any
       * warnings occur */
      for (long i = start_pos.frames;
           i < stop_pos.frames;
           i += BUFFER_SIZE)
        {
          /* try all possible offsets */
          for (int j = 0; j < BUFFER_SIZE; j++)
            {
              EngineProcessTimeInfo time_nfo = {
                .g_start_frames = i,
                .local_offset = (nframes_t) j,
                .nframes = BUFFER_SIZE, };
              track_fill_events (
                track, &time_nfo, events, NULL);
              midi_events_clear (events, true);
            }
        }

      /* sleep to avoid being killed */
      g_usleep (10000);

      if (iter == MAX_FILES)
        break;
    }
  g_strfreev (midi_files);

  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/integration/midi_file/"

  g_test_add_func (
    TEST_PREFIX "test_midi_file_playback",
    (GTestFunc) test_midi_file_playback);

  return g_test_run ();
}
