/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Actions.
 */

#ifndef __ACTIONS_ACTIONS_H__
#define __ACTIONS_ACTIONS_H__

#include <stdbool.h>

#include <gtk/gtk.h>

#define DECLARE_SIMPLE(x) \
  void x ( \
    GSimpleAction *action, \
    GVariant      *variant, \
    gpointer       user_data)

/**
 * @addtogroup actions
 *
 * @{
 */

void
actions_set_app_action_enabled (
  const char * action_name,
  const bool   enabled);

void
activate_about (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data);

/**
 * Show preferences window.
 */
void
activate_preferences (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

/**
 * Show preferences window.
 */
void
activate_log (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

/**
 * Show preferences window.
 */
void
activate_scripting_interface (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

/**
 * Activate audition mode.
 */
void
activate_audition_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

/**
 * Activate select mode.
 */
void
activate_select_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

/**
 * Activate edit mode.
 */
void
activate_edit_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

/**
 * Activate cut mode.
 */
void
activate_cut_mode (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

/**
 * Activate eraser mode.
 */
void
activate_eraser_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

/**
 * Activate ramp mode.
 */
void
activate_ramp_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

void
activate_quit (GSimpleAction *action,
               GVariant      *variant,
               gpointer       user_data);

void
activate_zoom_in (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_zoom_out (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

COLD
DECLARE_SIMPLE (activate_new);

COLD
DECLARE_SIMPLE (activate_minimize);

COLD
DECLARE_SIMPLE (activate_open);

COLD
void
activate_save (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

COLD
DECLARE_SIMPLE (activate_save_as);

COLD
DECLARE_SIMPLE (activate_export_as);

COLD
DECLARE_SIMPLE (activate_export_graph);

void
activate_properties (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

DECLARE_SIMPLE (activate_undo);
DECLARE_SIMPLE (activate_redo);
DECLARE_SIMPLE (activate_undo_n);
DECLARE_SIMPLE (activate_redo_n);
DECLARE_SIMPLE (activate_cut);
DECLARE_SIMPLE (activate_copy);
DECLARE_SIMPLE (activate_paste);
DECLARE_SIMPLE (activate_delete);
DECLARE_SIMPLE (activate_duplicate);
DECLARE_SIMPLE (activate_clear_selection);
DECLARE_SIMPLE (activate_select_all);

void
activate_toggle_left_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_toggle_right_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_toggle_bot_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

/**
 * Toggle timeline visibility.
 */
void
activate_toggle_top_panel (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
change_state_piano_roll_drum_mode (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
change_state_show_automation_values (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
activate_toggle_status_bar (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_fullscreen (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

COLD DECLARE_SIMPLE (activate_news);
COLD DECLARE_SIMPLE (activate_manual);
COLD DECLARE_SIMPLE (activate_chat);
COLD DECLARE_SIMPLE (activate_bugreport);
COLD DECLARE_SIMPLE (activate_donate);

void
activate_loop_selection (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_best_fit (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_original_size (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_snap_to_grid (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_snap_keep_offset (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

DECLARE_SIMPLE (activate_create_audio_track);
DECLARE_SIMPLE (activate_create_midi_track);
DECLARE_SIMPLE (activate_create_audio_bus_track);
DECLARE_SIMPLE (activate_create_midi_bus_track);
DECLARE_SIMPLE (activate_create_audio_group_track);
DECLARE_SIMPLE (activate_create_midi_group_track);
DECLARE_SIMPLE (activate_create_folder_track);

DECLARE_SIMPLE (activate_duplicate_selected_tracks);
DECLARE_SIMPLE (activate_delete_selected_tracks);
DECLARE_SIMPLE (activate_hide_selected_tracks);

DECLARE_SIMPLE (activate_pin_selected_tracks);
DECLARE_SIMPLE (activate_solo_selected_tracks);
DECLARE_SIMPLE (activate_unsolo_selected_tracks);
DECLARE_SIMPLE (activate_mute_selected_tracks);
DECLARE_SIMPLE (activate_unmute_selected_tracks);
DECLARE_SIMPLE (activate_listen_selected_tracks);
DECLARE_SIMPLE (activate_unlisten_selected_tracks);
DECLARE_SIMPLE (activate_enable_selected_tracks);
DECLARE_SIMPLE (activate_disable_selected_tracks);
DECLARE_SIMPLE (activate_change_track_color);
DECLARE_SIMPLE (activate_track_set_midi_channel);
DECLARE_SIMPLE (activate_quick_bounce_selected_tracks);
DECLARE_SIMPLE (activate_bounce_selected_tracks);
DECLARE_SIMPLE (
  activate_selected_tracks_direct_out_to);
DECLARE_SIMPLE (
  activate_selected_tracks_direct_out_new);
DECLARE_SIMPLE (
  activate_toggle_track_passthrough_input);

void
activate_snap_events (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_goto_prev_marker (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_goto_next_marker (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_play_pause (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
change_state_dim_output (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
change_state_loop (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
change_state_metronome (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
change_state_musical_mode (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
change_state_listen_notes (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
activate_quick_quantize (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_quantize_options (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

DECLARE_SIMPLE (activate_mute_selection);
DECLARE_SIMPLE (activate_merge_selection);

void
activate_set_timebase_master (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_set_transport_client (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_unlink_jack_transport (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

/**
 * All purpose menuitem callback for binding MIDI
 * CC to a port.
 *
 * An action will be performed if bound.
 */
void
activate_bind_midi_cc (
  GSimpleAction * action,
  GVariant      * _variant,
  gpointer        user_data);

void
activate_delete_cc_binding (
  GSimpleAction * simple_action,
  GVariant      * _variant,
  gpointer        user_data);

COLD DECLARE_SIMPLE (activate_show_file_browser);
DECLARE_SIMPLE (
  activate_toggle_timeline_event_viewer);
DECLARE_SIMPLE (
  activate_toggle_editor_event_viewer);
DECLARE_SIMPLE (activate_insert_silence);
DECLARE_SIMPLE (activate_remove_range);

DECLARE_SIMPLE (
  change_state_timeline_playhead_scroll_edges);
DECLARE_SIMPLE (
  change_state_timeline_playhead_follow);
DECLARE_SIMPLE (
  change_state_editor_playhead_scroll_edges);
DECLARE_SIMPLE (
  change_state_editor_playhead_follow);

/* Editor functions. */
DECLARE_SIMPLE (activate_editor_function);
DECLARE_SIMPLE (activate_editor_function_lv2);

COLD DECLARE_SIMPLE (
  activate_midi_editor_highlighting);

DECLARE_SIMPLE (
  activate_rename_track_or_region);

DECLARE_SIMPLE (activate_add_region);
DECLARE_SIMPLE (activate_go_to_start);
DECLARE_SIMPLE (activate_input_bpm);
DECLARE_SIMPLE (activate_tap_bpm);

DECLARE_SIMPLE (activate_nudge_selection);
DECLARE_SIMPLE (activate_detect_bpm);
DECLARE_SIMPLE (activate_timeline_function);
DECLARE_SIMPLE (activate_export_midi_region);
DECLARE_SIMPLE (activate_quick_bounce_selections);
DECLARE_SIMPLE (activate_bounce_selections);
DECLARE_SIMPLE (activate_set_curve_algorithm);
DECLARE_SIMPLE (
  activate_set_region_fade_in_algorithm_preset);
DECLARE_SIMPLE (
  activate_set_region_fade_out_algorithm_preset);
DECLARE_SIMPLE (activate_arranger_object_view_info);

/* port actions */
DECLARE_SIMPLE (activate_reset_stereo_balance);
DECLARE_SIMPLE (activate_reset_fader);
DECLARE_SIMPLE (activate_reset_control);
DECLARE_SIMPLE (activate_port_view_info);
DECLARE_SIMPLE (activate_port_connection_remove);

/* plugin actions */
DECLARE_SIMPLE (activate_plugin_toggle_enabled);
DECLARE_SIMPLE (activate_plugin_inspect);

/* panel file browser actions */
DECLARE_SIMPLE (
  activate_panel_file_browser_add_bookmark);
DECLARE_SIMPLE (
  activate_panel_file_browser_delete_bookmark);

/* plugin browser actions */
DECLARE_SIMPLE (
  activate_plugin_browser_add_to_project);
DECLARE_SIMPLE (
  activate_plugin_browser_add_to_project_carla);
DECLARE_SIMPLE (
  activate_plugin_browser_add_to_project_bridged_ui);
DECLARE_SIMPLE (
  activate_plugin_browser_add_to_project_bridged_full);
DECLARE_SIMPLE (
  change_state_plugin_browser_toggle_generic_ui);
DECLARE_SIMPLE (
  activate_plugin_browser_add_to_collection);
DECLARE_SIMPLE (
  activate_plugin_browser_remove_from_collection);
DECLARE_SIMPLE (activate_plugin_browser_reset);
DECLARE_SIMPLE (activate_plugin_collection_add);
DECLARE_SIMPLE (activate_plugin_collection_rename);
DECLARE_SIMPLE (activate_plugin_collection_remove);

/**
 * @}
 */

#undef DECLARE_SIMPLE

#endif
