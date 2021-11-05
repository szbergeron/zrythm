/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel_sends_expander.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/fader_controls_expander.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/plugin_strip_expander.h"
#include "gui/widgets/ports_expander.h"
#include "gui/widgets/track_input_expander.h"
#include "gui/widgets/track_properties_expander.h"
#include "gui/widgets/text_expander.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  InspectorTrackWidget,
  inspector_track_widget,
  GTK_TYPE_BOX)

static void
reveal_cb (
  ExpanderBoxWidget * expander,
  const bool    revealed,
  void *        user_data)
{
  InspectorTrackWidget * self =
    Z_INSPECTOR_TRACK_WIDGET (user_data);

#define SET_SETTING(mem,key) \
  if (expander == Z_EXPANDER_BOX_WIDGET (self->mem)) \
    { \
      g_settings_set_boolean ( \
        S_UI_INSPECTOR, \
        "track-" key "-expanded", revealed); \
    }

  SET_SETTING (track_info, "properties");
  SET_SETTING (inputs, "inputs");
  SET_SETTING (outputs, "outputs");
  SET_SETTING (sends, "sends");
  SET_SETTING (controls, "controls");
  SET_SETTING (inserts, "inserts");
  SET_SETTING (midi_fx, "midi-fx");
  SET_SETTING (fader, "fader");
  SET_SETTING (comment, "comment");

#undef SET_SETTING
}

static void
setup_color (
  InspectorTrackWidget * self,
  Track *                track)
{
  if (track)
    {
      color_area_widget_setup_track (
        self->color, track);
    }
  else
    {
      GdkRGBA color = { 1, 1, 1, 1 };
      color_area_widget_set_color (
        self->color, &color);
    }
}

/**
 * Shows the inspector page for the given tracklist
 * selection.
 *
 * @param set_notebook_page Whether to set the
 *   current left panel tab to the track page.
 */
void
inspector_track_widget_show_tracks (
  InspectorTrackWidget * self,
  TracklistSelections *  tls,
  bool                   set_notebook_page)
{
  g_debug ("showing %d tracks", tls->num_tracks);

  if (set_notebook_page &&
      gtk_notebook_get_current_page (
        GTK_NOTEBOOK (
          MW_LEFT_DOCK_EDGE->inspector_notebook)) !=
      0)
    {
      gtk_notebook_set_current_page (
        GTK_NOTEBOOK (
          MW_LEFT_DOCK_EDGE->inspector_notebook), 0);
    }

  /* show info for first track */
  Track * track = NULL;
  if (tls->num_tracks > 0)
    {
      track = tls->tracks[0];
      g_debug ("track %s", track->name);

      setup_color (self, track);

      track_properties_expander_widget_refresh (
        self->track_info, track);

      gtk_widget_set_visible (
        GTK_WIDGET (self->sends), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->outputs), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->controls), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->inputs), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->inserts), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->midi_fx), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->fader), false);
      gtk_widget_set_visible (
        GTK_WIDGET (self->comment), true);

      text_expander_widget_setup (
        self->comment, true, track_get_comment,
        track_comment_setter, track);
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self->comment),
        _("Comment"));

      if (track_type_has_channel (track->type))
        {
          gtk_widget_set_visible (
            GTK_WIDGET (self->sends), true);
#if 0
          gtk_widget_set_visible (
            GTK_WIDGET (self->outputs), true);
          gtk_widget_set_visible (
            GTK_WIDGET (self->controls), true);
#endif
          gtk_widget_set_visible (
            GTK_WIDGET (self->fader), true);
          gtk_widget_set_visible (
            GTK_WIDGET (self->inserts), true);

          if (track_has_inputs (track))
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self->inputs), true);
              track_input_expander_widget_refresh (
                self->inputs, track);
            }
          if (track->in_signal_type == TYPE_EVENT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self->midi_fx), true);
              plugin_strip_expander_widget_setup (
                self->midi_fx, PLUGIN_SLOT_MIDI_FX,
                PSE_POSITION_INSPECTOR, track);
            }
          ports_expander_widget_setup_track (
            self->outputs,
            track, PE_TRACK_PORT_TYPE_SENDS);
          ports_expander_widget_setup_track (
            self->controls,
            track, PE_TRACK_PORT_TYPE_CONTROLS);

          plugin_strip_expander_widget_setup (
            self->inserts, PLUGIN_SLOT_INSERT,
            PSE_POSITION_INSPECTOR, track);

          fader_controls_expander_widget_setup (
            self->fader, track);

          channel_sends_expander_widget_setup (
            self->sends, CSE_POSITION_INSPECTOR,
            track);
        }
    }
  else /* no tracks selected */
    {
      track_properties_expander_widget_refresh (
        self->track_info, NULL);
      ports_expander_widget_setup_track (
        self->outputs,
        track, PE_TRACK_PORT_TYPE_SENDS);
      ports_expander_widget_setup_track (
        self->controls,
        track, PE_TRACK_PORT_TYPE_CONTROLS);
      text_expander_widget_setup (
        self->comment, false, NULL, NULL, NULL);

      setup_color (self, NULL);
    }
}

/**
 * Sets up the inspector track widget for the first
 * time.
 */
void
inspector_track_widget_setup (
  InspectorTrackWidget * self,
  TracklistSelections *  tls)
{
  g_return_if_fail (tls);
  if (tls->num_tracks == 0)
    {
      g_critical (
        "no tracks selected. this should never "
        "happen");
      return;
    }

  Track * track = tls->tracks[0];
  g_return_if_fail (track);

  track_properties_expander_widget_setup (
    self->track_info, track);
}

InspectorTrackWidget *
inspector_track_widget_new (void)
{
  InspectorTrackWidget * self =
    g_object_new (
      INSPECTOR_TRACK_WIDGET_TYPE, NULL);

  return self;
}

/**
 * Prepare for finalization.
 */
void
inspector_track_widget_tear_down (
  InspectorTrackWidget * self)
{
  g_message ("tearing down %p...", self);

  if (self->fader)
    {
      fader_controls_expander_widget_tear_down (
        self->fader);
    }

  g_message ("done");
}

static void
inspector_track_widget_class_init (
  InspectorTrackWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "inspector_track.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    GTK_WIDGET_CLASS (klass), \
    InspectorTrackWidget, \
    child);

  BIND_CHILD (track_info);
  BIND_CHILD (sends);
  BIND_CHILD (outputs);
  BIND_CHILD (controls);
  BIND_CHILD (inputs);
  BIND_CHILD (inserts);
  BIND_CHILD (midi_fx);
  BIND_CHILD (fader);
  BIND_CHILD (comment);
  BIND_CHILD (color);

#undef BIND_CHILD
}

static void
inspector_track_widget_init (
  InspectorTrackWidget * self)
{
  g_type_ensure (
    TRACK_PROPERTIES_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    TRACK_INPUT_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    PORTS_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    PLUGIN_STRIP_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    CHANNEL_SENDS_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    FADER_CONTROLS_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    TEXT_EXPANDER_WIDGET_TYPE);
  g_type_ensure (
    COLOR_AREA_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "inspector");

  expander_box_widget_set_vexpand (
    Z_EXPANDER_BOX_WIDGET (self->inserts), false);
  expander_box_widget_set_vexpand (
    Z_EXPANDER_BOX_WIDGET (self->midi_fx), false);
  expander_box_widget_set_vexpand (
    Z_EXPANDER_BOX_WIDGET (self->comment), false);

  /* set states */
#define SET_STATE(mem,key) \
  expander_box_widget_set_reveal ( \
    Z_EXPANDER_BOX_WIDGET (self->mem), \
    g_settings_get_boolean ( \
      S_UI_INSPECTOR, \
      "track-" key "-expanded")); \
  expander_box_widget_set_reveal_callback ( \
    Z_EXPANDER_BOX_WIDGET (self->mem), \
    reveal_cb, self)

  SET_STATE (track_info, "properties");
  SET_STATE (inputs, "inputs");
  SET_STATE (outputs, "outputs");
  SET_STATE (sends, "sends");
  SET_STATE (controls, "controls");
  SET_STATE (inserts, "inserts");
  SET_STATE (midi_fx, "midi-fx");
  SET_STATE (fader, "fader");
  SET_STATE (comment, "comment");

#undef SET_STATE
}
