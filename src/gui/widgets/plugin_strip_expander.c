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

#include "audio/channel.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/plugin_strip_expander.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#define PLUGIN_STRIP_EXPANDER_WIDGET_TYPE \
  (plugin_strip_expander_widget_get_type ())
G_DEFINE_TYPE (
  PluginStripExpanderWidget,
  plugin_strip_expander_widget,
  EXPANDER_BOX_WIDGET_TYPE)

/**
 * Queues a redraw of the given slot.
 */
void
plugin_strip_expander_widget_redraw_slot (
  PluginStripExpanderWidget * self,
  int                         slot)
{
  switch (self->slot_type)
    {
    case PLUGIN_SLOT_INSERT:
    case PLUGIN_SLOT_MIDI_FX:
      gtk_widget_queue_draw (
        GTK_WIDGET (self->slots[slot]));
      break;
    default:
      break;
    }
}

/**
 * Unsets state flags and redraws the widget at the
 * given slot.
 *
 * @param slot Slot, or -1 for all slots.
 * @param set True to set, false to unset.
 */
void
plugin_strip_expander_widget_set_state_flags (
  PluginStripExpanderWidget * self,
  int                         slot,
  GtkStateFlags               flags,
  bool                        set)
{
  switch (self->slot_type)
    {
    case PLUGIN_SLOT_INSERT:
    case PLUGIN_SLOT_MIDI_FX:
      if (slot == -1)
        {
          for (int i = 0; i < STRIP_SIZE; i++)
            {
              plugin_strip_expander_widget_set_state_flags (
                self, i, flags, set);
            }
        }
      else
        {
          channel_slot_widget_set_state_flags (
            self->slots[slot], flags, set);
        }
      break;
    default:
      break;
    }
}

/**
 * Refreshes each field.
 */
void
plugin_strip_expander_widget_refresh (
  PluginStripExpanderWidget * self)
{
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      switch (self->slot_type)
        {
        case PLUGIN_SLOT_INSERT:
          {
            Channel * ch =
              track_get_channel (self->track);
            Plugin * pl = ch->inserts[i];
            plugin_strip_expander_widget_set_state_flags (
              self, i,
              GTK_STATE_FLAG_SELECTED,
              pl && plugin_is_selected (pl));
            gtk_widget_queue_draw (
              GTK_WIDGET (self->slots[i]));
          }
          break;
        case PLUGIN_SLOT_MIDI_FX:
          {
            Channel * ch =
              track_get_channel (self->track);
            Plugin * pl = ch->midi_fx[i];
            plugin_strip_expander_widget_set_state_flags (
              self, i,
              GTK_STATE_FLAG_SELECTED,
              pl && plugin_is_selected (pl));
            gtk_widget_queue_draw (
              GTK_WIDGET (self->slots[i]));
          }
          break;
        default:
          break;
        }
    }
}

static void
on_reveal_changed (
  ExpanderBoxWidget *         expander_box,
  bool                        revealed,
  PluginStripExpanderWidget * self)
{
  if (self->position == PSE_POSITION_CHANNEL)
    {
      Channel * ch =
        track_get_channel (self->track);
      if (self->slot_type == PLUGIN_SLOT_INSERT)
        {
          g_settings_set_boolean (
            S_UI_MIXER, "inserts-expanded",
            revealed);
          EVENTS_PUSH (
            ET_MIXER_CHANNEL_INSERTS_EXPANDED_CHANGED,
            ch);
        }
      else if (self->slot_type ==
                 PLUGIN_SLOT_MIDI_FX)
        {
          g_settings_set_boolean (
            S_UI_MIXER, "midi-fx-expanded",
            revealed);
          EVENTS_PUSH (
            ET_MIXER_CHANNEL_MIDI_FX_EXPANDED_CHANGED,
            ch);
        }
    }
}

/**
 * Sets up the PluginStripExpanderWidget.
 */
void
plugin_strip_expander_widget_setup (
  PluginStripExpanderWidget * self,
  PluginSlotType              slot_type,
  PluginStripExpanderPosition position,
  Track *                     track)
{
  /* set name and icon */
  char fullstr[200];
  bool is_midi = false;
  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      strcpy (fullstr, _("Inserts"));
      is_midi =
        track &&
        track->out_signal_type == TYPE_EVENT;
      break;
    case PLUGIN_SLOT_MIDI_FX:
      strcpy (fullstr, "MIDI FX");
      is_midi = true;
      break;
    default:
      g_return_if_reached ();
      break;
    }
  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    fullstr);

  if (is_midi)
    {
      expander_box_widget_set_icon_name (
        Z_EXPANDER_BOX_WIDGET (self),
        "midi-insert");
    }
  else
    {
      expander_box_widget_set_icon_name (
        Z_EXPANDER_BOX_WIDGET (self),
        "audio-insert");
    }

  if (track != self->track ||
      slot_type != self->slot_type ||
      position != self->position)
    {
      /* remove children */
      z_gtk_widget_destroy_all_children (
        GTK_WIDGET (self->box));

      Channel * ch = track_get_channel (track);
      g_return_if_fail (ch);
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          GtkBox * strip_box =
            GTK_BOX (
              gtk_box_new (
                GTK_ORIENTATION_HORIZONTAL, 0));
          self->strip_boxes[i] = strip_box;

          switch (slot_type)
            {
            case PLUGIN_SLOT_INSERT:
            case PLUGIN_SLOT_MIDI_FX:
              {
                ChannelSlotWidget * csw =
                  channel_slot_widget_new (
                    i, track, slot_type,
                    position ==
                      PSE_POSITION_CHANNEL);
                self->slots[i] = csw;
                gtk_box_append (
                  strip_box, GTK_WIDGET (csw));
              }
              break;
            default:
              g_warn_if_reached ();
              break;
            }

          gtk_box_append (
            self->box, GTK_WIDGET (strip_box));
        }
    }

  self->track = track;
  self->slot_type = slot_type;
  self->position = position;

  switch (position)
    {
    case PSE_POSITION_INSPECTOR:
      gtk_widget_set_size_request (
        GTK_WIDGET (self->scroll), -1, 124);
      break;
    case PSE_POSITION_CHANNEL:
      gtk_widget_set_size_request (
        GTK_WIDGET (self->scroll), -1, 68);
      if (slot_type == PLUGIN_SLOT_INSERT ||
          slot_type == PLUGIN_SLOT_MIDI_FX)
        {
          expander_box_widget_set_reveal_callback (
            Z_EXPANDER_BOX_WIDGET (self),
            (ExpanderBoxRevealFunc)
            on_reveal_changed, self);
        }
      break;
    }

  plugin_strip_expander_widget_refresh (self);
}

static void
plugin_strip_expander_widget_class_init (
  PluginStripExpanderWidgetClass * klass)
{
}

static void
plugin_strip_expander_widget_init (
  PluginStripExpanderWidget * self)
{
  self->scroll =
    GTK_SCROLLED_WINDOW (
      gtk_scrolled_window_new ());
  gtk_widget_set_vexpand (
    GTK_WIDGET (self->scroll), 1);
  gtk_widget_set_visible (
    GTK_WIDGET (self->scroll), 1);
  /*gtk_scrolled_window_set_shadow_type (*/
    /*self->scroll, GTK_SHADOW_ETCHED_IN);*/

  self->viewport =
    GTK_VIEWPORT (
      gtk_viewport_new (NULL, NULL));
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (self->scroll),
    GTK_WIDGET (self->viewport));

  self->box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_viewport_set_child (
    GTK_VIEWPORT (self->viewport),
    GTK_WIDGET (self->box));

  expander_box_widget_add_content (
    Z_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->scroll));
}
