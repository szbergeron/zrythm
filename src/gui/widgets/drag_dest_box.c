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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Box used as destination for DnD.
 */

#include "actions/tracklist_selections.h"
#include "actions/mixer_selections_action.h"
#include "audio/channel.h"
#include "audio/modulator_track.h"
#include "audio/port_connections_manager.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/accel.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/file_browser.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/plugin_settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/strv_builder.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  DragDestBoxWidget,
  drag_dest_box_widget,
  GTK_TYPE_BOX)

static void
on_dnd_leave (
  GtkDropTarget *     drop_target,
  DragDestBoxWidget * self)
{
  GdkDrop * drop =
    gtk_drop_target_get_current_drop (
      drop_target);

  gdk_drop_read_value_async (
    drop, G_TYPE_STRING,
    0, NULL, NULL, NULL);
  const GValue * str_val =
    gdk_drop_read_value_finish (drop, NULL, NULL);
  Track * dropped_track = NULL;
  if (str_val)
    {
      const char * str =
        g_value_get_string (str_val);
      if (g_str_has_prefix (str, TRACK_DND_PREFIX))
        {
          sscanf (
            str, TRACK_DND_PREFIX "%p",
            &dropped_track);
        }
    }

  if (dropped_track)
    {
      /* unhighlight bottom part of last track */
      Track * track =
        tracklist_get_last_track (
          TRACKLIST,
          TRACKLIST_PIN_OPTION_UNPINNED_ONLY, true);
      track_widget_do_highlight (
        track->widget, 0, 0, 0);
    }
}

static GdkDragAction
on_dnd_motion (
  GtkDropTarget * drop_target,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  /*DragDestBoxWidget * self =*/
    /*Z_DRAG_DEST_BOX_WIDGET (user_data);*/

  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (drop_target));

  GdkDrop * drop =
    gtk_drop_target_get_current_drop (
      drop_target);

  gdk_drop_read_value_async (
    drop, G_TYPE_STRING,
    0, NULL, NULL, NULL);
  const GValue * str_val =
    gdk_drop_read_value_finish (drop, NULL, NULL);
  SupportedFile * supported_file = NULL;
  Track * dropped_track = NULL;
  Plugin * pl = NULL;
  PluginDescriptor * pl_descr = NULL;
  if (str_val)
    {
      const char * str =
        g_value_get_string (str_val);
      if (g_str_has_prefix (
            str, SUPPORTED_FILE_DND_PREFIX))
        {
          sscanf (
            str, SUPPORTED_FILE_DND_PREFIX "%p",
            &supported_file);
        }
      else if (g_str_has_prefix (
                 str, PLUGIN_DESCRIPTOR_DND_PREFIX))
        {
          sscanf (
            str, PLUGIN_DESCRIPTOR_DND_PREFIX "%p",
            &pl_descr);
        }
      else if (g_str_has_prefix (
                 str, PLUGIN_DND_PREFIX))
        {
          sscanf (
            str, PLUGIN_DND_PREFIX "%p", &pl);
        }
      else if (g_str_has_prefix (
                 str, TRACK_DND_PREFIX))
        {
          sscanf (
            str, TRACK_DND_PREFIX "%p",
            &dropped_track);
        }
    }

  gdk_drop_read_value_async (
    drop, GDK_TYPE_FILE_LIST,
    0, NULL, NULL, NULL);
  const GValue * file_list_val =
    gdk_drop_read_value_finish (drop, NULL, NULL);
  gdk_drop_read_value_async (
    drop, G_TYPE_FILE,
    0, NULL, NULL, NULL);
  const GValue * file_val =
    gdk_drop_read_value_finish (drop, NULL, NULL);

  if (file_list_val || file_val)
    {
      /* defer to drag_data_received */
      /*self->defer_drag_motion_status = 1;*/

      /*gtk_drag_get_data (*/
        /*widget, context, target, time);*/

      return GDK_ACTION_COPY;
    }
  else if (supported_file)
    {
      return GDK_ACTION_COPY;
    }
  else if (pl_descr)
    {
      /*gtk_drag_highlight (widget);*/
      return GDK_ACTION_COPY;
    }
  else if (pl)
    {
      if (state & GDK_CONTROL_MASK)
        return GDK_ACTION_COPY;
      else
        return GDK_ACTION_MOVE;
    }
  else if (dropped_track)
    {
      /*gtk_drag_unhighlight (widget);*/

      /* highlight bottom part of last track */
      Track * track =
        tracklist_get_last_track (
          TRACKLIST,
          TRACKLIST_PIN_OPTION_UNPINNED_ONLY, true);
      int track_height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget));
      track_widget_do_highlight (
        track->widget, 0,
        track_height - 1, 1);

      if (state & GDK_CONTROL_MASK)
        return GDK_ACTION_COPY;
      else
        return GDK_ACTION_MOVE;
    }
  else
    {
      /*gtk_drag_unhighlight (widget);*/
      return 0;
    }

  return 0;
}

static gboolean
on_dnd_drop (
  GtkDropTarget * drop_target,
  const GValue  * value,
  double          x,
  double          y,
  gpointer        data)
{
  DragDestBoxWidget * self =
    Z_DRAG_DEST_BOX_WIDGET (data);

  GdkDragAction action =
    z_gtk_drop_target_get_selected_action (
      drop_target);

  SupportedFile * file = NULL;
  PluginDescriptor * pd = NULL;
  Plugin * pl = NULL;
  Track * track = NULL;
  if (G_VALUE_HOLDS_STRING (value))
    {
      const char * str = g_value_get_string (value);
      if (g_str_has_prefix (
            str, SUPPORTED_FILE_DND_PREFIX))
        {
          sscanf (
            str, SUPPORTED_FILE_DND_PREFIX "%p",
            &file);
        }
      else if (g_str_has_prefix (
                 str, PLUGIN_DESCRIPTOR_DND_PREFIX))
        {
          sscanf (
            str, PLUGIN_DESCRIPTOR_DND_PREFIX "%p",
            &pd);
        }
      else if (g_str_has_prefix (
                 str, PLUGIN_DND_PREFIX))
        {
          sscanf (
            str, PLUGIN_DND_PREFIX "%p", &pl);
        }
      else if (g_str_has_prefix (
                 str, TRACK_DND_PREFIX))
        {
          sscanf (
            str, TRACK_DND_PREFIX "%p", &track);
        }
    }

  if (
    G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)
    || G_VALUE_HOLDS (value, G_TYPE_FILE)
    || file)
    {
      char ** uris = NULL;
      if (G_VALUE_HOLDS (value, G_TYPE_FILE))
        {
          GFile * gfile =
            g_value_get_object (value);
          StrvBuilder * uris_builder =
            strv_builder_new ();
          char * uri = g_file_get_uri (gfile);
          strv_builder_add (uris_builder, uri);
          uris =
            strv_builder_end (uris_builder);
        }
      else if (G_VALUE_HOLDS (
                 value, GDK_TYPE_FILE_LIST))
        {
          StrvBuilder * uris_builder =
            strv_builder_new ();
          GSList * l;
          for (l = g_value_get_boxed (value); l;
               l = l->next)
            {
              char * uri = g_file_get_uri (l->data);
              strv_builder_add (uris_builder, uri);
              g_free (uri);
            }
          uris =
            strv_builder_end (uris_builder);
        }

      tracklist_handle_file_drop (
        TRACKLIST, uris, file, NULL, NULL, NULL,
        true);
      return true;
    }
  else if (pd)
    {
      if (self->type == DRAG_DEST_BOX_TYPE_MIXER
          ||
          self->type == DRAG_DEST_BOX_TYPE_TRACKLIST)
        {
          TrackType tt =
            track_get_type_from_plugin_descriptor (
              pd);

          PluginSetting * setting =
            plugin_setting_new_default (pd);
          GError * err = NULL;
          bool ret =
            tracklist_selections_action_perform_create (
              tt, setting, NULL,
              TRACKLIST->num_tracks,
              PLAYHEAD, 1, -1, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _("Failed to create track"));
            }
          plugin_setting_free (setting);
        }
      else
        {
          PluginSetting * setting =
            plugin_setting_new_default (pd);
          GError * err = NULL;
          bool ret =
            mixer_selections_action_perform_create (
              PLUGIN_SLOT_MODULATOR,
              track_get_name_hash (
                P_MODULATOR_TRACK),
              P_MODULATOR_TRACK->num_modulators,
              setting, 1, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _("Failed to create plugin"));
            }
          plugin_setting_free (setting);
        }

      return true;
    }
  else if (pl)
    {
      /* NOTE this is a cloned pointer, don't use
       * it */
      g_warn_if_fail (pl);

      GError * err = NULL;
      bool ret;
      if (action == GDK_ACTION_COPY)
        {
          ret =
            mixer_selections_action_perform_copy (
              MIXER_SELECTIONS,
              PORT_CONNECTIONS_MGR,
              PLUGIN_SLOT_INSERT,
              0, 0, &err);
        }
      else if (action == GDK_ACTION_MOVE)
        {
          ret =
            mixer_selections_action_perform_move (
              MIXER_SELECTIONS,
              PORT_CONNECTIONS_MGR,
              PLUGIN_SLOT_INSERT,
              0, 0, &err);
        }
      else
        g_return_val_if_reached (true);

      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s",
            _("Failed to move or copy plugin"));
        }

      return true;
    }
  else if (track)
    {
      tracklist_selections_select_foldable_children (
        TRACKLIST_SELECTIONS);
      int pos =
        tracklist_get_last_pos (
          TRACKLIST,
          TRACKLIST_PIN_OPTION_UNPINNED_ONLY, true);
      pos++;

      GError * err = NULL;
      bool ret;
      if (action == GDK_ACTION_COPY)
        {
          ret =
            tracklist_selections_action_perform_copy (
              TRACKLIST_SELECTIONS,
              PORT_CONNECTIONS_MGR, pos, &err);
        }
      else if (action == GDK_ACTION_MOVE)
        {
          ret =
            tracklist_selections_action_perform_move (
              TRACKLIST_SELECTIONS,
              PORT_CONNECTIONS_MGR, pos, &err);
        }
      else
        g_return_val_if_reached (true);

      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s",
            _("Failed to move or copy track"));
        }

      return true;
    }

  /* drag was not accepted */
  return false;
}

static void
show_context_menu (DragDestBoxWidget * self)
{
  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem =
    z_gtk_create_menu_item (
      _("Add _MIDI Track"), NULL,
      "app.create-midi-track");
  g_menu_append_item (menu, menuitem);

  menuitem =
    z_gtk_create_menu_item (
      _("Add Audio Track"), NULL,
      "app.create-audio-track");
  g_menu_append_item (menu, menuitem);

  GMenu * bus_submenu = g_menu_new ();
  menuitem =
    z_gtk_create_menu_item (
      _(track_type_to_string (
          TRACK_TYPE_AUDIO_BUS)),
      NULL, "app.create-audio-bus-track");
  g_menu_append_item (bus_submenu, menuitem);
  menuitem =
    z_gtk_create_menu_item (
      _(track_type_to_string (
          TRACK_TYPE_MIDI_BUS)),
      NULL, "app.create-midi-bus-track");
  g_menu_append_item (bus_submenu, menuitem);
  g_menu_append_section (
    menu, _("Add FX Track"),
    G_MENU_MODEL (bus_submenu));

  GMenu * group_submenu = g_menu_new ();
  menuitem =
    z_gtk_create_menu_item (
      _(track_type_to_string (
          TRACK_TYPE_AUDIO_GROUP)),
      NULL, "app.create-audio-group-track");
  g_menu_append_item (group_submenu, menuitem);
  menuitem =
    z_gtk_create_menu_item (
      _(track_type_to_string (
          TRACK_TYPE_MIDI_GROUP)),
      NULL, "app.create-midi-group-track");
  g_menu_append_item (group_submenu, menuitem);
  g_menu_append_section (
    menu, _("Add Group Track"),
    G_MENU_MODEL (group_submenu));

  menuitem =
    z_gtk_create_menu_item (
      _("Add Folder Track"), NULL,
      "app.create-folder-track");
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    GTK_WIDGET (self), menu);
}

static void
on_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  DragDestBoxWidget * self =
    Z_DRAG_DEST_BOX_WIDGET (user_data);

  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

static void
on_click_pressed (
  GtkGestureClick * gesture,
  gint                   n_press,
  gdouble                x,
  gdouble                y,
  DragDestBoxWidget *    self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS,
    F_PUBLISH_EVENTS);
  tracklist_selections_select_last_visible (
    TRACKLIST_SELECTIONS);

  PROJECT->last_selection =
    SELECTION_TYPE_TRACKLIST;
  EVENTS_PUSH (
    ET_PROJECT_SELECTION_TYPE_CHANGED, NULL);
}

static void
setup_dnd (
  DragDestBoxWidget * self)
{
  GtkDropTarget * drop_target =
    gtk_drop_target_new (
      G_TYPE_INVALID,
      GDK_ACTION_COPY);
  GType types[] = {
    GDK_TYPE_FILE_LIST, G_TYPE_FILE, G_TYPE_STRING };
  gtk_drop_target_set_gtypes (
    drop_target, types, G_N_ELEMENTS (types));
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (drop_target));

  /* connect signal */
  g_signal_connect (
    drop_target, "motion",
    G_CALLBACK (on_dnd_motion), self);
  g_signal_connect (
    drop_target, "drop",
    G_CALLBACK (on_dnd_drop), self);
  g_signal_connect (
    drop_target, "leave",
    G_CALLBACK (on_dnd_leave), self);
}

/**
 * Creates a drag destination box widget.
 */
DragDestBoxWidget *
drag_dest_box_widget_new (
  GtkOrientation  orientation,
  int             spacing,
  DragDestBoxType type)
{
  /* create */
  DragDestBoxWidget * self =
    g_object_new (
      DRAG_DEST_BOX_WIDGET_TYPE, NULL);

  self->type = type;

  switch (type)
    {
    case DRAG_DEST_BOX_TYPE_MIXER:
    case DRAG_DEST_BOX_TYPE_MODULATORS:
      gtk_widget_set_size_request (
        GTK_WIDGET (self), 160, -1);
      break;
    case DRAG_DEST_BOX_TYPE_TRACKLIST:
      gtk_widget_set_size_request (
        GTK_WIDGET (self), -1, 160);
      break;
    }

  /* make expandable */
  gtk_widget_set_vexpand (
    GTK_WIDGET (self), true);
  gtk_widget_set_hexpand (
    GTK_WIDGET (self), true);

  setup_dnd (self);

  return self;
}

/**
 * GTK boilerplate.
 */
static void
drag_dest_box_widget_init (DragDestBoxWidget * self)
{
  self->click =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (self->click), "pressed",
    G_CALLBACK (on_click_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->click));

  self->right_click =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_click),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (self->right_click), "pressed",
    G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->right_click));

#if 0
  self->drag =
    GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->drag));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (motion_controller));
#endif
}

static void
drag_dest_box_widget_class_init (
  DragDestBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "drag-dest-box");
}
