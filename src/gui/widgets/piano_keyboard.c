/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/chord_descriptor.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/piano_keyboard.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  PianoKeyboardWidget, piano_keyboard_widget,
  GTK_TYPE_DRAWING_AREA)

/**
 * Draws an orange circle if the note is enabled.
 */
static void
draw_orange_circle (
  PianoKeyboardWidget * self,
  cairo_t *             cr,
  double                key_width,
  double                cur_offset,
  int                   i)
{
  double height =
    (double)
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  if (self->chord_descr)
    {
      if (self->chord_descr->notes[
            self->start_key + i])
        {
          double circle_radius = key_width / 3.0;
          bool is_black =
            piano_roll_is_key_black (
              self->start_key + i);
          gdk_cairo_set_source_rgba (
            cr, &UI_COLORS->dark_orange);
          cairo_set_source_rgba (cr, 1, 0, 0, 1);
          cairo_arc (
            cr,
            cur_offset + key_width / 2.0,
            is_black ?
              height / 3.0 :
              height / 1.2,
            circle_radius, 0, 2 * M_PI);
          cairo_fill (cr);
        }
    }
}

static void
piano_keyboard_draw_cb (
  GtkDrawingArea * drawing_area,
  cairo_t *        cr,
  int              width,
  int              height,
  gpointer         user_data)
{
  PianoKeyboardWidget * self =
    Z_PIANO_KEYBOARD_WIDGET (user_data);
  GtkWidget * widget = GTK_WIDGET (drawing_area);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  int num_white_keys = 0;
  for (int i = 0; i < self->num_keys; i++)
    {
      if (!piano_roll_is_key_black (
            self->start_key + i))
        num_white_keys++;
    }

  /* draw all white keys */
  double key_width =
    (double) width / (double) num_white_keys;
  double cur_offset = 0.0;
  for (int i = 0; i < self->num_keys; i++)
    {
      bool is_black =
        piano_roll_is_key_black (
          self->start_key + i);
      if (is_black)
        continue;

      cairo_set_source_rgba (cr, 0, 0, 0, 1);
      cairo_rectangle (
        cr, cur_offset, 0, key_width, height);
      cairo_stroke_preserve (cr);
      cairo_set_source_rgba (cr, 1, 1, 1, 1);
      cairo_fill (cr);

      /* draw orange circle if part of chord */
      draw_orange_circle (
        self, cr, key_width, cur_offset, i);

      cur_offset += key_width;
    }

  /* draw all black keys */
  /*int num_black_keys = self->num_keys - num_white_keys;*/
  cur_offset = 0.0;
  for (int i = 0; i < self->num_keys; i++)
    {
      bool is_black =
        piano_roll_is_key_black (
          self->start_key + i);
      if (!is_black)
        {
          bool is_next_black =
            piano_roll_is_next_key_black (
              self->start_key + i);

          if (is_next_black)
            cur_offset += key_width / 2.0;
          else
            cur_offset += key_width;

          continue;
        }

      cairo_set_source_rgba (cr, 0, 0, 0, 1);
      cairo_rectangle (
        cr, cur_offset, 0, key_width, height / 1.4);
      cairo_fill (cr);

      /* draw orange circle if part of chord */
      draw_orange_circle (
        self, cr, key_width, cur_offset, i);

      cur_offset += key_width / 2.0;
    }
}

void
piano_keyboard_widget_refresh (
  PianoKeyboardWidget * self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Creates a piano keyboard widget.
 */
PianoKeyboardWidget *
piano_keyboard_widget_new_for_chord_key (
  ChordDescriptor * descr)
{
  PianoKeyboardWidget * self =
    piano_keyboard_widget_new (
      GTK_ORIENTATION_HORIZONTAL);

  self->chord_descr = descr;
  self->editable = true;
  self->playable = false;
  self->scrollable = false;
  self->start_key = 0;
  self->num_keys = 48;

  return self;
}

/**
 * Creates a piano keyboard widget.
 */
PianoKeyboardWidget *
piano_keyboard_widget_new (
  GtkOrientation orientation)
{
  PianoKeyboardWidget * self =
    g_object_new (PIANO_KEYBOARD_WIDGET_TYPE, NULL);

  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self),
    piano_keyboard_draw_cb,
    self, NULL);

  return self;
}

static void
piano_keyboard_widget_class_init (
  PianoKeyboardWidgetClass * _klass)
{
}

static void
piano_keyboard_widget_init (
  PianoKeyboardWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), true);

  self->editable = true;
  self->playable = false;
  self->scrollable = false;
  self->start_key = 0;
  self->num_keys = 36;
}
