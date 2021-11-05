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

#include "audio/control_port.h"
#include "audio/midi_event.h"
#include "audio/midi_mapping.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

static MidiMapping *
create_midi_mapping (void)
{
  MidiMapping * self = object_new (MidiMapping);

  self->schema_version =
    MIDI_MAPPING_SCHEMA_VERSION;
  self->gobj =
    wrapped_object_with_change_signal_new (
      self, WRAPPED_OBJECT_TYPE_MIDI_MAPPING);

  return self;
}

/**
 * Initializes the MidiMappings after a Project
 * is loaded.
 */
void
midi_mappings_init_loaded (
  MidiMappings * self)
{
  self->mappings_size = (size_t) self->num_mappings;

  for (int i = 0; i < self->num_mappings; i++)
    {
      MidiMapping * mapping = self->mappings[i];
      mapping->dest =
        port_find_from_identifier (
          &mapping->dest_id);
    }
}

/**
 * Binds the CC represented by the given raw buffer
 * (must be size 3) to the given Port.
 *
 * @param idx Index to insert at.
 * @param buf The buffer used for matching at [0] and
 *   [1].
 * @param device_port Device port, if custom mapping.
 */
void
midi_mappings_bind_at (
  MidiMappings * self,
  midi_byte_t *  buf,
  ExtPort *      device_port,
  Port *         dest_port,
  int            idx,
  bool           fire_events)
{
  g_return_if_fail (
    self && buf && dest_port);

  array_double_size_if_full (
    self->mappings, self->num_mappings,
    self->mappings_size, MidiMapping);

  /* if inserting, move later mappings further */
  for (int i = self->num_mappings; i > idx; i--)
    {
      self->mappings[i] = self->mappings[i - 1];
    }
  self->num_mappings++;

  MidiMapping * mapping = midi_mapping_new ();
  self->mappings[idx] = mapping;

  memcpy (
    mapping->key, buf, sizeof (midi_byte_t) * 3);
  if (device_port)
    {
      mapping->device_port =
        ext_port_clone (device_port);
    }
  mapping->dest_id = dest_port->id;
  mapping->dest = dest_port;
  g_atomic_int_set (
    &mapping->enabled, (guint) true);

  char str[100];
  midi_ctrl_change_get_ch_and_description (
    buf, str);

  if (!(dest_port->id.flags &
          PORT_FLAG_MIDI_AUTOMATABLE))
    {
      g_message (
        "bounded MIDI mapping from %s to %s",
        str, dest_port->id.label);
    }

  if (fire_events && ZRYTHM_HAVE_UI)
    {
      EVENTS_PUSH (ET_MIDI_BINDINGS_CHANGED, NULL);
    }
}

/**
 * Unbinds the given binding.
 *
 * @note This must be called inside a port operation
 *   lock, such as inside an undoable action.
 */
void
midi_mappings_unbind (
  MidiMappings * self,
  int            idx,
  bool           fire_events)
{
  g_return_if_fail (self && idx >= 0);

  MidiMapping * mapping_before =
    self->mappings[idx];

  for (int i = self->num_mappings - 2;
       i >= idx; i--)
    {
      self->mappings[i] = self->mappings[i + 1];
    }
  self->num_mappings--;

  object_free_w_func_and_null (
    midi_mapping_free, mapping_before);

  if (fire_events && ZRYTHM_HAVE_UI)
    {
      EVENTS_PUSH (ET_MIDI_BINDINGS_CHANGED, NULL);
    }
}

void
midi_mapping_set_enabled (
  MidiMapping * self,
  bool          enabled)
{
  g_atomic_int_set (
    &self->enabled, (guint) enabled);
}

int
midi_mapping_get_index (
  MidiMappings * self,
  MidiMapping *  mapping)
{
  for (int i = 0; i < self->num_mappings; i++)
    {
      if (self->mappings[i] == mapping)
        return i;
    }
  g_return_val_if_reached (-1);
}

MidiMapping *
midi_mapping_new (void)
{
  MidiMapping * self = create_midi_mapping ();

  return self;
}

MidiMapping *
midi_mapping_clone (
  const MidiMapping * src)
{
  MidiMapping * self = create_midi_mapping ();

  memcpy (
    &self->key[0], &src->key[0],
    3 * sizeof (midi_byte_t));
  if (src->device_port)
    self->device_port =
      ext_port_clone (src->device_port);
  port_identifier_copy (
    &self->dest_id, &src->dest_id);
  g_atomic_int_set (
    &self->enabled,
    (guint) g_atomic_int_get (&src->enabled));

  return self;
}

void
midi_mapping_free (
  MidiMapping * self)
{
  object_free_w_func_and_null (
    ext_port_free, self->device_port);

  g_clear_object (&self->gobj);

  object_zero_and_free (self);
}

static void
apply_mapping (
  MidiMapping * mapping,
  midi_byte_t * buf)
{
  g_return_if_fail (mapping->dest);

  Port * dest = mapping->dest;
  if (dest->id.type == TYPE_CONTROL)
    {
      /* if toggle, reverse value */
      if (dest->id.flags & PORT_FLAG_TOGGLE)
        {
          control_port_set_toggled (
            dest,
            !control_port_is_toggled (
               dest),
            F_PUBLISH_EVENTS);
        }
      /* else if not toggle set the control
       * value received */
      else
        {
          float normalized_val =
            (float) buf[2] / 127.f;
          port_set_control_value (
            dest, normalized_val,
            F_NORMALIZED, F_PUBLISH_EVENTS);
        }
    }
  else if (dest->id.type == TYPE_EVENT)
    {
      if (dest->id.flags2 &
            PORT_FLAG2_TRANSPORT_ROLL)
        {
          transport_request_roll (TRANSPORT);
        }
      else if (dest->id.flags2 &
            PORT_FLAG2_TRANSPORT_STOP)
        {
          transport_request_pause (
            TRANSPORT);
        }
      else if (dest->id.flags2 &
            PORT_FLAG2_TRANSPORT_BACKWARD)
        {
          transport_move_backward (
            TRANSPORT);
        }
      else if (dest->id.flags2 &
            PORT_FLAG2_TRANSPORT_FORWARD)
        {
          transport_move_forward (TRANSPORT);
        }
      else if (dest->id.flags2 &
            PORT_FLAG2_TRANSPORT_LOOP_TOGGLE)
        {
          transport_set_loop (
            TRANSPORT, !TRANSPORT->loop);
        }
      else if (dest->id.flags2 &
            PORT_FLAG2_TRANSPORT_REC_TOGGLE)
        {
          transport_set_recording (
            TRANSPORT,
            !TRANSPORT->recording,
            F_PUBLISH_EVENTS);
        }
    }
}

/**
 * Applies the events to the appropriate mapping.
 *
 * This is used only for TrackProcessor.cc_mappings.
 *
 * @note Must only be called while transport is
 *   recording.
 */
void
midi_mappings_apply_from_cc_events (
  MidiMappings * self,
  MidiEvents *   events,
  bool           queued)
{
  /* queued not implemented yet */
  g_return_if_fail (!queued);

  for (int i = 0; i < events->num_events; i++)
    {
      MidiEvent * ev = &events->events[i];
      if (ev->raw_buffer[0] >=
            (midi_byte_t) MIDI_CH1_CTRL_CHANGE &&
          ev->raw_buffer[0] <=
            (midi_byte_t)
            (MIDI_CH1_CTRL_CHANGE | 15))
        {
          midi_byte_t channel =
            (midi_byte_t)
            ((ev->raw_buffer[0] & 0xf) + 1);
          midi_byte_t controller = ev->raw_buffer[1];
          MidiMapping * mapping =
            self->mappings[
              (channel - 1) * 128 + controller];
          apply_mapping (
            mapping, ev->raw_buffer);
        }
    }
}

/**
 * Applies the given buffer to the matching ports.
 */
void
midi_mappings_apply (
  MidiMappings * self,
  midi_byte_t *  buf)
{
  for (int i = 0; i < self->num_mappings; i++)
    {
      MidiMapping * mapping = self->mappings[i];

      if (g_atomic_int_get (&mapping->enabled) &&
          mapping->key[0] == buf[0] &&
          mapping->key[1] == buf[1])
        {
          apply_mapping (mapping, buf);
        }
    }
}

/**
 * Returns a newly allocated MidiMappings.
 */
MidiMappings *
midi_mappings_new ()
{
  MidiMappings * self = object_new (MidiMappings);
  self->schema_version =
    MIDI_MAPPINGS_SCHEMA_VERSION;

  self->mappings_size = 4;
  self->mappings =
    object_new_n (self->mappings_size, MidiMapping);

  return self;
}

/**
 * Get MIDI mappings for the given port.
 *
 * @param size Size to set.
 *
 * @return a newly allocated array that must be
 * free'd with free().
 */
MidiMapping **
midi_mappings_get_for_port (
  MidiMappings * self,
  Port *         dest_port,
  int *          count)
{
  g_return_val_if_fail (self && dest_port, NULL);

  MidiMapping ** arr = NULL;
  *count = 0;
  for (int i = 0; i < self->num_mappings; i++)
    {
      MidiMapping * mapping =
        self->mappings[i];
      if (mapping->dest == dest_port)
        {
          arr =
            g_realloc (
              arr,
              (size_t) (*count + 1) *
                sizeof (MidiMapping *));
          array_append (
            arr, *count, mapping);
        }
    }
  return arr;
}

MidiMappings *
midi_mappings_clone (
  const MidiMappings * src)
{
  MidiMappings * self = object_new (MidiMappings);
  self->schema_version =
    MIDI_MAPPINGS_SCHEMA_VERSION;

  self->mappings =
    object_new_n (
      (size_t) src->num_mappings, MidiMapping);
  for (int i = 0; i < src->num_mappings; i++)
    {
      self->mappings[i] =
        midi_mapping_clone (src->mappings[i]);
    }
  self->num_mappings = src->num_mappings;

  return self;
}

void
midi_mappings_free (
  MidiMappings * self)
{
  for (int i = 0; i < self->num_mappings; i++)
    {
      object_free_w_func_and_null (
        midi_mapping_free, self->mappings[i]);
    }
  object_zero_and_free (self->mappings);

  object_zero_and_free (self);
}
