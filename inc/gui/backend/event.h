/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * UI event.
 */
#ifndef __GUI_BACKEND_EVENT_H__
#define __GUI_BACKEND_EVENT_H__

/**
 * @addtogroup events
 *
 * @{
 */

typedef enum EventType
{
  /* arranger objects */
  ET_ARRANGER_OBJECT_CREATED,
  ET_ARRANGER_OBJECT_REMOVED,
  ET_ARRANGER_OBJECT_CHANGED,

  /* arranger_selections */
  ET_ARRANGER_SELECTIONS_CREATED,
  ET_ARRANGER_SELECTIONS_CHANGED,
  ET_ARRANGER_SELECTIONS_REMOVED,
  ET_ARRANGER_SELECTIONS_MOVED,
  ET_ARRANGER_SELECTIONS_QUANTIZED,

  /** Redraws all arrangers. */
  ET_ARRANGER_SELECTIONS_CHANGED_REDRAW_EVERYTHING,

  /** To be used after an action is finished to
   * redraw everything. */
  ET_ARRANGER_SELECTIONS_ACTION_FINISHED,

  /** also for channels */
  ET_TRACK_STATE_CHANGED,
  /** works for all rulers */
  ET_RULER_STATE_CHANGED,
  ET_AUTOMATION_TRACK_ADDED,
  ET_AUTOMATION_TRACK_REMOVED,
  ET_TIME_SIGNATURE_CHANGED,
  ET_TRACK_ADDED,
  ET_TRACK_CHANGED,
  ET_TRACK_COLOR_CHANGED,
  ET_TRACK_NAME_CHANGED,
  ET_CHANNEL_SEND_CHANGED,

  ET_RULER_DISPLAY_TYPE_CHANGED,

  ET_TRANSPORT_TOTAL_BARS_CHANGED,

  ET_TRACK_AUTOMATION_VISIBILITY_CHANGED,
  ET_AUTOMATION_TRACK_CHANGED,

  /**
   * ZRegion (clip) to show in the piano roll
   * changed.
   *
   * Eg., a region in the timeline was clicked.
   */
  ET_CLIP_EDITOR_REGION_CHANGED,

  /**
   * Clip marker (clip start, loop start, loop end)
   * position changed.
   */
  ET_CLIP_MARKER_POS_CHANGED,

  ET_UNDO_REDO_ACTION_DONE,
  ET_RANGE_SELECTION_CHANGED,
  ET_TIMELINE_LOOP_MARKER_POS_CHANGED,
  ET_TIMELINE_PUNCH_MARKER_POS_CHANGED,
  ET_TIMELINE_SONG_MARKER_POS_CHANGED,
  ET_RULER_SIZE_CHANGED,

  ET_LOOP_TOGGLED,

  /** Selected tool (mode) changed. */
  ET_TOOL_CHANGED,

  /**
   * Zoom level or view area changed.
   *
   * Arg: RulerWidget pointer.
   */
  ET_RULER_VIEWPORT_CHANGED,

  ET_PLUGIN_ADDED,
  ET_PLUGINS_ADDED,
  ET_PLUGINS_REMOVED,
  ET_PLUGIN_DELETED,
  ET_PLUGIN_CRASHED,
  ET_PLAYHEAD_POS_CHANGED,
  ET_PLAYHEAD_POS_CHANGED_MANUALLY,
  ET_AUTOMATION_VALUE_CHANGED,

  ET_TRACKLIST_SELECTIONS_CHANGED,

  /** Plugin visibility parameter changed, should
   * close/open UI. */
  ET_PLUGIN_VISIBILITY_CHANGED,

  /** Plugin UI opened or closed, should redraw */
  ET_PLUGIN_WINDOW_VISIBILITY_CHANGED,

  ET_PLUGIN_STATE_CHANGED,

  ET_TRACKS_ADDED,
  ET_TRACKS_REMOVED,
  ET_TRACKS_MOVED,
  ET_CHANNEL_REMOVED,
  ET_REFRESH_ARRANGER,
  ET_MIXER_SELECTIONS_CHANGED,
  ET_CHANNEL_OUTPUT_CHANGED,
  ET_CHANNEL_SLOTS_CHANGED,

  ET_MODULATOR_ADDED,
  ET_RT_SELECTIONS_CHANGED,
  ET_PINNED_TRACKLIST_SIZE_CHANGED,

  /**
   * Drum mode changed.
   *
   * Arg: Track pointer.
   */
  ET_DRUM_MODE_CHANGED,

  /**
   * Track lanes visibility changed.
   *
   * Arg: (null).
   */
  ET_TRACK_LANES_VISIBILITY_CHANGED,

  /**
   * Track lane added.
   *
   * Arg: (null).
   */
  ET_TRACK_LANE_ADDED,

  /**
   * Track lane removed.
   *
   * Arg: (null).
   */
  ET_TRACK_LANE_REMOVED,

  ET_PIANO_ROLL_HIGHLIGHTING_CHANGED,
  ET_PIANO_ROLL_MIDI_MODIFIER_CHANGED,
  ET_AUTOMATION_TRACKLIST_AT_REMOVED,
  ET_ARRANGER_SELECTIONS_IN_TRANSIT,
  ET_CHORD_KEY_CHANGED,
  ET_JACK_TRANSPORT_TYPE_CHANGED,
  ET_TRACK_VISIBILITY_CHANGED,
  ET_SELECTING_IN_ARRANGER,
  ET_TRACKS_RESIZED,
  ET_CLIP_EDITOR_FIRST_TIME_REGION_SELECTED,
  ET_BPM_CHANGED,
  ET_CHANNEL_FADER_VAL_CHANGED,
  ET_PIANO_ROLL_KEY_HEIGHT_CHANGED,

  /**
   * Trial limit reached.
   *
   * Used to show a window to inform the user.
   */
  ET_TRIAL_LIMIT_REACHED,

  /** Sent after the main window finishes loading. */
  ET_MAIN_WINDOW_LOADED,

  /** Sent when a project is loaded. */
  ET_PROJECT_LOADED,

  /** Sent when a project is saved. */
  ET_PROJECT_SAVED,

  /** Sent when plugin latency changes, to update
   * the graph. */
  ET_PLUGIN_LATENCY_CHANGED,

  /** Arranger highlight rectangle changed. */
  ET_ARRANGER_HIGHLIGHT_CHANGED,

  ET_ENGINE_ACTIVATE_CHANGED,
  ET_ENGINE_BUFFER_SIZE_CHANGED,
  ET_ENGINE_SAMPLE_RATE_CHANGED,
  ET_MIDI_BINDINGS_CHANGED,
  ET_PORT_CONNECTION_CHANGED,
  ET_EDITOR_FUNCTION_APPLIED,

  ET_SPLASH_CLOSED,

  ET_AUTOMATION_VALUE_VISIBILITY_CHANGED,
  ET_PROJECT_SELECTION_TYPE_CHANGED,

  /** Range selection changed in audio editor. */
  ET_AUDIO_SELECTIONS_RANGE_CHANGED,

  ET_PLUGIN_COLLETIONS_CHANGED,

  ET_SNAP_GRID_OPTIONS_CHANGED,

  ET_TRANSPORT_RECORDING_ON_OFF_CHANGED,

  /**
   * Track freeze status changed.
   *
   * Param: Track.
   */
  ET_TRACK_FREEZE_CHANGED,

  /** Log has new unseen warnings or warnings have
   * been seen. */
  ET_LOG_WARNING_STATE_CHANGED,

  /**
   * Follow playhead/scroll edges state changed.
   */
  ET_PLAYHEAD_SCROLL_MODE_CHANGED,

  /**
   * Fader button (mute/solo/etc) changed.
   *
   * Param: pointer to Track.
   */
  ET_TRACK_FADER_BUTTON_CHANGED,

  /**
   * Plugin preset saved.
   *
   * Arg: Plugin.
   */
  ET_PLUGIN_PRESET_SAVED,

  /**
   * Plugin preset loaded.
   *
   * Arg: Plugin.
   */
  ET_PLUGIN_PRESET_LOADED,

  /**
   * Track folded status changed.
   *
   * Arg: track.
   */
  ET_TRACK_FOLD_CHANGED,

  /**
   * Channel MIDI FX revealed status changed in the
   * mixer.
   *
   * Arg: Pointer to Channel.
   */
  ET_MIXER_CHANNEL_MIDI_FX_EXPANDED_CHANGED,

  /**
   * Channel inserts revealed status changed in the
   * mixer.
   *
   * Arg: Pointer to Channel.
   */
  ET_MIXER_CHANNEL_INSERTS_EXPANDED_CHANGED,

  /**
   * Channel sends revealed status changed in the
   * mixer.
   *
   * Arg: Pointer to Channel.
   */
  ET_MIXER_CHANNEL_SENDS_EXPANDED_CHANGED,

  /**
   * Double click (or similar action) on region.
   *
   * Arg: None.
   */
  ET_REGION_ACTIVATED,

  /**
   * Piano roll key pressed or unpressed.
   *
   * Arg: None.
   */
  ET_PIANO_ROLL_KEY_ON_OFF,

  /**
   * Velocities have been ramped.
   *
   * Arg: None.
   */
  ET_VELOCITIES_RAMPED,

  /**
   * Audio region fade in/out changed.
   *
   * Arg: ZRegion pointer.
   */
  ET_AUDIO_REGION_FADE_IN_CHANGED,
  ET_AUDIO_REGION_FADE_OUT_CHANGED,

  /**
   * Audio region gain changed.
   *
   * Arg: ZRegion pointer.
   */
  ET_AUDIO_REGION_GAIN_CHANGED,

  /**
   * File browser bookmark added.
   *
   * Arg: None.
   */
  ET_FILE_BROWSER_BOOKMARK_ADDED,

  /**
   * File browser bookmark deleted.
   *
   * Arg: None.
   */
  ET_FILE_BROWSER_BOOKMARK_DELETED,
} EventType;

/**
 * A Zrythm event.
 */
typedef struct ZEvent
{
  /** Event type. */
  EventType    type;

  /** Argument. */
  void *       arg;

  /** Source file initialized from. */
  const char * file;

  /** Function initialized from. */
  const char * func;

  /** Line no initialized from. */
  int          lineno;

  /** Backtrace. */
  char *       backtrace;
} ZEvent;

ZEvent *
event_new (void);

void
event_free (ZEvent * self);

/**
 * @}
 */

#endif
