/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#include <time.h>
#include <sys/stat.h>

#include "zrythm.h"
#include "project.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/modulator_track.h"
#include "audio/port_connections_manager.h"
#include "audio/router.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/dialogs/create_project_dialog.h"
#include "gui/widgets/dialogs/project_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/region.h"
#include "gui/widgets/splash.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_state.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/datetime.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/general.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <zstd.h>

typedef enum
{
  Z_PROJECT_ERROR_FAILED,
} ZProjectError;

#define Z_PROJECT_ERROR \
  z_project_error_quark ()
GQuark z_project_error_quark (void);
G_DEFINE_QUARK (
  z-project-error-quark, z_project_error)

static void
init_common (
  Project * self)
{
  zix_sem_init (&self->save_sem, 1);
}

/**
 * Compresses/decompress a project from a file/data
 * to a file/data.
 *
 * @param compress True to compress, false to
 *   decompress.
 * @param[out] _dest Pointer to a location to allocate
 *   memory.
 * @param[out] _dest_size Pointer to a location to
 *   store the size of the allocated memory.
 * @param _src Input buffer or filepath.
 * @param _src_size Input buffer size, if not
 *   filepath.
 *
 * @return Whether successful.
 */
bool
_project_compress (
  bool                   compress,
  char **                _dest,
  size_t *               _dest_size,
  ProjectCompressionFlag dest_type,
  const char *           _src,
  const size_t           _src_size,
  ProjectCompressionFlag src_type,
  GError **              error)
{
  g_message (
    "using zstd v%d.%d.%d",
    ZSTD_VERSION_MAJOR,
    ZSTD_VERSION_MINOR,
    ZSTD_VERSION_RELEASE);

  char * src = NULL;
  size_t src_size = 0;
  switch (src_type)
    {
    case PROJECT_COMPRESS_DATA:
      src = (char *) _src;
      src_size = _src_size;
      break;
    case PROJECT_COMPRESS_FILE:
      {
        bool ret =
          g_file_get_contents (
            _src, &src, &src_size, error);
        if (!ret)
          {
            return false;
          }
      }
      break;
    }

  char * dest = NULL;
  size_t dest_size = 0;
  if (compress)
    {
      g_message ("compressing project...");
      size_t compress_bound =
        ZSTD_compressBound (src_size);
      dest = malloc (compress_bound);
      dest_size =
        ZSTD_compress (
          dest, compress_bound,
          src, src_size, 1);
      if (ZSTD_isError (dest_size))
        {
          free (dest);

          g_set_error (
            error, Z_PROJECT_ERROR,
            Z_PROJECT_ERROR_FAILED,
            "Failed to compress project file: %s",
            ZSTD_getErrorName (dest_size));
          return false;
        }
    }
  else /* decompress */
    {
#if (ZSTD_VERSION_MAJOR == 1 && \
  ZSTD_VERSION_MINOR < 3)
      unsigned long long const frame_content_size =
        ZSTD_getDecompressedSize (src, src_size);
      if (frame_content_size == 0)
#else
      unsigned long long const frame_content_size =
        ZSTD_getFrameContentSize (src, src_size);
      if (frame_content_size ==
            ZSTD_CONTENTSIZE_ERROR)
#endif
        {
          g_set_error_literal (
            error, Z_PROJECT_ERROR,
            Z_PROJECT_ERROR_FAILED,
            "Project not compressed by zstd");
          return false;
        }
      dest =
        malloc ((size_t) frame_content_size);
      dest_size =
        ZSTD_decompress (
          dest, frame_content_size,
          src, src_size);
      if (ZSTD_isError (dest_size))
        {
          free (dest);

          g_set_error (
            error, Z_PROJECT_ERROR,
            Z_PROJECT_ERROR_FAILED,
            "Failed to decompress project file: %s",
            ZSTD_getErrorName (dest_size));
          return false;
        }
      if (dest_size != frame_content_size)
        {
          free (dest);

          /* impossible because zstd will check
           * this condition */
          g_set_error_literal (
            error, Z_PROJECT_ERROR,
            Z_PROJECT_ERROR_FAILED,
            "uncompressed_size != "
            "frame_content_size");
          return false;
        }
    }

  g_message (
    "%s : %u bytes -> %u bytes",
    compress ? "Compression" : "Decompression",
    (unsigned) src_size,
    (unsigned) dest_size);

  switch (dest_type)
    {
    case PROJECT_COMPRESS_DATA:
      *_dest = dest;
      *_dest_size = dest_size;
      break;
    case PROJECT_COMPRESS_FILE:
      {
        bool ret =
          g_file_set_contents (
            *_dest, dest, (gssize) dest_size,
            error);
        if (!ret)
          return false;
      }
      break;
    }

  return true;
}

/**
 * Frees the current x if any and sets a copy of
 * the given string.
 */
#define DEFINE_SET_STR(x) \
static void \
set_##x ( \
  Project *    self, \
  const char * x) \
{ \
  if (self->x) \
    g_free (self->x); \
  self->x = \
    g_strdup (x); \
}

DEFINE_SET_STR (dir);
DEFINE_SET_STR (title);

#undef DEFINE_SET_STR

/**
 * Returns the filepath of a backup (directory),
 * if any,
 * if it has a newer timestamp than the actual
 * file being loaded.
 *
 * @param dir The non-backup dir.
 */
static char *
get_newer_backup (
  Project * self)
{
  GDir *dir;
  GError *error = NULL;
  const gchar *filename;
  struct stat stat_res;
  struct tm *orig_tm, *nowtm;
  time_t t1;
  time_t t2;

  char * filepath =
    project_get_path (
      self, PROJECT_PATH_PROJECT_FILE, false);
  g_return_val_if_fail (filepath, NULL);

  if (stat (filepath, &stat_res) == 0)
    {
      orig_tm = localtime (&stat_res.st_mtime);
      t1 = mktime (orig_tm);
    }
  else
    {
      g_warning (
        "Failed to get last modified for %s",
        filepath);
      return NULL;
    }
  g_free (filepath);

  char * result = NULL;
  char * backups_dir =
    project_get_path (
      self, PROJECT_PATH_BACKUPS, false);
  dir = g_dir_open (backups_dir, 0, &error);
  if (!dir)
    {
      return NULL;
    }
  while ((filename = g_dir_read_name(dir)))
    {
      char * full_path =
        g_build_filename (
          backups_dir,
          filename,
          PROJECT_FILE,
          NULL);
      g_message ("%s", full_path);

      if (stat (full_path, &stat_res)==0)
        {
          nowtm = localtime (&stat_res.st_mtime);
          t2 = mktime (nowtm);
          /* if backup is after original project */
          if (difftime (t2, t1) > 0)
            {
              if (!result)
                g_free (result);
              result =
                g_build_filename (
                  backups_dir,
                  filename,
                  NULL);
              t1 = t2;
            }
        }
      else
        {
          g_warning (
            "Failed to get last modified for %s",
            full_path);
          g_dir_close (dir);
          g_free (backups_dir);
          return NULL;
        }
      g_free (full_path);
    }
  g_free (backups_dir);
  g_dir_close (dir);

  return result;
}

static void
set_datetime_str (
  Project * self)
{
  if (self->datetime_str)
    g_free (self->datetime_str);
  self->datetime_str =
    datetime_get_current_as_string ();
}

/**
 * Sets the next available backup dir to use for
 * saving a backup during this call.
 */
static void
set_and_create_next_available_backup_dir (
  Project * self)
{
  if (self->backup_dir)
    g_free (self->backup_dir);

  char * backups_dir =
    project_get_path (
      self, PROJECT_PATH_BACKUPS, false);

  int i = 0;
  do
    {
      if (i > 0)
        {
          g_free (self->backup_dir);
          char * bak_title =
            g_strdup_printf (
              "%s.bak%d",
              self->title, i);
          self->backup_dir =
            g_build_filename (
              backups_dir,
              bak_title, NULL);
          g_free (bak_title);
        }
      else
        {
          char * bak_title =
            g_strdup_printf (
              "%s.bak",
              self->title);
          self->backup_dir =
            g_build_filename (
              backups_dir,
              bak_title, NULL);
          g_free (bak_title);
        }
      i++;
    } while (
      file_exists (
        self->backup_dir));
  g_free (backups_dir);

  io_mkdir (self->backup_dir);
}

/**
 * Sets the next available "Untitled Project" title
 * and directory.
 *
 * @param _dir The directory of the project to
 *   create, including its title.
 */
static void
create_and_set_dir_and_title (
  Project *    self,
  const char * _dir)
{
  set_dir (self, _dir);
  char * str = g_path_get_basename (_dir);
  set_title (self, str);
  g_free (str);
}

/**
 * Checks that everything is okay with the project.
 */
void
project_validate (Project * self)
{
  g_message ("%s: validating...", __func__);

#if 0
  size_t max_size = 20;
  Port ** ports =
    object_new_n (max_size, Port *);
  int num_ports = 0;
  port_get_all (
    &ports, &max_size, true, &num_ports);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      port_update_identifier (
        port, &port->id, port->track,
        F_UPDATE_AUTOMATION_TRACK);
    }
  free (ports);
#endif

  tracklist_validate (self->tracklist);

  region_link_group_manager_validate (
    self->region_link_group_manager);

  /* TODO add arranger_object_get_all and check
   * positions (arranger_object_validate) */

  g_message ("%s: done", __func__);
}

#if 0
ArrangerWidget *
project_get_arranger_for_last_selection (
  Project * self)
{
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);
  switch (self->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
      return TL_SELECTIONS;
      break;
    case SELECTION_TYPE_EDITOR:
      if (r)
        {
          switch (r->id.type)
            {
            case REGION_TYPE_AUDIO:
              return AUDIO_SELECTIONS;
            case REGION_TYPE_AUTOMATION:
              return AUTOMATION_SELECTIONS;
            case REGION_TYPE_MIDI:
              return MA_SELECTIONS;
            case REGION_TYPE_CHORD:
              return CHORD_SELECTIONS;
            }
        }
      break;
    default:
      return NULL;
    }

  return NULL;
}
#endif

ArrangerSelections *
project_get_arranger_selections_for_last_selection (
  Project * self)
{
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);
  switch (self->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
      return (ArrangerSelections *) TL_SELECTIONS;
      break;
    case SELECTION_TYPE_EDITOR:
      if (r)
        {
          switch (r->id.type)
            {
            case REGION_TYPE_AUDIO:
              return
                (ArrangerSelections *)
                AUDIO_SELECTIONS;
            case REGION_TYPE_AUTOMATION:
              return
                (ArrangerSelections *)
                AUTOMATION_SELECTIONS;
            case REGION_TYPE_MIDI:
              return
                (ArrangerSelections *)
                MA_SELECTIONS;
            case REGION_TYPE_CHORD:
              return
                (ArrangerSelections *)
                CHORD_SELECTIONS;
            }
        }
      break;
    default:
      return NULL;
    }

  return NULL;
}

static MainWindowWidget *
hide_prev_main_window (void)
{
  event_manager_stop_events (EVENT_MANAGER);

  MainWindowWidget * mww = MAIN_WINDOW;
  MAIN_WINDOW = NULL;
  if (GTK_IS_WIDGET (mww))
    {
      g_message (
        "hiding previous main window...");
      gtk_widget_set_visible (
        GTK_WIDGET (mww), false);
    }

  return mww;
}

static void
destroy_prev_main_window (
  MainWindowWidget * mww)
{
  if (GTK_IS_WIDGET (mww))
    {
      g_message (
        "destroying previous main window...");
      main_window_widget_tear_down (mww);
      g_object_unref (mww);
    }
}

static void
setup_main_window (
  Project * self)
{
  /* mimic behavior when starting the app */
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("setting up main window...");
      event_manager_start_events (EVENT_MANAGER);
      main_window_widget_setup (MAIN_WINDOW);

      EVENTS_PUSH (
        ET_PROJECT_LOADED, self);
    }
}

static void
recreate_main_window (void)
{
  g_message ("recreating main window...");
  MAIN_WINDOW =
    main_window_widget_new (zrythm_app);
  g_warn_if_fail (MAIN_WINDOW->center_dock->main_notebook->timeline_panel->tracklist);
}

/**
 * Initializes the selections in the project.
 *
 * @note
 * Not meant to be used anywhere besides
 * tests and project.c
 */
void
project_init_selections (Project * self)
{
  self->automation_selections =
    (AutomationSelections *)
    arranger_selections_new (
      ARRANGER_SELECTIONS_TYPE_AUTOMATION);
  self->audio_selections =
    (AudioSelections *)
    arranger_selections_new (
      ARRANGER_SELECTIONS_TYPE_AUDIO);
  self->chord_selections =
    (ChordSelections *)
    arranger_selections_new (
      ARRANGER_SELECTIONS_TYPE_CHORD);
  self->timeline_selections =
    (TimelineSelections *)
    arranger_selections_new (
      ARRANGER_SELECTIONS_TYPE_TIMELINE);
  self->midi_arranger_selections =
    (MidiArrangerSelections *)
    arranger_selections_new (
      ARRANGER_SELECTIONS_TYPE_MIDI);
  self->mixer_selections =
    mixer_selections_new ();
  mixer_selections_init (self->mixer_selections);
}

/**
 * Creates a default project.
 *
 * This is only used internally or for generating
 * projects from scripts.
 *
 * @param prj_dir The directory of the project to
 *   create, including its title.
 * @param headless Create the project assuming we
 *   are running without a UI.
 * @param start_engine Whether to also start the
 *   engine after creating the project.
 */
Project *
project_create_default (
  Project *    self,
  const char * prj_dir,
  bool         headless,
  bool         with_engine)
{
  g_message ("creating default project...");

  bool have_ui = !headless && ZRYTHM_HAVE_UI;

  MainWindowWidget * mww = NULL;
  if (have_ui)
    {
      g_message ("hiding prev window...");
      mww = hide_prev_main_window ();
    }

  if (self)
    {
      project_free (self);
    }
  self = project_new (ZRYTHM);

  self->tracklist = tracklist_new (self, NULL);

  /* initialize selections */
  project_init_selections (self);

  self->audio_engine = engine_new (self);

  /* init undo manager */
  self->undo_manager = undo_manager_new ();

  self->title = g_path_get_basename (prj_dir);

  /* init pinned tracks */

  /* chord */
  g_message ("adding chord track...");
  Track * track =
    chord_track_new (TRACKLIST->num_tracks);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  self->tracklist->chord_track = track;

  /* tempo */
  g_message ("adding tempo track...");
  track =
    tempo_track_default (TRACKLIST->num_tracks);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  self->tracklist->tempo_track = track;
  int beats_per_bar =
    tempo_track_get_beats_per_bar (track);
  int beat_unit =
    tempo_track_get_beat_unit (track);
  bpm_t bpm =
    tempo_track_get_current_bpm (track);
  transport_update_caches (
    self->audio_engine->transport, beats_per_bar,
    beat_unit);
  engine_update_frames_per_tick (
    self->audio_engine, beats_per_bar, bpm,
    self->audio_engine->sample_rate, true, true);

  /* modulator */
  g_message ("adding modulator track...");
  track =
    modulator_track_default (TRACKLIST->num_tracks);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  self->tracklist->modulator_track = track;

  /* marker */
  g_message ("adding marker track...");
  track =
    marker_track_default (TRACKLIST->num_tracks);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  self->tracklist->marker_track = track;

  self->tracklist->pinned_tracks_cutoff =
    track->pos + 1;

  /* add master channel to mixer and tracklist */
  g_message ("adding master track...");
  track =
    track_new (
      TRACK_TYPE_MASTER, TRACKLIST->num_tracks,
      _("Master"), F_WITHOUT_LANE);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  self->tracklist->master_track = track;
  tracklist_selections_add_track (
    self->tracklist_selections, track, 0);
  self->last_selection = SELECTION_TYPE_TRACKLIST;

  /* pre-setup engine */
  if (with_engine)
    {
      engine_pre_setup (self->audio_engine);
    }

  engine_setup (self->audio_engine);

  if (with_engine)
    {
      tracklist_expose_ports_to_backend (
        self->tracklist);
    }

  engine_update_frames_per_tick (
    AUDIO_ENGINE, beats_per_bar,
    tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate, true, true);

  /* create untitled project */
  create_and_set_dir_and_title (self, prj_dir);

  if (have_ui)
    {
      g_message ("recreating main window...");
      recreate_main_window ();

      if (mww)
        {
          g_message ("destroying prev window...");
          destroy_prev_main_window (mww);
        }
    }

  self->loaded = true;

  snap_grid_init (
    self->snap_grid_timeline,
    SNAP_GRID_TYPE_TIMELINE,
    NOTE_LENGTH_BAR);
  quantize_options_init (
    self->quantize_opts_timeline,
    NOTE_LENGTH_1_1);
  snap_grid_init (
    self->snap_grid_editor,
    SNAP_GRID_TYPE_EDITOR,
    NOTE_LENGTH_1_8);
  quantize_options_init (
    self->quantize_opts_editor,
    NOTE_LENGTH_1_8);
  clip_editor_init (self->clip_editor);
  timeline_init (self->timeline);
  snap_grid_update_snap_points_default (
    self->snap_grid_timeline);
  snap_grid_update_snap_points_default (
    self->snap_grid_editor);
  quantize_options_update_quantize_points (
    self->quantize_opts_timeline);
  quantize_options_update_quantize_points (
    self->quantize_opts_editor);

  if (have_ui)
    {
      g_message ("setting up main window...");
      g_warn_if_fail (
        MAIN_WINDOW->center_dock->main_notebook->
          timeline_panel->tracklist);
      setup_main_window (self);
    }

  if (with_engine)
    {
      /* recalculate the routing graph */
      router_recalc_graph (ROUTER, F_NOT_SOFT);

      engine_set_run (self->audio_engine, true);
    }

  g_message ("done");

  return self;
}

/**
 * Returns the YAML representation of the saved
 * project file.
 *
 * To be free'd with free().
 *
 * @param backup Whether to use the project file
 *   from the most recent backup.
 */
char *
project_get_existing_yaml (
  Project * self,
  bool      backup)
{
  /* get file contents */
  char * project_file_path =
    project_get_path (
      self, PROJECT_PATH_PROJECT_FILE, backup);
  g_return_val_if_fail (project_file_path, NULL);
  g_message (
    "%s: getting YAML for project file %s",
    __func__, project_file_path);

  char * compressed_pj;
  gsize compressed_pj_size;
  GError *err = NULL;
  g_file_get_contents (
    project_file_path, &compressed_pj,
    &compressed_pj_size, &err);
  if (err != NULL)
    {
      /* Report error to user, and free error */
      char str[1200];
      sprintf (
        str, _("Unable to read file: %s"),
        err->message);
      ui_show_error_message (MAIN_WINDOW, str);
      g_error_free (err);
      return NULL;
    }

  /* decompress */
  g_message (
    "%s: decompressing project...", __func__);
  char * yaml = NULL;
  size_t yaml_size;
  err = NULL;
  bool ret =
    project_decompress (
      &yaml, &yaml_size,
      PROJECT_DECOMPRESS_DATA,
      compressed_pj, compressed_pj_size,
      PROJECT_DECOMPRESS_DATA, &err);
  g_free (compressed_pj);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to decompress project file"));
      return NULL;
    }

  /* make string null-terminated */
  yaml =
    g_realloc (
      yaml, yaml_size + sizeof (char));
  yaml[yaml_size] = '\0';

  return yaml;
}

/**
 * @param filename The filename to open. This will
 *   be the template in the case of template, or
 *   the actual project otherwise.
 * @param is_template Load the project as a
 *   template and create a new project from it.
 *
 * @return Non-zero if error.
 */
static int
load (
  const char * filename,
  const int is_template)
{
  g_warn_if_fail (filename);
  char * dir = io_get_dir (filename);

  if (!PROJECT)
    {
      /* create a temporary project struct */
      PROJECT = project_new (ZRYTHM);
    }

  set_dir (PROJECT, dir);

  /* if loading an actual project, check for
   * backups */
  if (!is_template)
    {
      if (PROJECT->backup_dir)
        g_free (PROJECT->backup_dir);
      PROJECT->backup_dir =
        get_newer_backup (PROJECT);
      if (PROJECT->backup_dir)
        {
          g_message (
            "newer backup found %s",
            PROJECT->backup_dir);

          GtkWidget * dialog =
            gtk_message_dialog_new (
              NULL,
              GTK_DIALOG_MODAL |
                GTK_DIALOG_DESTROY_WITH_PARENT,
              GTK_MESSAGE_INFO,
              GTK_BUTTONS_YES_NO,
              _("Newer backup found:\n  %s.\n"
                "Use the newer backup?"),
              PROJECT->backup_dir);
          gtk_window_set_title (
            GTK_WINDOW (dialog),
            _("Backup found"));
          gtk_window_set_icon_name (
            GTK_WINDOW (dialog), "zrythm");
          if (MAIN_WINDOW)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (MAIN_WINDOW), 0);
            }
          int res =
            z_gtk_dialog_run (
              GTK_DIALOG (dialog), true);
          switch (res)
            {
            case GTK_RESPONSE_YES:
              break;
            case GTK_RESPONSE_NO:
              g_free (PROJECT->backup_dir);
              PROJECT->backup_dir = NULL;
              break;
            default:
              break;
            }
          if (MAIN_WINDOW)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (MAIN_WINDOW), 1);
            }
        }
    }

  bool use_backup = PROJECT->backup_dir != NULL;
  PROJECT->loading_from_backup = use_backup;

  char * yaml =
    project_get_existing_yaml (PROJECT, use_backup);

  g_message ("project from yaml...");
  gint64 time_before = g_get_monotonic_time ();
  Project * self =
    (Project *)
    yaml_deserialize (yaml, &project_schema);
  gint64 time_after = g_get_monotonic_time ();
  g_message (
    "time to deserialize: %ldms",
    (long) (time_after - time_before) / 1000);
  free (yaml);
  if (!self)
    {
      g_warning ("Failed to load project");
      return -1;
    }
  self->backup_dir =
    g_strdup (PROJECT->backup_dir);

  char * version = zrythm_get_version (0);
  if (!string_is_equal (self->version, version))
    {
      char * str =
        g_strdup_printf (
          _("This project was created with a "
            "different version of %s (%s). "
            "It may not work correctly."),
          PROGRAM_NAME, self->version);
      ui_show_message_full (
        GTK_WINDOW (MAIN_WINDOW),
        GTK_MESSAGE_WARNING, "%s", str);
      g_free (str);
    }
  g_free (version);

  if (self == NULL)
    {
      ui_show_error_message (
        MAIN_WINDOW,
        _("Failed to load project. Please check the "
        "logs for more information."));
      RETURN_ERROR;
    }
  g_message ("Project successfully deserialized.");

  /* if template, also copy the pool and plugin
   * states */
  if (is_template)
    {
      char * prev_pool_dir =
        g_build_filename (
          dir, PROJECT_POOL_DIR, NULL);
      char * new_pool_dir =
        g_build_filename (
          ZRYTHM->create_project_path,
          PROJECT_POOL_DIR, NULL);
      char * prev_plugins_dir =
        g_build_filename (
          dir, PROJECT_PLUGINS_DIR, NULL);
      char * new_plugins_dir =
        g_build_filename (
          ZRYTHM->create_project_path,
          PROJECT_PLUGINS_DIR, NULL);
      io_copy_dir (
        new_pool_dir, prev_pool_dir,
        F_NO_FOLLOW_SYMLINKS, F_RECURSIVE);
      io_copy_dir (
        new_plugins_dir, prev_plugins_dir,
        F_NO_FOLLOW_SYMLINKS, F_RECURSIVE);
      g_free (prev_pool_dir);
      g_free (new_pool_dir);
      g_free (prev_plugins_dir);
      g_free (new_plugins_dir);

      g_free (dir);
      dir =
        g_strdup (ZRYTHM->create_project_path);
    }

  MainWindowWidget * mww = NULL;
  if (ZRYTHM_HAVE_UI)
    {
      g_message ("hiding prev window...");
      mww = hide_prev_main_window ();
    }

  if (PROJECT)
    {
      g_message (
        "%s: freeing previous project...",
        __func__);
      object_free_w_func_and_null (
        project_free, PROJECT);
    }

  g_message (
    "%s: initing loaded structures", __func__);
  PROJECT = self;

  /* re-update paths for the newly loaded project */
  set_dir (self, dir);

  /* set the tempo track */
  Tracklist * tracklist = self->tracklist;
  for (int i = 0; i < tracklist->num_tracks; i++)
    {
      Track * track = tracklist->tracks[i];

      if (track->type == TRACK_TYPE_TEMPO)
        {
          tracklist->tempo_track = track;
          break;
        }
    }

  char * filepath_noext =
    g_path_get_basename (dir);
  g_free (dir);

  self->title = filepath_noext;

  init_common (self);

  engine_init_loaded (self->audio_engine, self);
  engine_pre_setup (self->audio_engine);

  /* re-load clips because sample rate can change
   * during engine pre setup */
  audio_pool_init_loaded (
    self->audio_engine->pool);

  if (self->undo_manager)
    {
      undo_manager_init_loaded (self->undo_manager);
    }
  else
    {
      self->undo_manager = undo_manager_new ();
    }

  clip_editor_init_loaded (self->clip_editor);
  timeline_init_loaded (self->timeline);
  tracklist_init_loaded (
    self->tracklist, self, NULL);

  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  engine_update_frames_per_tick (
    AUDIO_ENGINE, beats_per_bar,
    tempo_track_get_current_bpm (P_TEMPO_TRACK),
    AUDIO_ENGINE->sample_rate, true, true);

  midi_mappings_init_loaded (
    self->midi_mappings);

  arranger_selections_init_loaded (
    (ArrangerSelections *)
    self->timeline_selections, true);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    self->midi_arranger_selections, true);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    self->chord_selections, true);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    self->automation_selections, true);
  arranger_selections_init_loaded (
    (ArrangerSelections *)
    self->audio_selections, true);

  tracklist_selections_init_loaded (
    self->tracklist_selections);

  snap_grid_update_snap_points_default (
    self->snap_grid_timeline);
  snap_grid_update_snap_points_default (
    self->snap_grid_editor);
  quantize_options_update_quantize_points (
    self->quantize_opts_timeline);
  quantize_options_update_quantize_points (
    self->quantize_opts_editor);

  region_link_group_manager_init_loaded (
    self->region_link_group_manager);
  port_connections_manager_init_loaded (
    self->port_connections_manager);

  if (ZRYTHM_HAVE_UI)
    {
      g_message ("recreating main window...");
      recreate_main_window ();

      if (mww)
        {
          g_message ("destroying prev window...");
          destroy_prev_main_window (mww);
        }
    }

  /* sanity check */
  project_validate (self);

  engine_setup (self->audio_engine);

  /* init ports */
  GPtrArray * ports = g_ptr_array_new ();
  port_get_all (ports);
  g_message ("Initializing loaded Ports...");
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = g_ptr_array_index (ports, i);
      if (port_is_exposed_to_backend (port))
        {
          port_set_expose_to_backend (port, true);
        }
    }
  object_free_w_func_and_null (
    g_ptr_array_unref, ports);

  self->loaded = true;
  self->loading_from_backup = false;

  g_message ("project loaded");

  /* recalculate the routing graph */
  router_recalc_graph (ROUTER, F_NOT_SOFT);

  g_message ("setting up main window...");
  setup_main_window (self);

  engine_set_run (self->audio_engine, true);

  return 0;
}

/**
 * If project has a filename set, it loads that.
 * Otherwise it loads the default project.
 *
 * @param filename The filename to open. This will
 *   be the template in the case of template, or
 *   the actual project otherwise.
 * @param is_template Load the project as a
 *   template and create a new project from it.
 *
 * @return 0 if successful, non-zero otherwise.
 */
int
project_load (
  const char * filename,
  const bool   is_template)
{
  g_message (
    "%s: filename: %s, is template: %d",
    __func__, filename, is_template);

  if (filename)
    {
      int ret = load (filename, is_template);
      if (ret)
        {
          ui_show_error_message (
            NULL,
            _("Failed to load project. Will create "
            "a new one instead."));

          CreateProjectDialogWidget * dialog =
            create_project_dialog_widget_new ();

          ret =
            z_gtk_dialog_run (
              GTK_DIALOG (dialog), true);

          if (ret != GTK_RESPONSE_OK)
            return -1;

          g_message (
            "%s: creating project %s",
            __func__, ZRYTHM->create_project_path);
          project_create_default (
            PROJECT, ZRYTHM->create_project_path,
            false, true);
        } /* endif failed to load project */
    }
  /* else if no filename given */
  else
    {
      project_create_default (
        PROJECT, ZRYTHM->create_project_path,
        false, true);
    }

  if (is_template || !filename)
    {
      project_save (
        PROJECT, PROJECT->dir, 0, 0, F_NO_ASYNC);
    }

  engine_activate (AUDIO_ENGINE, true);

  /* connect channel inputs to hardware. has to
   * be done after engine activation */
  Channel *ch;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      ch = TRACKLIST->tracks[i]->channel;
      if (!ch)
        continue;

      channel_reconnect_ext_input_ports (ch);
    }

  /* pause engine and reconnect graph */
  EngineState state;
  engine_wait_for_pause (
    AUDIO_ENGINE, &state, true);
  router_recalc_graph (ROUTER, F_NOT_SOFT);
  engine_resume (AUDIO_ENGINE, &state);

  return 0;
}

/**
 * Autosave callback.
 *
 * This will keep getting called at regular short
 * intervals, and if enough time has passed and
 * it's okay to save it will autosave, otherwise it
 * will wait until the next interval and check
 * again.
 */
int
project_autosave_cb (
  void * data)
{
  if (!PROJECT || !PROJECT->loaded
      || !PROJECT->dir || !PROJECT->datetime_str)
    return G_SOURCE_CONTINUE;

  unsigned int autosave_interval_mins =
    g_settings_get_uint (
      S_P_PROJECTS_GENERAL, "autosave-interval");

  /* return if autosave disabled */
  if (autosave_interval_mins <= 0)
    return G_SOURCE_CONTINUE;

  StereoPorts * out_ports;
  gint64 cur_time = g_get_monotonic_time ();
  gint64 microsec_to_autosave =
    (gint64)
    autosave_interval_mins * 60 * 1000000 -
      /* subtract 4 seconds because the time
       * this gets called is not exact */
      4 * 1000000;

  /* skip if semaphore busy */
  if (!zix_sem_try_wait (&PROJECT->save_sem))
    {
      g_message (
        "can't acquire project lock - skipping "
        "autosave");
      return G_SOURCE_CONTINUE;
    }

  /* skip if bad time to save or rolling */
  if (cur_time - PROJECT->last_autosave_time <
        microsec_to_autosave ||
      TRANSPORT_IS_ROLLING)
    {
      goto post_save_sem_and_continue;
    }

  /* skip if sound is playing */
  out_ports =
    P_MASTER_TRACK->channel->stereo_out;
  if (out_ports->l->peak >= 0.0001f ||
      out_ports->r->peak >= 0.0001f)
    {
      g_debug (
        "sound is playing, skipping autosave");
      goto post_save_sem_and_continue;
    }

  /* skip if currently performing action */
  if (arranger_widget_any_doing_action ())
    {
      g_debug (
        "in the middle of an action, skipping "
        "autosave");
      goto post_save_sem_and_continue;
    }

  /* ok to save */
  project_save (
    PROJECT, PROJECT->dir, 1, 1,
    F_ASYNC);
  PROJECT->last_autosave_time = cur_time;

post_save_sem_and_continue:
  zix_sem_post (&PROJECT->save_sem);

  return G_SOURCE_CONTINUE;
}

/**
 * Returns the requested project path as a newly
 * allocated string.
 *
 * @param backup Whether to get the path for the
 *   current backup instead of the main project.
 */
char *
project_get_path (
  Project *     self,
  ProjectPath   path,
  bool          backup)
{
  g_return_val_if_fail (self->dir, NULL);
  char * dir =
    backup ? self->backup_dir : self->dir;
  switch (path)
    {
    case PROJECT_PATH_BACKUPS:
      return
        g_build_filename (
          dir, PROJECT_BACKUPS_DIR, NULL);
    case PROJECT_PATH_EXPORTS:
      return
        g_build_filename (
          dir, PROJECT_EXPORTS_DIR, NULL);
    case PROJECT_PATH_EXPORTS_STEMS:
      return
        g_build_filename (
          dir, PROJECT_EXPORTS_DIR,
          PROJECT_STEMS_DIR, NULL);
    case PROJECT_PATH_PLUGINS:
      return
        g_build_filename (
          dir, PROJECT_PLUGINS_DIR, NULL);
    case PROJECT_PATH_PLUGIN_STATES:
      {
        char * plugins_dir =
          project_get_path (
            self, PROJECT_PATH_PLUGINS, backup);
        char * ret =
          g_build_filename (
            plugins_dir, PROJECT_PLUGIN_STATES_DIR,
            NULL);
        g_free (plugins_dir);
        return ret;
      }
      break;
    case PROJECT_PATH_PLUGIN_EXT_COPIES:
      {
        char * plugins_dir =
          project_get_path (
            self, PROJECT_PATH_PLUGINS, backup);
        char * ret =
          g_build_filename (
            plugins_dir,
            PROJECT_PLUGIN_EXT_COPIES_DIR,
            NULL);
        g_free (plugins_dir);
        return ret;
      }
      break;
    case PROJECT_PATH_PLUGIN_EXT_LINKS:
      {
        char * plugins_dir =
          project_get_path (
            self, PROJECT_PATH_PLUGINS, backup);
        char * ret =
          g_build_filename (
            plugins_dir,
            PROJECT_PLUGIN_EXT_LINKS_DIR,
            NULL);
        g_free (plugins_dir);
        return ret;
      }
      break;
    case PROJECT_PATH_POOL:
      return
        g_build_filename (
          dir, PROJECT_POOL_DIR, NULL);
    case PROJECT_PATH_PROJECT_FILE:
      return
        g_build_filename (
          dir, PROJECT_FILE, NULL);
      break;
    default:
      g_return_val_if_reached (NULL);
    }
  g_return_val_if_reached (NULL);
}

/**
 * Creates an empty project object.
 */
Project *
project_new (
  Zrythm * _zrythm)
{
  g_message ("%s: Creating...", __func__);

  Project * self = object_new (Project);
  self->schema_version =
    PROJECT_SCHEMA_VERSION;

  if (_zrythm)
    {
      _zrythm->project = self;
    }

  self->version = zrythm_get_version (0);
  self->clip_editor = clip_editor_new ();
  self->timeline = timeline_new ();
  self->snap_grid_timeline = snap_grid_new ();
  self->snap_grid_editor = snap_grid_new ();
  self->quantize_opts_timeline =
    quantize_options_new ();
  self->quantize_opts_editor =
    quantize_options_new ();
  self->tracklist_selections =
    tracklist_selections_new (true);
  self->region_link_group_manager =
    region_link_group_manager_new ();
  self->port_connections_manager =
    port_connections_manager_new ();
  self->midi_mappings = midi_mappings_new ();

  init_common (self);

  g_message ("%s: done", __func__);

  return self;
}

static void
project_save_data_free (
  ProjectSaveData * self)
{
  g_free_and_null (self->project_file_path);
  object_free_w_func_and_null (
    project_free, self->project);

  object_zero_and_free (self);
}

/**
 * Thread that does the serialization and saving.
 */
static void *
serialize_project_thread (
  ProjectSaveData * data)
{
  char * compressed_yaml;
  size_t compressed_size;
  bool ret;

  /* generate yaml */
  g_message ("serializing project to yaml...");
  GError *err = NULL;
  gint64 time_before = g_get_monotonic_time ();
  char * yaml =
    yaml_serialize (
      data->project, &project_schema);
  gint64 time_after = g_get_monotonic_time ();
  g_message (
    "time to serialize: %ldms",
    (long) (time_after - time_before) / 1000);
  if (!yaml)
    {
      g_critical ("Failed to serialize project");
      data->has_error = true;
      goto serialize_end;
    }

  /* compress */
  err = NULL;
  ret =
    project_compress (
      &compressed_yaml, &compressed_size,
      PROJECT_COMPRESS_DATA,
      yaml, strlen (yaml) * sizeof (char),
      PROJECT_COMPRESS_DATA, &err);
  g_free (yaml);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to compress project file"));
      data->has_error = true;
      goto serialize_end;
    }

  /* set file contents */
  g_message (
    "%s: saving project file at %s...",
    __func__, data->project_file_path);
  g_file_set_contents (
    data->project_file_path, compressed_yaml,
    (gssize) compressed_size, &err);
  free (compressed_yaml);
  if (err != NULL)
    {
      g_critical (
        "%s: Unable to write project file: %s",
        __func__, err->message);
      g_error_free (err);
      data->has_error = true;
    }

  g_message (
    "%s: successfully saved project", __func__);

serialize_end:
  zix_sem_post (&UNDO_MANAGER->action_sem);
  data->finished = true;
  return NULL;
}

/**
 * Idle func to check if the project has finished
 * saving and show a notification.
 */
static int
project_idle_saved_cb (
  ProjectSaveData * data)
{
  if (!data->finished)
    {
      return G_SOURCE_CONTINUE;
    }

  if (data->is_backup)
    {
      g_message (_("Backup saved."));
    }
  else
    {
      if (!ZRYTHM_TESTING)
        {
          zrythm_add_to_recent_projects (
            ZRYTHM, data->project_file_path);
        }
      if (data->show_notification)
        ui_show_notification (_("Project saved."));
    }

  if (ZRYTHM_HAVE_UI && PROJECT->loaded &&
      MAIN_WINDOW)
    {
      EVENTS_PUSH (ET_PROJECT_SAVED, PROJECT);
    }

  data->progress_info.progress = 1.0;

  return G_SOURCE_REMOVE;
}

/**
 * Saves the project to a project file in the
 * given dir.
 *
 * @param is_backup 1 if this is a backup. Backups
 *   will be saved as <original filename>.bak<num>.
 * @param show_notification Show a notification
 *   in the UI that the project was saved.
 * @param async Save asynchronously in another
 *   thread.
 *
 * @return Non-zero if error.
 */
int
project_save (
  Project *    self,
  const char * _dir,
  const bool   is_backup,
  const bool   show_notification,
  const bool   async)
{
  /* pause engine */
  EngineState state;
  bool engine_paused = false;
  if (AUDIO_ENGINE->activated)
    {
      engine_wait_for_pause (
        AUDIO_ENGINE, &state, F_NO_FORCE);
      engine_paused = true;
    }

  if (async)
    {
      zix_sem_wait (&UNDO_MANAGER->action_sem);
    }

  project_validate (self);

  char * dir = g_strdup (_dir);

  /* set the dir and create it if it doesn't
   * exist */
  set_dir (self, dir);
  io_mkdir (PROJECT->dir);
  char * tmp;

  /* set the title */
  char * basename =
    g_path_get_basename (dir);
  set_title (self, basename);
  g_free (basename);
  g_free (dir);

  /* save current datetime */
  set_datetime_str (self);

  /* set the project version */
  g_free_and_null (self->version);
  self->version = zrythm_get_version (false);

  /* if backup, get next available backup dir */
  if (is_backup)
    {
      set_and_create_next_available_backup_dir (
        self);
    }

#define MK_PROJECT_DIR(_path) \
  tmp = \
    project_get_path ( \
      self, PROJECT_PATH_##_path, is_backup); \
  g_return_val_if_fail (tmp, -1); \
  io_mkdir (tmp); \
  g_free_and_null (tmp)

  MK_PROJECT_DIR (EXPORTS);
  MK_PROJECT_DIR (EXPORTS_STEMS);
  MK_PROJECT_DIR (POOL);
  MK_PROJECT_DIR (PLUGIN_STATES);
  MK_PROJECT_DIR (PLUGIN_EXT_COPIES);
  MK_PROJECT_DIR (PLUGIN_EXT_LINKS);

#if 0
  /* write plugin states, prepare channels for
   * serialization, */
  Track * track;
  Channel * ch;
  Plugin * pl;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (track->type == TRACK_TYPE_CHORD)
        continue;

      ch = track->channel;
      if (!ch)
        continue;

      for (j = 0; j < STRIP_SIZE * 2 + 1; j++)
        {
          if (j < STRIP_SIZE)
            pl = ch->midi_fx[j];
          else if (j == STRIP_SIZE)
            pl = ch->instrument;
          else
            pl = ch->inserts[j - (STRIP_SIZE + 1)];

          if (!pl)
            continue;

          if (!pl->instantiated)
            {
              g_warning (
                "skipping uninstantiated plugin "
                "%s...",
                pl->setting->descr->name);
              continue;
            }

          /* save state */
#ifdef HAVE_CARLA
          if (pl->setting->open_with_carla)
            {
              carla_native_plugin_save_state (
                pl->carla, is_backup, NULL);
            }
          else
            {
#endif
              switch (pl->setting->descr->protocol)
                {
                case PROT_LV2:
                  {
                    LilvState * tmp_state =
                      lv2_state_save_to_file (
                        pl->lv2, is_backup);
                    lilv_state_free (tmp_state);
                  }
                  break;
                default:
                  g_warn_if_reached ();
                  break;
                }
#ifdef HAVE_CARLA
            }
#endif
        }
    }
#endif

  /* write the pool */
  audio_pool_remove_unused (AUDIO_POOL, is_backup);
  audio_pool_write_to_disk (AUDIO_POOL, is_backup);

  /* save UI positions */
  if (ZRYTHM_HAVE_UI)
    {
      if (MW_TIMELINE)
        {
          GtkScrolledWindow * scroll =
            arranger_widget_get_scrolled_window (
              MW_TIMELINE);
          GtkAdjustment * adj =
            gtk_scrolled_window_get_hadjustment (
              scroll);
          PRJ_TIMELINE->editor_settings.
            scroll_start_x =
              (int) gtk_adjustment_get_value (adj);
          adj =
            gtk_scrolled_window_get_vadjustment (
              scroll);
          PRJ_TIMELINE->editor_settings.
            scroll_start_y =
              (int) gtk_adjustment_get_value (adj);
        }
      if (MW_MIDI_ARRANGER)
        {
          GtkScrolledWindow * scroll =
            arranger_widget_get_scrolled_window (
              MW_MIDI_ARRANGER);
          GtkAdjustment * adj =
            gtk_scrolled_window_get_hadjustment (
              scroll);
          PIANO_ROLL->editor_settings.
            scroll_start_x =
              (int) gtk_adjustment_get_value (adj);
          adj =
            gtk_scrolled_window_get_vadjustment (
              scroll);
          PIANO_ROLL->editor_settings.
            scroll_start_y =
              (int) gtk_adjustment_get_value (adj);
        }
      if (MW_AUTOMATION_ARRANGER)
        {
          GtkScrolledWindow * scroll =
            arranger_widget_get_scrolled_window (
              MW_AUTOMATION_ARRANGER);
          GtkAdjustment * adj =
            gtk_scrolled_window_get_hadjustment (
              scroll);
          AUTOMATION_EDITOR->editor_settings.
            scroll_start_x =
              (int) gtk_adjustment_get_value (adj);
          adj =
            gtk_scrolled_window_get_vadjustment (
              scroll);
          AUTOMATION_EDITOR->editor_settings.
            scroll_start_y =
              (int) gtk_adjustment_get_value (adj);
        }
      if (MW_AUDIO_ARRANGER)
        {
          GtkScrolledWindow * scroll =
            arranger_widget_get_scrolled_window (
              MW_AUDIO_ARRANGER);
          GtkAdjustment * adj =
            gtk_scrolled_window_get_hadjustment (
              scroll);
          AUDIO_CLIP_EDITOR->editor_settings.
            scroll_start_x =
              (int) gtk_adjustment_get_value (adj);
          adj =
            gtk_scrolled_window_get_vadjustment (
              scroll);
          AUDIO_CLIP_EDITOR->editor_settings.
            scroll_start_y =
              (int) gtk_adjustment_get_value (adj);
        }
      if (MW_CHORD_ARRANGER)
        {
          GtkScrolledWindow * scroll =
            arranger_widget_get_scrolled_window (
              MW_CHORD_ARRANGER);
          GtkAdjustment * adj =
            gtk_scrolled_window_get_hadjustment (
              scroll);
          CHORD_EDITOR->editor_settings.
            scroll_start_x =
              (int) gtk_adjustment_get_value (adj);
          adj =
            gtk_scrolled_window_get_vadjustment (
              scroll);
          CHORD_EDITOR->editor_settings.
            scroll_start_y =
              (int) gtk_adjustment_get_value (adj);
        }
    }

  ProjectSaveData * data =
    object_new (ProjectSaveData);
  data->project_file_path =
    project_get_path (
      self, PROJECT_PATH_PROJECT_FILE, is_backup);
  data->show_notification = show_notification;
  data->is_backup = is_backup;
  data->project = project_clone (PROJECT);
  data->project->tracklist_selections->free_tracks =
    true;

  if (async)
    {
      g_thread_new (
        "serialize_project_thread",
        (GThreadFunc)
        serialize_project_thread, data);
      g_idle_add (
        (GSourceFunc) project_idle_saved_cb,
        data);

      if (ZRYTHM_HAVE_UI)
        {
          /* show progress while saving */
          ProjectProgressDialogWidget * dialog =
            project_progress_dialog_widget_new (
              data);
          gtk_window_set_transient_for (
            GTK_WINDOW (dialog),
            GTK_WINDOW (MAIN_WINDOW));
          gtk_window_set_modal (
            GTK_WINDOW (dialog), true);
          z_gtk_dialog_run (
            GTK_DIALOG (dialog), true);
        }
      else
        {
          while (data->progress_info.progress < 1.0)
            {
              g_usleep (1000);
            }
        }
    }
  else /* else if no async */
    {
      /* call synchronously */
      serialize_project_thread (data);
      project_idle_saved_cb (data);
    }

  object_free_w_func_and_null (
    project_save_data_free, data);

  if (ZRYTHM_TESTING)
    tracklist_validate (self->tracklist);

  if (engine_paused)
    {
      engine_resume (AUDIO_ENGINE, &state);
    }

  RETURN_OK;
}

/**
 * Deep-clones the given project.
 *
 * To be used during save on the main thread.
 */
Project *
project_clone (
  const Project * src)
{
  g_message ("cloning project...");

  Project * self = object_new (Project);
  self->schema_version =
    PROJECT_SCHEMA_VERSION;

  self->title = g_strdup (src->title);
  self->datetime_str = g_strdup (src->datetime_str);
  self->version = g_strdup (src->version);
  self->tracklist =
    tracklist_clone (src->tracklist);
  self->clip_editor =
    clip_editor_clone (src->clip_editor);
  self->timeline =
    timeline_clone (src->timeline);
  self->snap_grid_timeline =
    snap_grid_clone (src->snap_grid_timeline);
  self->snap_grid_editor =
    snap_grid_clone (src->snap_grid_editor);
  self->quantize_opts_timeline =
    quantize_options_clone (
      src->quantize_opts_timeline);
  self->quantize_opts_editor =
    quantize_options_clone (
      src->quantize_opts_editor);
  self->audio_engine =
    engine_clone (src->audio_engine);
  self->mixer_selections =
    mixer_selections_clone (
      src->mixer_selections, F_PROJECT);
  self->timeline_selections =
    (TimelineSelections *)
    arranger_selections_clone (
      (ArrangerSelections *)
      src->timeline_selections);
  self->midi_arranger_selections =
    (MidiArrangerSelections *)
    arranger_selections_clone (
      (ArrangerSelections *)
      src->midi_arranger_selections);
  self->chord_selections =
    (ChordSelections *)
    arranger_selections_clone (
      (ArrangerSelections *)
      src->chord_selections);
  self->automation_selections =
    (AutomationSelections *)
    arranger_selections_clone (
      (ArrangerSelections *)
      src->automation_selections);
  self->audio_selections =
    (AudioSelections *)
    arranger_selections_clone (
      (ArrangerSelections *)
      src->audio_selections);
  GError * err = NULL;
  self->tracklist_selections =
    tracklist_selections_clone (
      src->tracklist_selections, &err);
  if (!self->tracklist_selections)
    {
      g_critical (
        "Failed to clone track selections: %s",
        err->message);
      return NULL;
    }
  self->tracklist_selections->is_project = true;
  self->region_link_group_manager =
    region_link_group_manager_clone (
      src->region_link_group_manager);
  self->port_connections_manager =
    port_connections_manager_clone (
      src->port_connections_manager);
  self->midi_mappings =
    midi_mappings_clone (src->midi_mappings);
  self->undo_manager =
    undo_manager_clone (src->undo_manager);

  g_message ("finished cloning project");

  return self;
}

/**
 * Frees the selections in the project.
 */
static void
free_arranger_selections (
  Project * self)
{
  object_free_w_func_and_null_cast (
    arranger_selections_free,
    ArrangerSelections *,
    self->automation_selections);
  object_free_w_func_and_null_cast (
    arranger_selections_free,
    ArrangerSelections *,
    self->audio_selections);
  object_free_w_func_and_null_cast (
    arranger_selections_free,
    ArrangerSelections *,
    self->chord_selections);
  object_free_w_func_and_null_cast (
    arranger_selections_free,
    ArrangerSelections *,
    self->timeline_selections);
  object_free_w_func_and_null_cast (
    arranger_selections_free,
    ArrangerSelections *,
    self->midi_arranger_selections);
}

/**
 * Tears down the project.
 */
void
project_free (Project * self)
{
  g_message ("%s: tearing down...", __func__);

  self->loaded = false;

  g_free_and_null (self->title);

  if (self->audio_engine &&
      self->audio_engine->activated)
    {
      engine_activate (self->audio_engine, false);
    }
  /* remove region from clip editor to avoid
   * lookups for it when removing regions from
   * track */
  self->clip_editor->has_region = false;

  object_free_w_func_and_null (
    undo_manager_free, self->undo_manager);

  /* must be free'd before tracklist selections,
   * mixer selections, engine, and port connection
   * manager */
  object_free_w_func_and_null (
    tracklist_free, self->tracklist);

  object_free_w_func_and_null (
    midi_mappings_free, self->midi_mappings);
  object_free_w_func_and_null (
    clip_editor_free, self->clip_editor);
  object_free_w_func_and_null (
    timeline_free, self->timeline);
  object_free_w_func_and_null (
    snap_grid_free, self->snap_grid_timeline);
  object_free_w_func_and_null (
    snap_grid_free, self->snap_grid_editor);
  object_free_w_func_and_null (
    quantize_options_free,
    self->quantize_opts_timeline);
  object_free_w_func_and_null (
    quantize_options_free,
    self->quantize_opts_editor);
  object_free_w_func_and_null (
    region_link_group_manager_free,
    self->region_link_group_manager);

  object_free_w_func_and_null (
    tracklist_selections_free,
    self->tracklist_selections);

  free_arranger_selections (self);

  object_free_w_func_and_null (
    engine_free, self->audio_engine);

  /* must be free'd after engine */
  object_free_w_func_and_null (
    port_connections_manager_free,
    self->port_connections_manager);

  /* must be free'd after port connections
   * manager */
  object_free_w_func_and_null (
    mixer_selections_free,
    self->mixer_selections);

  zix_sem_destroy (&self->save_sem);

  object_zero_and_free (self);

  g_message ("%s: free'd project", __func__);
}
